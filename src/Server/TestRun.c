/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define CANBUFLEN 20
#define CAN_PORT "13247"
#define CAN_PORT_NUM 13247
#define CAN_BROADCAST "192.168.69.255"

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;

#define OK 0
#define TRUE (0==0)
#define FALSE (0==1)

int SendSockFD;
struct addrinfo *servinfo, *SendInfo ;


/* Einzelne Hex-Zahlen einlesen */

inline ULONG BuildCANId (char Prio, char Repeat, char FromLine, USHORT FromAdd, char ToLine, USHORT ToAdd, char Group)
{
  return ((Group&0x1)<<1|ToAdd<<2|(ToLine&0xf)<<10|FromAdd<<14|(FromLine&0xf)<<22|(Repeat&0x1)<<26|(Prio&0x3)<<27) ;
}

int getrand (toprange)
{
 return (rand () % toprange) + 1;
}



void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int InitNetwork(void)
{

	struct addrinfo hints;
	int rv;
	struct sockaddr_in RecAddr;
	int val ;

	val = (0==0) ;

	// Set up send-Socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(CAN_BROADCAST, CAN_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(SendInfo = servinfo; SendInfo != NULL; SendInfo = SendInfo->ai_next) {
		if ((SendSockFD = socket(SendInfo->ai_family, SendInfo->ai_socktype,
				SendInfo->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (SendInfo == NULL) {
		fprintf(stderr, "talker: failed to get Send-socket\n");
		return 2;
	}
} ;

int SendCANMessage (ULONG CANID, char Len, char *Data)
{
  int i ;
  char Message [CANBUFLEN] ;
  char *CANIDP ;
  int numbytes;  

  /* Nachricht zusammensetzen */
  CANIDP = (char*)&CANID ;
  for (i=0;i<4;i++) Message[i] = CANIDP[i] ;

  Message[4] = Len ;

  for (i=0;i<Len;i++) Message[i+5] = Data[i] ;
  for (;i<8;i++) Message[i+5] = 0 ;
  
  if ((numbytes = sendto(SendSockFD, Message, 13, 0,
			 SendInfo->ai_addr, SendInfo->ai_addrlen)) != 13) {
    perror("SendCANMessage nicht erfolgreich");
    return(1);
  }

  return (0);
}

void CloseNetwork (void) 
{
  freeaddrinfo(servinfo);

  close(SendSockFD);
}

int main (int argc, char *argv[])
{
  volatile int i,j,k,l ;
  int R,G,B,W ;
  ULONG CanID ;

  clock_t start,end ;

  int Line ;
  int Add ;
  int Command ;
  unsigned char Data[8] ;
  unsigned char DuDu[22] ;
  unsigned char LaLa[22] ;

  InitNetwork () ;

  Line = 1 ; 
  l=0 ;
  for (Add = 7 ; Add<8;Add++) {
    for (i=0;i<22;i++) DuDu[i] = 0 ;
    for (i=0;i<22;i++) LaLa[i] = getrand(255) ;
    Data[0] = 9 ; 
    Data[1] = 0 ;
    Data[2] = 0 ;
    Data[3] = 0 ;
    Data[4] = 0 ;
    Data[5] = 0 ;
    start = clock() ;
    for (;;) {
      for (j=0;j<22;j++) {
	if (LaLa[j]>DuDu[j]) { 
	  DuDu[j]++ ; 
	} else if (LaLa[j]<DuDu[j]) {
	  DuDu[j]-- ;
	} else {
	  LaLa[j] = getrand(255) ;
	} ;
      } ;
      for (j=0;j<7;j++) {
	Data[1] = DuDu[j*3] ;
	Data[2] = DuDu[j*3+1] ;
	Data[3] = DuDu[j*3+2] ;
	CanID = BuildCANId (0,0,0,1,Line,(j&0x7)+16,0) ;
	SendCANMessage (CanID, 5 ,Data) ;
	l++ ;
	for (k=0;k<200000;k++) ;
      } ;
      for (k=0;k<500000;k++) ;
	//	printf("Add : %d, R: %d, G: %d, B: %d\n",j,Data[1],Data[2],Data[3]) ;


    } ;
    
    end = clock () ;
    printf ("Send Messages: %d, clockcycles : %d, cps: %d\n",l,end-start,CLOCKS_PER_SEC) ;
    Data[1] = 0 ;
    Data[2] = 0 ;
    Data[3] = 0 ;
    for (Data[1] = 0;Data[1] <255;Data[1]++) {
      CanID = BuildCANId (0,0,0,1,Line,(Add&0x7)+16,0) ;
      SendCANMessage (CanID, 5 ,Data) ;
      printf("Add : %d, R: %d, G: %d, B: %d\n",Add,Data[1],Data[2],Data[3]) ;
      usleep (20000) ;
    } ;
    Data[1] = 0 ;
    for (Data[2] = 0;Data[2] <255;Data[2]++) {
      CanID = BuildCANId (0,0,0,1,Line,(Add&0x7)+16,0) ;
      SendCANMessage (CanID, 5 ,Data) ;
      printf("Add : %d, R: %d, G: %d, B: %d\n",Add,Data[1],Data[2],Data[3]) ;
      usleep (20000) ;
    } ;
    Data[2] = 0 ;
    for (Data[3] = 0;Data[3] <255;Data[3]++) {
      CanID = BuildCANId (0,0,0,1,Line,(Add&0x7)+16,0) ;
      SendCANMessage (CanID, 5 ,Data) ;
      printf("Add : %d, R: %d, G: %d, B: %d\n",Add,Data[1],Data[2],Data[3]) ;
      usleep (20000) ;
    } ;
  } ;
  CloseNetwork () ;
}

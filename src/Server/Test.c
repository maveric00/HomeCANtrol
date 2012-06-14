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

extern char ** environ ;

void Rolladen (int updown, int Line, int Add, int Num)
{
  ULONG CanID ;
  unsigned char Data[8] ; 
  if
 (updown==1) {
    CanID = BuildCANId (0,0,0,1,Line,Add,0) ;
    Data [0] = 31 ;
    Data [1] = Num;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = Num+5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = Num;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  if (updown==-1) {
    CanID = BuildCANId (0,0,0,1,Line,Add,0) ;
    Data [0] = 31 ;
    Data [1] = Num;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 31 ;
    Data [1] = Num+5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = Num;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  if (updown==0) {
    CanID = BuildCANId (0,0,0,1,Line,Add,0) ;
    Data [0] = 31 ;
    Data [1] = Num;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(100000) ;
    Data [0] = 31 ;
    Data [1] = Num+5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;
}

void Jalousie (int updown)
{
  ULONG CanID ;
  unsigned char Data[8] ; 
  
  CanID = BuildCANId (0,0,0,1,3,104,0) ;
   
  if (updown==1) {
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  if (updown==-1) {
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  if (updown==0) {
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(100000) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;
}

void Beckiefenster (int updown)
{
  ULONG CanID ;
  unsigned char Data[8] ; 
  
  if (updown==1) {
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(200000) ;
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 30 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 30 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;

  if (updown==-1) {
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;

  if (updown==0) {
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(100000) ;
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;
}

void Niklasfenster (int updown)
{
  ULONG CanID ;
  unsigned char Data[8] ; 
  
  if (updown==1) {
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(200000) ;
    Data [0] = 30 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;

  if (updown==-1) {
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] =31 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;

  if (updown==0) {
    CanID = BuildCANId (0,0,0,1,3,103,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(100000) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;
}

void Elternfenster (int updown,int lr)
{
  ULONG CanID ;
  unsigned char Data[8] ; 
  
  if (updown==1) {
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(200000) ;
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 30 ;
    Data [1] = 6;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 8;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 9;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 10;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 9;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 10;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 30 ;
    Data [1] = 1;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 3;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 30 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;

  if (updown==-1) {
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 31 ;
    Data [1] = 6;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 8;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 9;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 9;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(500000) ;
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 30 ;
    Data [1] = 1;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 3;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 30 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 30 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;

  if (updown==0) {
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 3;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 4;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 5;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(100000) ;
    CanID = BuildCANId (0,0,0,1,3,100,0) ;
    Data [0] = 31 ;
    Data [1] = 6;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 8;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 9;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    CanID = BuildCANId (0,0,0,1,3,101,0) ;
    Data [0] = 31 ;
    Data [1] = 9;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
    Data [0] = 31 ;
    Data [1] = 10;
    if ((lr==0)||(lr==1)) SendCANMessage (CanID, 2 ,Data) ;
    usleep(20000) ;
  } ;
}

/* Linie 1:
   Sensor 50: Wohnzimmer Licht
   Sensor 51: Kueche Licht
   Sensor 52: Esszimmer Rolladen
   Sensor 53: Wohnzimmer Terassentuer


   100 2: Licht Kueche (Steckdose)
   100 4: Licht Wohnzimmer
   100 5: Steckdose Fernseher Links 1
   100 7: Steckdose Kueche Tresen?
   100 9: Licht Wohnzimmer
   100 10: Steckdose Fenster Links 2

   101 1,6: Rolladen Kueche
   101 3: Steckdose Mikrowelle / Geschirrspueler
   101 5: Licht Kueche
   101 6,1: Rolladen Kueche
   101 8: Steckdose Mikrowelle 3-fach
   101 10: Licht Terasse

   102 1: Licht Tuer
   102 2: Licht Flur
   102 4: Steckdosen Kueche Fenster Links/Rechts
   102 6: Licht Garten 
   102 8: Licht Badezimmer
   102 9: Steckdosen Kueche Fenster Mitte
 
   103 1: Steckdose Fernseher Rechts  
   103 2: Steckdose EZ Ecke
   103 3,8: Esszimmer Rolladen
   103 4: Steckdose EZ Fluegeltuer
   103 7: Steckdose EZ Fenster Links
   103 8,3: Esszimmer Rolladen

Linie 3:
   Sensor 50: Niklas Zimmer
   Sensor 51: Beckies Zimmer
   Sensor 52: Badezimmer

   100,1: Eltern Rolladen1
   100,3: Eltern Rolladen2
   100,4: Eltern Rolladen3
   100,5: Eltern Rolladen4

   101, 1: Becky Rolladen 1
   101, 3: Niklas Rolladen 3
   101, 4: Eltern Rolladen5
   101, 5: Eltern Rolladen6


   102,1: Becky Licht Mitte
   102,2: Niklas Licht Mitte
   102,3 Kinderbad Licht Mitte
   102,6: Becky Licht Aussen
   102,7: Niklas Licht Aussen

   103, 1: Niklas Rolladen 1
   103, 3: Becky Rolladen 2
   103, 4: Niklas Rolladen 2
   103, 5: Becky Rolladen 3
   
   104,1:  Jalousie
   104,3: Jalousie
   104,4: Jalousie
   104,5: Jalousie

   105,5: Rolladen Kinderbad
   105,10: Rolladen Kinderbad
 */

int main (int argc, char* argv[])
{
  int i,j ;
  char *RetVal ;
  char *Hue ;
  char *Sat ;
  char *Val ;
  char *White ;
  unsigned char Data[8] ; 
  int R,G,B,W ;
  int H,S,V ;
  ULONG CanID ;
  char Var[256] ;
  FILE *Dat ;
  clock_t start,end ;

  usleep(1) ;
  InitNetwork () ;

 
  Dat = fopen("LED","r") ;
  if (Dat!=NULL) {
    fscanf (Dat,"%d %d %d %d\n",&H,&S,&V,&W) ;
    fclose (Dat) ;
  } ;

  RetVal = getenv("QUERY_STRING") ;
  if (RetVal==NULL) {
    printf ("Content-type: text/html\n\n") ;
    printf ("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n") ;
    printf ("<html><head><title>Umgebungsvariablen</title></head><body>\n");
    
    printf ("No Query String\n") ;
    printf ("</html>") ;  
    exit(0);
  } ;
  Hue = strstr(RetVal,"Hue.x=") ;
  if (Hue!=NULL) {
    sscanf (Hue,"Hue.x=%d",&H) ;
  } ;
  Sat = strstr(RetVal,"Sat.x=") ;
  if (Sat!=NULL) {
    sscanf (Sat,"Sat.x=%d",&S) ;
  } ;

  Val = strstr(RetVal,"Val.x=") ;
  if (Val!=NULL) {
    sscanf (Val,"Val.x=%d",&V) ;
  } ;

  Val = strstr(RetVal,"White.x=") ;
  if (Val!=NULL) {
    sscanf (Val,"White.x=%d",&W) ;
  } ;

  Val = strstr(RetVal,"An") ;
  if (Val!=NULL) {
    V = 255 ;
    S = 0 ;
    W = 255 ;
  } ;
  
  Val = strstr(RetVal,"Aus") ;
  if (Val!=NULL) {
    V = 0 ;
    W = 0 ;
  } ;

  Val = strstr(RetVal,"KH") ;
  if (Val!=NULL) {
    Rolladen (1,1,101,1) ;
  } ;

  Val = strstr(RetVal,"KR") ;
  if (Val!=NULL) {
    Rolladen (-1,1,101,1) ;
  } ;

  Val = strstr(RetVal,"KS") ;
  if (Val!=NULL) {
    Rolladen (0,1,101,1) ;
  } ;

  Val = strstr(RetVal,"WAH") ;
  if (Val!=NULL) {
    Jalousie (-1) ;
  } ;

  Val = strstr(RetVal,"WAR") ;
  if (Val!=NULL) {
    Jalousie (1) ;
  } ;

  Val = strstr(RetVal,"WAS") ;
  if (Val!=NULL) {
    Jalousie (0) ;
  } ;

  Val = strstr(RetVal,"WOH") ;
  if (Val!=NULL) {
    Rolladen (-1,3,104,4) ;
  } ;

  Val = strstr(RetVal,"WOR") ;
  if (Val!=NULL) {
    Rolladen (1,3,104,4) ;
  } ;

  Val = strstr(RetVal,"WOS") ;
  if (Val!=NULL) {
    Rolladen (0,3,104,4) ;
  } ;

  Val = strstr(RetVal,"WLH") ;
  if (Val!=NULL) {
    Rolladen (-1,3,104,1) ;
  } ;

  Val = strstr(RetVal,"WLR") ;
  if (Val!=NULL) {
    Rolladen (1,3,104,1) ;
  } ;

  Val = strstr(RetVal,"WLS") ;
  if (Val!=NULL) {
    Rolladen (0,3,104,1) ;
  } ;

  Val = strstr(RetVal,"WMH") ;
  if (Val!=NULL) {
    Rolladen (-1,3,104,3) ;
  } ;

  Val = strstr(RetVal,"WMR") ;
  if (Val!=NULL) {
    Rolladen (1,3,104,3) ;
  } ;

  Val = strstr(RetVal,"WMS") ;
  if (Val!=NULL) {
    Rolladen (0,3,104,3) ;
  } ;

  Val = strstr(RetVal,"WRH") ;
  if (Val!=NULL) {
    Rolladen (-1,3,104,5) ;
  } ;

  Val = strstr(RetVal,"WRR") ;
  if (Val!=NULL) {
    Rolladen (1,3,104,5) ;
  } ;

  Val = strstr(RetVal,"WRS") ;
  if (Val!=NULL) {
    Rolladen (0,3,104,5) ;
  } ;

  Val = strstr(RetVal,"WAO") ;
  if (Val!=NULL) {
    Jalousie(-1);
    usleep(500000);
    Jalousie(0);
  } ;


  Val = strstr(RetVal,"EH") ;
  if (Val!=NULL) {
    Rolladen (1,1,103,3) ;
  } ;

  Val = strstr(RetVal,"ER") ;
  if (Val!=NULL) {
    Rolladen (-1,1,103,3) ;
  } ;

  Val = strstr(RetVal,"ES") ;
  if (Val!=NULL) {
    Rolladen (0,1,103,3) ;
  } ;

  Val = strstr(RetVal,"WLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 30 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"WLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 31 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 30 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 31 ;
    Data [1] = 8;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"FLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 30 ;
    Data [1] = 2;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"FLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 31 ;
    Data [1] = 2;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SWZFRAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,103,0) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SWZFRAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,103,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SWZFLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 30 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 30 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SWZFLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 31 ;
    Data [1] = 10;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SEZFLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,103,0) ;
    Data [0] = 30 ;
    Data [1] = 7;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SEZFLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,103,0) ;
    Data [0] = 31 ;
    Data [1] = 7;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SEZFTAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,103,0) ;
    Data [0] = 30 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"SEZFTAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,103,0) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KFSMAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 30 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KFSMAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 31 ;
    Data [1] = 9;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KFSSAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 30 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KFSSAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 31 ;
    Data [1] = 4;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"GLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 30 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"GLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,102,0) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KLAn") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,101,0) ;
    Data [0] = 30 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 30 ;
    Data [1] = 2;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KLAus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,1,101,0) ;
    Data [0] = 31 ;
    Data [1] = 5;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
    CanID = BuildCANId (0,0,0,1,1,100,0) ;
    Data [0] = 31 ;
    Data [1] = 2;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KBAL1An") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 30 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"KBAL1Aus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 31 ;
    Data [1] = 3;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BL1An") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 30 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BL1Aus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 31 ;
    Data [1] = 6;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BL2An") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 30 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BL2Aus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 31 ;
    Data [1] = 1;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"NL1An") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 30 ;
    Data [1] = 7;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"NL1Aus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 31 ;
    Data [1] = 7;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"NL2An") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 30 ;
    Data [1] = 2;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"NL2Aus") ;
  if (Val!=NULL) {
    CanID = BuildCANId (0,0,0,1,3,102,0) ;
    Data [0] = 31 ;
    Data [1] = 2;
    SendCANMessage (CanID, 2 ,Data) ;
    usleep(50000) ;
  } ;

  Val = strstr(RetVal,"BRH") ;
  if (Val!=NULL) {
    Beckiefenster (-1) ;
  } ;

  Val = strstr(RetVal,"BRR") ;
  if (Val!=NULL) {
    Beckiefenster (1) ;
  } ;

  Val = strstr(RetVal,"BRS") ;
  if (Val!=NULL) {
    Beckiefenster (0) ;
  } ;

  Val = strstr(RetVal,"NRH") ;
  if (Val!=NULL) {
    Niklasfenster (-1) ;
  } ;

  Val = strstr(RetVal,"NRR") ;
  if (Val!=NULL) {
    Niklasfenster (1) ;
  } ;

  Val = strstr(RetVal,"NRS") ;
  if (Val!=NULL) {
    Niklasfenster (0) ;
  } ;

  Val = strstr(RetVal,"DAH") ;
  if (Val!=NULL) {
    Elternfenster (-1,0) ;
  } ;

  Val = strstr(RetVal,"DAR") ;
  if (Val!=NULL) {
    Elternfenster (1,0) ;
  } ;

  Val = strstr(RetVal,"DAS") ;
  if (Val!=NULL) {
    Elternfenster (0,0) ;
  } ;

  Val = strstr(RetVal,"DGGH") ;
  if (Val!=NULL) {
    Elternfenster (-1,2) ;
  } ;

  Val = strstr(RetVal,"DGGR") ;
  if (Val!=NULL) {
    Elternfenster (1,2) ;
  } ;

  Val = strstr(RetVal,"DGGS") ;
  if (Val!=NULL) {
    Elternfenster (0,2) ;
  } ;

  Val = strstr(RetVal,"DGVH") ;
  if (Val!=NULL) {
    Elternfenster (-1,1) ;
  } ;

  Val = strstr(RetVal,"DGVR") ;
  if (Val!=NULL) {
    Elternfenster (1,1) ;
  } ;

  Val = strstr(RetVal,"DGVS") ;
  if (Val!=NULL) {
    Elternfenster (0,1) ;
  } ;

  Val = strstr(RetVal,"DG1H") ;
  if (Val!=NULL) {
    Rolladen (1,3,100,1) ;
  } ;

  Val = strstr(RetVal,"DG1R") ;
  if (Val!=NULL) {
    Rolladen (-1,3,100,1) ;
  } ;

  Val = strstr(RetVal,"DG1S") ;
  if (Val!=NULL) {
    Rolladen (0,3,100,1) ;
  } ;

  Val = strstr(RetVal,"DG2H") ;
  if (Val!=NULL) {
    Rolladen (1,3,100,3) ;
  } ;

  Val = strstr(RetVal,"DG2R") ;
  if (Val!=NULL) {
    Rolladen (-1,3,100,3) ;
  } ;

  Val = strstr(RetVal,"DG2S") ;
  if (Val!=NULL) {
    Rolladen (0,3,100,3) ;
  } ;

  Val = strstr(RetVal,"DG3H") ;
  if (Val!=NULL) {
    Rolladen (1,3,100,4) ;
  } ;

  Val = strstr(RetVal,"DG3R") ;
  if (Val!=NULL) {
    Rolladen (-1,3,100,4) ;
  } ;

  Val = strstr(RetVal,"DG3S") ;
  if (Val!=NULL) {
    Rolladen (0,3,100,4) ;
  } ;

  Val = strstr(RetVal,"DG4H") ;
  if (Val!=NULL) {
    Rolladen (1,3,100,5) ;
  } ;

  Val = strstr(RetVal,"DG4R") ;
  if (Val!=NULL) {
    Rolladen (-1,3,100,5) ;
  } ;

  Val = strstr(RetVal,"DG4S") ;
  if (Val!=NULL) {
    Rolladen (0,3,100,5) ;
  } ;

  Val = strstr(RetVal,"DG5H") ;
  if (Val!=NULL) {
    Rolladen (1,3,101,4) ;
  } ;

  Val = strstr(RetVal,"DG5R") ;
  if (Val!=NULL) {
    Rolladen (-1,3,101,4) ;
  } ;

  Val = strstr(RetVal,"DG5S") ;
  if (Val!=NULL) {
    Rolladen (0,3,101,4) ;
  } ;

  Val = strstr(RetVal,"DG6H") ;
  if (Val!=NULL) {
    Rolladen (1,3,101,5) ;
  } ;

  Val = strstr(RetVal,"DG6R") ;
  if (Val!=NULL) {
    Rolladen (-1,3,101,5) ;
  } ;

  Val = strstr(RetVal,"DG6S") ;
  if (Val!=NULL) {
    Rolladen (0,3,101,5) ;
  } ;

  Val = strstr(RetVal,"KBH") ;
  if (Val!=NULL) {
    Rolladen (1,3,105,5) ;
  } ;

  Val = strstr(RetVal,"KBR") ;
  if (Val!=NULL) {
    Rolladen (-1,3,105,5) ;
  } ;

  Val = strstr(RetVal,"KBS") ;
  if (Val!=NULL) {
    Rolladen (0,3,105,5) ;
  } ;

  Dat = fopen("LED","w") ;
  fprintf (Dat,"%d %d %d %d\n",H,S,V,W) ;
  fclose (Dat) ;
  

  /*  Data[0] = 10 ;
      Data[1] = (unsigned char)H ;
      Data[2] = (unsigned char)S ;
      Data[3] = (unsigned char)V ;
      Data[4] = (unsigned char)W ;
      
      Val = strstr(RetVal,"Disko") ;
      if (Val!=NULL) {
      for (i=0;i<20;i++) {
      Data[0] = 19 ;
      Data[1] = i ;
      CanID = BuildCANId (0,0,0,1,1,23,0) ;
      SendCANMessage (CanID, 2 ,Data) ; 
      usleep(100000) ;
      } ;
      } else if (strstr(RetVal,"DAus")) {
      Data[0] = 20 ;
      Data[1] = 22 ;
      CanID = BuildCANId (0,0,0,1,1,23,0) ;
      SendCANMessage (CanID, 2 ,Data) ; 
      } else if (strstr(RetVal,"LED=7")!=NULL) {
      CanID = BuildCANId (0,0,0,1,1,23,0) ;
      SendCANMessage (CanID, 5 ,Data) ;
      } else {
      for (i=0;i<7;i++) {
      sprintf (Var,"LED=%d",i) ;
      if (strstr(RetVal,Var)!=NULL) {
      CanID = BuildCANId (0,0,0,1,1,16+i,0) ;
      SendCANMessage (CanID, 5 ,Data) ;
      usleep(1000) ;
      } ;
      } ;
      } ;
  */

  printf ("Content-type: text/html\n\n") ;
  printf ("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n") ;
  printf ("<html><head><title>Umgebungsvariablen</title></head><body>\n");

  //  printf ("%s\n",RetVal) ;
  printf ("<script type=\"text/javascript\">\n") ;
  printf ("history.back();\n") ;
  printf ("</script>") ;
  printf ("</html>") ;
  CloseNetwork () ;
}

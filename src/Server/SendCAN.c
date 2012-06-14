/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
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

int main (int argc, char *argv[])
{
  int i ;
  ULONG CanID ;

  int Line ;
  int Add ;
  int Command ;
  char Data[8] ;

  if ((argc<4)||(argc>11)) {
    printf ("Usage: SendCAN Line Address Command [Data1]..[Data7]\n") ;
    printf ("Command:\n") ;
    printf ("SEND_STATUS        = 1\n") ;
    printf ("READ_CONFIG	= 2\n") ;
    printf ("WRITE_CONFIG	= 3\n") ;
    printf ("SET_VAR		= 4\n") ;
    printf ("START_BOOT		= 5\n") ;
    printf ("TIME		= 6\n") ;
    printf ("LED_OFF		= 7\n") ;
    printf ("LED_ON		= 8\n") ;
    printf ("SET_TO		= 9\n") ;
    printf ("HSET_TO		= 10\n") ;
    printf ("L_AND_S		= 11\n") ;
    printf ("SET_TO_G1		= 12\n") ;
    printf ("SET_TO_G2		= 13\n") ;
    printf ("SET_TO_G3		= 14\n") ;
    printf ("LOAD_LOW		= 15\n") ;
    printf ("LOAD_MID		= 16\n") ;
    printf ("LOAD_HIGH		= 17\n") ;
    printf ("START_PROG		= 18\n") ;
    exit(-1) ;
  } ;

  InitNetwork () ;

  sscanf (argv[1],"%d",&Line) ;
  sscanf (argv[2],"%d",&Add) ;
  sscanf (argv[3],"%d",&Command) ;
  Data [0] = (char) Command ;
  for (i=0;i<(argc-4);i++) {
    sscanf(argv[i+4],"%d",&Command) ;
    Data[i+1] = Command ;
  } ;

  CanID = BuildCANId (0,0,0,1,Line,Add,0) ;

  SendCANMessage (CanID, argc-3,Data) ;

  CloseNetwork () ;
}

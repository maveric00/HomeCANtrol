/*
** CANGateway.c -- Gateway functionality between UDP and CAN for Linux CAN (socketcan)
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
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <netdb.h>
#include <ctype.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define CANBUFLEN 20
#define MAXLINE 520

char CAN_PORT[MAXLINE] ;
int CAN_PORT_NUM ;
char CAN_BROADCAST[MAXLINE] ;

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;

int RecSockFD;
int SendSockFD;
int Can0SockFD;
int Can1SockFD;

struct addrinfo *servinfo, *SendInfo ;

/* Einzelne Hex-Zahlen einlesen */

inline ULONG BuildCANId (char Prio, char Repeat, char FromLine, USHORT FromAdd, char ToLine, USHORT ToAdd, char Group)
{
  return ((Group&0x1)<<1|ToAdd<<2|(ToLine&0xf)<<10|FromAdd<<14|(FromLine&0xf)<<22|(Repeat&0x1)<<26|(Prio&0x3)<<27) ;
}

inline char MatchAddress (ULONG CANId, char ToLine, USHORT ToAdd)
{
  ToLine = ToLine&0xf ;
  if (((CANId & (0xf<<10))==(ToLine<<10))&&(((CANId & (0xfff<<2))==(ToAdd<<2)))) {
    return(1) ;
  } else {
    return(0) ;
  } ;
}

void GetSourceAddress (ULONG CANId, char *FromLine, USHORT *FromAdd)
{
  *FromLine = (char)((CANId>>22)&0xf) ;
  *FromAdd = (USHORT) ((CANId>>14)&0xff) ;
}

int InitNetwork(void)
{

	struct addrinfo hints;
	int rv;
	struct sockaddr_in RecAddr;
	int val ;
	struct sockaddr_can CANAddr ;
	struct ifreq ifr ;

	val = (0==0) ;
	// Reserve Receive-Socket
	if ((RecSockFD = socket(AF_INET, SOCK_DGRAM,0)) == -1) {
	  perror("CANGateway: Receive socket");
	  exit(0);
	}

	memset(&RecAddr, 0, sizeof(struct sockaddr_in));
	RecAddr.sin_family      = AF_INET;
	RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
	RecAddr.sin_port        = htons(CAN_PORT_NUM);

	if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
	  close(RecSockFD);
	  perror("listener: bind");
	}

	setsockopt(RecSockFD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;

	// Set up send-Socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(CAN_BROADCAST, CAN_PORT, &hints, &servinfo)) != 0) {
	  perror("CANGateway: Get Broadcast info") ;
	  return 1;
	}

	// loop through all the results and make a socket
	for(SendInfo = servinfo; SendInfo != NULL; SendInfo = SendInfo->ai_next) {
		if ((SendSockFD = socket(SendInfo->ai_family, SendInfo->ai_socktype,
				SendInfo->ai_protocol)) == -1) {
			perror("CANGateway: Send socket");
			continue;
		}

		break;
	}

	if (SendInfo == NULL) {
		fprintf(stderr, "CANGateway: failed to get Send-socket\n");
		return 2;
	}

	setsockopt(SendSockFD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;

	Can0SockFD = socket (PF_CAN,SOCK_RAW,CAN_RAW) ;
	if (Can0SockFD<0) {
	  perror ("CANGateway: Could not get CAN socket") ;
	  return 3 ;
	} ;

	CANAddr.can_family = AF_CAN ;
	memset (&ifr.ifr_name,0,sizeof(ifr.ifr_name)) ;
	strcpy (ifr.ifr_name,"can0") ;
	
	if (ioctl(Can0SockFD,SIOCGIFINDEX,&ifr)<0) {
	  perror ("CANGateway: Could not resolve can0") ;
	  return (3) ;
	} ;
	
	CANAddr.can_ifindex = ifr.ifr_ifindex ;

	if (bind(Can0SockFD,(struct sockaddr*)&CANAddr,sizeof(CANAddr))<0) {
	  perror ("CANGateway: Could not bind CAN0 socket") ;
	  return (3) ;
	} ;

	Can1SockFD = socket (PF_CAN,SOCK_RAW,CAN_RAW) ;
	if (Can1SockFD<0) {
	  perror ("CANGateway: Could not get CAN socket") ;
	  return 3 ;
	} ;

	CANAddr.can_family = AF_CAN ;
	memset (&ifr.ifr_name,0,sizeof(ifr.ifr_name)) ;
	strcpy (ifr.ifr_name,"can1") ;
	
	if (ioctl(Can1SockFD,SIOCGIFINDEX,&ifr)<0) {
	  perror ("CANGateway: Could not resolve can1") ;
	  return (3) ;
	} ;
	
	CANAddr.can_ifindex = ifr.ifr_ifindex ;

	if (bind(Can1SockFD,(struct sockaddr*)&CANAddr,sizeof(CANAddr))<0) {
	  perror ("CANGateway: Could not bind CAN1 socket") ;
	  return (3) ;
	} ;
	return(0);
} ;



int ReceiveFromUDP (ULONG *CANID, unsigned char *Len, unsigned char *Data)
{
  int i ;
  char *CANIDP ;
  char buf[CANBUFLEN];
  size_t addr_len;
  int numbytes ;
  struct sockaddr_storage their_addr;
  
  addr_len = sizeof their_addr;
  if ((numbytes = recvfrom(RecSockFD, buf, CANBUFLEN-1 , 0,
			   (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    perror("recvfrom");
    exit(1);
  }

  CANIDP = (char*)CANID ;
  for (i=0;i<4;i++) CANIDP[i]=buf[i] ;
  *Len = buf[4];

  for (i=0;i<*Len;i++) Data[i] = buf[i+5] ;
  for (;i<8;i++) buf[i+5] = 0 ;
    
  return 0;
}

int ReceiveFromCAN (int socket, ULONG *CANID, unsigned char *Len, unsigned char *Data)
{
  int i ;
  struct can_frame frame ;
  int numbytes ;

  if ((numbytes = read(socket, &frame, sizeof(struct can_frame))) < 0) {
    perror("CANGateway: CAN raw socket read");
    exit(1);
  } ;

  *CANID = frame.can_id & CAN_EFF_MASK ;

  for (i=0;i<frame.can_dlc;i++) Data[i]=frame.data[i] ;
  *Len = frame.can_dlc;
    
  return 0;
}

int SendToCAN (int socket, ULONG CANID, char Len, unsigned char *Data)
{
  int i ;
  struct can_frame frame ;
  int numbytes ;

  frame.can_id = CANID|CAN_EFF_FLAG ;
  frame.can_dlc = Len ;
  for (i=0;i<Len;i++) frame.data[i] = Data[i] ;
  
  if ((numbytes = write(socket, &frame, sizeof(struct can_frame))) < 0) {
    perror("CANGateway: CAN raw socket read");
    exit(1);
  } ;
  
  return 0;
}


int SendToUDP (ULONG CANID, char Len, unsigned char *Data)
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

  close(RecSockFD);
  close(SendSockFD);
  close(Can0SockFD) ;
  close(Can1SockFD) ;
}


int main (int argc, char*argv[])
{
  int i,j ;
  ULONG TestID ;
  USHORT FromAdd ;
  char FromLine ;
  unsigned char Len ;
  unsigned char Data[8] ;
  fd_set rdfs ;

  if (argc!=2) {
    printf ("Usage: %s IP:Port     , e.g.\n",argv[0]) ;
    printf ("       %s 192.168.69.255:13247\n",argv[0]); 
    exit(1) ;
  } ;

  for (i=0;i<MAXLINE;i++) {
    CAN_BROADCAST[i]=argv[1][i] ;
    if (argv[1][i]==':') break ;
  }
  CAN_BROADCAST[i]='\0' ;
  i++;

  for (j=0;i<MAXLINE;i++,j++) {
    CAN_PORT[j]=argv[1][i] ;
    if (argv[1][i]=='\0') break ;
  }

  sscanf (CAN_PORT,"%d",&CAN_PORT_NUM) ;

  printf ("Initializing %s at port %d\n",CAN_BROADCAST,CAN_PORT_NUM) ;
  
  InitNetwork () ;
 
  for (;;) {
    FD_ZERO(&rdfs) ;
    FD_SET(RecSockFD,&rdfs) ;
    FD_SET(Can0SockFD,&rdfs) ;
    FD_SET(Can1SockFD,&rdfs) ;

    if ((i=select(Can1SockFD+1,&rdfs,NULL,NULL,NULL))<0) {
      perror ("CANGateway: Could not select") ;
      exit (1) ;
    } ;
    
    if (FD_ISSET(RecSockFD,&rdfs)) {
      ReceiveFromUDP (&TestID,&Len,Data) ;
      SendToCAN (Can0SockFD,TestID,Len,Data) ;
      SendToCAN (Can1SockFD,TestID,Len,Data) ;
      GetSourceAddress(TestID,&FromLine,&FromAdd) ;
      //      printf ("Net Rec: 0x%lx ; Line:%d, Add:%d, Len:%d, Data: 0x%x 0x%x 0x%x 0x%x\n",TestID,FromLine,FromAdd,Len,Data[0],Data[1],Data[2],Data[3]) ;
    } ;

    if (FD_ISSET(Can0SockFD,&rdfs)) {
      ReceiveFromCAN (Can0SockFD,&TestID,&Len,Data) ;
      SendToUDP(TestID,Len,Data) ;
      SendToCAN(Can1SockFD,TestID,Len,Data) ;
      GetSourceAddress(TestID,&FromLine,&FromAdd) ;
      //      printf ("CAN0 Rec: 0x%lx ; Line:%d, Add:%d, Len:%d, Data: 0x%x 0x%x 0x%x 0x%x\n",TestID,FromLine,FromAdd,Len,Data[0],Data[1],Data[2],Data[3]) ;

    } ;

    if (FD_ISSET(Can1SockFD,&rdfs)) {
      ReceiveFromCAN (Can1SockFD,&TestID,&Len,Data) ;
      SendToUDP(TestID,Len,Data) ;
      SendToCAN(Can0SockFD,TestID,Len,Data) ;
      GetSourceAddress(TestID,&FromLine,&FromAdd) ;
      //      printf ("CAN1 Rec: 0x%lx ; Line:%d, Add:%d, Len:%d, Data: 0x%x 0x%x 0x%x 0x%x\n",TestID,FromLine,FromAdd,Len,Data[0],Data[1],Data[2],Data[3]) ;
    } ;    
  } ;
  
  CloseNetwork () ;
}

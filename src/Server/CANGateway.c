/*
** CANGateway.c -- Gateway (and simple filter) functionality between UDP and CAN for Linux CAN (socketcan)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
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
#include "RelUDP.h"
#include "artnet.h"
#include "packets.h"
#define _XOPEN_SOURCE
#include <time.h>
#include <sys/time.h>


#define TRUE (1==1)
#define FALSE (1==0)

struct timeval Now ;
artnet_node node1, node2;

char *CommandName[]={
  "Update request",
  "Identify",
  "Set Address",
  "Firmware data",
  "Start application",
  "Undefined (6)",
  "Undefined (7)",
  "Undefined (8)",
  "Undefined (9)",
  "Send Status",
  "Read Config",
  "WriteConfig",
  "Read Variable",
  "Set Variable",
  "Start Bootloader",
  "Time",
  "Undefined (17)",
  "Undefined (18)",
  "Undefined (19)",
  "Channel on", //20
  "Channel off", //21
  "Channel toggle", //22
  "Shade up (full)", //23
  "Shade down (full)", //24
  "Shade up (short)", //25
  "Shade down (short)",//26
  "Send LEDPort", //27
  "Undefined (28)",
  "Undefined (29)",
  "Set to", //30
  "HSet to", //31
  "Load and store", //32
  "Set to G1", //33
  "Set to G2", //34
  "Set To G3", //35
  "Load program", //36
  "Start program", //37
  "Stop program", //38
  "Dim to", //39
  "HDim to", //40
  "Load two LEDs", //41
  "Undefined (42)", //42
  "Undefined (43)", 
  "Undefined (44)",
  "Undefined (45)",
  "Undefined (46)",
  "Undefined (47)",
  "Undefined (48)",
  "Undefined (49)",
  "Set Pin",
  "Load LED",
  "Out LED",
  "Start Sensor",
  "Stop Sensor",
  "Analog Value",
  "Light Value"
} ;

char CommandNum[40]; 
char TimeString[40] ;

char *LogTime(void) 
{
  struct tm *LTime ;

  LTime = localtime(&(Now.tv_sec)) ;
  sprintf (TimeString,"%02d.%02d, %02d:%02d:%02d.%03d",LTime->tm_mday,LTime->tm_mon,LTime->tm_hour,LTime->tm_min,LTime->tm_sec,(int)(Now.tv_usec/1000)) ;
  return (TimeString) ;
}



char *ToCommand(int Command)
{
  int Type ;
  Type = 0 ;
  if ((Command&0x40)!=0) {
    Command = Command & ~0x40 ;
    Type = 1 ;
  }
  if ((Command&0x80)!=0) {
    Command = Command & ~0x80 ;
    Type +=2 ;
  } ;
  if ((Command<1)||(Command>56)) {
    sprintf (CommandNum,"Out of Range: %d (0x%02x)",Command,Command) ;
    return (CommandNum) ;
  } ;
  if (Type==0) {
    return (CommandName[Command-1]) ;
  } else if (Type==1) {
    sprintf (CommandNum,"Success: %s",CommandName[Command-1]) ;
    return (CommandNum) ;
  } else if (Type==2) {
    sprintf (CommandNum,"Error: %s",CommandName[Command-1]) ;
    return (CommandNum) ;
  } else {
    sprintf (CommandNum,"WrongNum: %s",CommandName[Command-1]) ;
    return (CommandNum) ;
  }
}

#define CANBUFLEN 20
#define MAXLINE 520

#define CIF_CAN0 1
#define CIF_CAN1 2
#define CIF_NET 3

char CAN_PORT[MAXLINE] ;
int CAN_PORT_NUM ;
char CAN_BROADCAST[MAXLINE] ;
int GatewayNum ;
int Verbose=0 ;
int NoTime=0 ;
int NoRoute=0 ;

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;

int RecSockFD;
int SendSockFD;
int Can0SockFD;
int Can1SockFD;
int artnetFD;
FILE *logfd ;

// Struct for a single CAN-Message, including masking, receiving interface and pointer to modification methods

struct CANCommand {
  struct CANCommand *Next ;
  char Interface ;
  char Prio ;
  char PrioMask ;
  char Repeat ;
  char RepeatMask ;
  char Group ;
  char GroupMask ;
  char FromLine ;
  char FromLineMask ;
  USHORT FromAdd ;
  USHORT FromAddMask ;
  char ToLine ;
  char ToLineMask ;
  USHORT ToAdd ;
  USHORT ToAddMask ;
  char Len ;
  char LenMask ;
  unsigned char Data[8] ;
  unsigned char DataMask[8] ;
  struct CANCommand *Exchange ;
} ;

struct CANToDMX {
  char Line ;
  USHORT Add ;
  char Start ;
  USHORT Map[6] ;
} ;

struct CANToDMX CANBuffer[4][ARTNET_DMX_LENGTH] ;

struct CANCommand *FilterList ;

char RouteIF0[255] ;
char RouteIF1[255] ;

// Read a CANCommand (from, to, length and data) including mask from configuration file

void ReadDMXTable (FILE *Conf, int Univ)
{
  char Line[255] ;
  char *Str ;
  int i,j ;
  int L1,A1,S1,S2 ;
 
  //  for (i=0;i<ARTNET_DMX_LENGTH/2;i++) CANBuffer[Univ][i].High=CANBuffer[Univ][i].Low=ARTNET_DMX_LENGTH ;

  i = 0 ;

  while (TRUE) {
    while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; //Read next line 
    if (Str==NULL) return ;
    if (strstr(Line,"End")) return ;
    if (i<ARTNET_DMX_LENGTH) { // Max 512 per Universe
      sscanf (Line,"%d: %d %d %d",&j,&L1,&A1,&S2) ;
      S1 = S2-S2%6 ;
      for (j=0;j<ARTNET_DMX_LENGTH;j++) 
	if ((CANBuffer[Univ][j].Line==L1)&&(CANBuffer[Univ][j].Add==A1)&&(CANBuffer[Univ][j].Start==S1)) break ;
      if (j==ARTNET_DMX_LENGTH) {
	// Not yet set, find next free
	for (j=0;j<ARTNET_DMX_LENGTH;j++) if (CANBuffer[Univ][j].Line==0) break ;
      } ;
      if (j<ARTNET_DMX_LENGTH) {
	CANBuffer[Univ][j].Line = L1 ;
	CANBuffer[Univ][j].Add = A1 ;
	CANBuffer[Univ][j].Start = S1 ;
	CANBuffer[Univ][j].Map[S2%6] = i ;
      } ;
      i++ ;
    } ;
  } ;
}
      


void ReadCommandBlock(FILE *Conf,struct CANCommand *Command)
{
  char Line[255] ;
  char *Str ;
  int i ;
  
  // Clear fields not defined in configuration
  Command->Next = NULL ;
  Command->Interface = 0 ;
  Command->Exchange = NULL ;
  Command->FromLineMask = Command->FromAddMask = Command->ToLineMask = 
    Command->ToAddMask = Command->LenMask = Command->PrioMask = Command->RepeatMask = Command->GroupMask = 0 ;
  for (i=0;i<8;i++) Command->DataMask[i] = 0 ;
  
  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"From: %hhu %hhx %hu %hx\n",&(Command->FromLine),&(Command->FromLineMask),
	 &(Command->FromAdd),&(Command->FromAddMask)) ;

  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"To: %hhu %hhx %hu %hx\n",&(Command->ToLine),&(Command->ToLineMask),
	 &(Command->ToAdd),&(Command->ToAddMask)) ;

  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"Prio: %hhd %hhx\n",&(Command->Prio),&(Command->PrioMask)) ;

  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"Repeat: %hhd %hhx\n",&(Command->Repeat),&(Command->RepeatMask)) ;

  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"Group: %hhd %hhx\n",&(Command->Group),&(Command->GroupMask)) ;


  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"Len: %hhd %hhx\n",&(Command->Len),&(Command->LenMask)) ;

  while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
  if (Str==NULL) return ;
  sscanf(Line,"Data: %hhu %hhx %hhu %hhx %hhu %hhx %hhu %hhx %hhu %hhx %hhu %hhx %hhu %hhx %hhu %hhx\n",
	 &(Command->Data[0]),&(Command->DataMask[0]),
	 &(Command->Data[1]),&(Command->DataMask[1]),
	 &(Command->Data[2]),&(Command->DataMask[2]),
	 &(Command->Data[3]),&(Command->DataMask[3]),
	 &(Command->Data[4]),&(Command->DataMask[4]),
	 &(Command->Data[5]),&(Command->DataMask[5]),
	 &(Command->Data[6]),&(Command->DataMask[6]),
	 &(Command->Data[7]),&(Command->DataMask[7])) ;
}


// Read filter definitions from configuration file

void ReadFilter(FILE *Conf)
{
  struct CANCommand *Filter ;
  struct CANCommand *Exchange ;
  char *Str ;
  char Line[255] ;

  //Allocate new filter

  if (FilterList==NULL) {
    FilterList = malloc(sizeof(struct CANCommand)) ;
    if (FilterList==NULL) {
      fprintf (stderr,"Out of mem\n") ;
      exit(-1) ;
    } ;
    Filter = FilterList ;
  } else {
    for (Filter=FilterList;Filter->Next==NULL;Filter=Filter->Next) ;
    Filter->Next = malloc(sizeof(struct CANCommand)) ;
    if (Filter->Next==NULL) {
      fprintf (stderr,"Out of mem\n") ;
      exit(-1) ;
    } ;
    Filter = Filter->Next ;
  } ;

  // Read infromations of to be filtered message
  
  ReadCommandBlock (Conf,Filter) ;
  

  // Read all modification rules from config file
  for (;;) {
    while ((Str=fgets(Line,sizeof(Line),Conf))!=NULL) if (Line[0]!='#') break; 
    // If end of file (or not "With"), return to caller
    if (Str==NULL) return ;
    if (strstr(Line,"With")==NULL) return ; // No additional with
    
    // Allocate new modification rule
    
    if (Filter->Exchange==NULL) {
      Filter->Exchange = malloc(sizeof(struct CANCommand)) ;
      if (Filter->Exchange==NULL) {
	fprintf (stderr,"Out of mem\n") ;
	exit(-1) ;
      } ;
      Exchange = Filter->Exchange ;
    } else {
      Exchange->Next = malloc(sizeof(struct CANCommand)) ;
      if (Exchange->Next==NULL) {
	fprintf (stderr,"Out of mem\n") ;
	exit(-1) ;
      } ;
      Exchange = Filter->Next ;
    } ;
    
    // Read in modification rule

    ReadCommandBlock (Conf,Exchange) ;
  } ;
}

// Read configuration file

void ReadConfig(void)
{
  int i,j ;
  FILE *Conf ;
  char Line[255] ;

  // Set default values in case that no config file is available

  strcpy(CAN_BROADCAST,"255.255.255.255") ;
  strcpy(CAN_PORT,"13247") ;
  CAN_PORT_NUM=13247 ;

  // Rooute everything to everywhere

  for (i=0;i<255;i++) {
    RouteIF0[i] = 1 ;
    RouteIF1[i] = 1 ;
  } ;

  // Open config file

  Conf = fopen("CANGateway.conf","r") ;

  if (Conf==NULL) {
    fprintf (stderr,"Using default values, routing all to all\n") ;
    return ;
  } ;

  // If configuration file is available, routing information should be included.
  
  for (i=0;i<255;i++) {
    RouteIF0[i] = 0 ;
    RouteIF1[i] = 0 ;
  } ;

  // read config file line for line and check
  while (fgets(Line,sizeof(Line),Conf)!=NULL) {
    if (strstr(Line,"Broadcast:")!=NULL) {
      sscanf(Line,"Broadcast: %s",CAN_BROADCAST) ;
    } ;
    if (strstr(Line,"Port:")!=NULL) {
      sscanf(Line,"Port: %s",CAN_PORT) ;
      sscanf (CAN_PORT,"%d",&CAN_PORT_NUM) ;
    } ;
    if (strstr(Line,"GatewayNumber:")!=NULL) {
      sscanf(Line,"GatewayNumber: %d",&GatewayNum) ;
    } ;
    
    // Read routing information; Format: "CAN0-Line: Line1 Line2 Line3 ..."
    // Messages targeted to Line1,... are sent out to CAN0

    if (strstr(Line,"CAN0-Line:")!=NULL) {
      for (i=0;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      i++ ;
      for (;Line[i]!='\0';i++) {
	sscanf (&(Line[i]),"%d",&j) ;
	if ((j>0)&&(j<255)) RouteIF0[j]=1 ;
	for (;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
	if (Line[i]=='\0') break ;
      } ;
    } ;

    // Same for CAN1

    if (strstr(Line,"CAN1-Line:")!=NULL) {
      for (i=0;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      i++ ;
      for (;Line[i]!='\0';i++) {
	sscanf (&(Line[i]),"%d",&j) ;
	if ((j>0)&&(j<255)) RouteIF1[j]=1 ;
	for (;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
	if (Line[i]=='\0') break ;
      } ;
    } ;

    // If filter information available, set up Filter and possibly modification rules

    if (strstr(Line,"Exchange")) {
      ReadFilter(Conf) ;
    } ;
    if (strstr(Line,"DMX Table")) {
      for (i=0;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      i++ ;
      sscanf (&(Line[i]),"%d",&j) ;
      if ((j>=0)&&(j<4)) ReadDMXTable(Conf,j) ;
    } ;
  } ;

  fclose (Conf) ;  

  // Set to default if no routing was configured
  for (i=0;(i<255)&&(RouteIF0[i]==0);i++) ;
  if (i==255) for(i=0;i<255;i++) RouteIF0[i] = 1 ;

  for (i=0;(i<255)&&(RouteIF1[i]==0);i++) ;
  if (i==255) for(i=0;i<255;i++) RouteIF1[i] = 1 ;

  if (GatewayNum==0) {
    fprintf (stderr,"Please specify GatewayNumber in Config\n") ;
    exit(0) ;
  }; 
}


struct addrinfo *servinfo, *SendInfo ;

// Build the 32bit CANID from address information

ULONG BuildCANId (char Prio, char Repeat, char FromLine, USHORT FromAdd, char ToLine, USHORT ToAdd, char Group)
{
  return ((Group&0x1)<<1|ToAdd<<2|(ToLine&0xf)<<10|FromAdd<<14|(FromLine&0xf)<<22|(Repeat&0x1)<<26|(Prio&0x3)<<27) ;
}

void GetExtendedAddress (ULONG CANId, char *Prio, char *Repeat, char *Group)
{
  *Prio = (char)((CANId>>27)&0x3) ;
  *Repeat = (char)((CANId>>26)&0x1) ;
  *Group = (char)((CANId>>1)&0x1) ;
}

// Extract source address from CANID

void GetSourceAddress (ULONG CANId, char *FromLine, USHORT *FromAdd)
{
  *FromLine = (char)((CANId>>22)&0xf) ;
  *FromAdd = (USHORT) ((CANId>>14)&0xff) ;
}

// Extract destination address form CANID

void GetDestAddress (ULONG CANId, char *ToLine, USHORT *ToAdd)
{
  *ToLine = (char)((CANId>>10)&0xf) ;
  *ToAdd = (USHORT) ((CANId>>2)&0xff) ;
}

int InitCAN(void)
{
  struct sockaddr_can CANAddr ;
  struct ifreq ifr ;
  // Open CAN sockets (one for each interface)

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
  
  // Bind it to the interface

  if (bind(Can0SockFD,(struct sockaddr*)&CANAddr,sizeof(CANAddr))<0) {
    perror ("CANGateway: Could not bind CAN0 socket") ;
    return (3) ;
  } ;
  
  // Second CAN Socket

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
  
  // Bind it to the interface

  if (bind(Can1SockFD,(struct sockaddr*)&CANAddr,sizeof(CANAddr))<0) {
    perror ("CANGateway: Could not bind CAN1 socket") ;
    return (3) ;
  } ;
  return(0);
} 

// Initialise the networks (LAN and CAN 0 and CAN 1)

int InitNetwork(void)
{
  struct addrinfo hints;
  int rv;
  struct sockaddr_in RecAddr;
  int val ;
  
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

  // Bind the socket to the Address
  if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
    // If address in use then try one lower - CANControl might be running
    close (RecSockFD) ;
    
    if ((RecSockFD = socket(AF_INET, SOCK_DGRAM,0)) == -1) {
      perror("CANGateway: Receive socket");
      exit(0);
    }
    
    memset(&RecAddr, 0, sizeof(struct sockaddr_in));
    RecAddr.sin_family      = AF_INET;
    RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
    RecAddr.sin_port        = htons(CAN_PORT_NUM-1);
    fprintf (stderr,"Socket in use, trying %d as TCP (CANControl is running)\n",CAN_PORT_NUM-1) ;
    
    // Bind the socket to the Address
    if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
      fprintf(stderr,"Could not connect to CANControl\n") ;
      exit(-1) ;
    } ;
  } ;

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
  
  // Make it a broadcast socket

  setsockopt(SendSockFD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;

  relinit(SendSockFD,SendInfo) ;
  return(0);
}  


// Close all requested connections

void CloseNetwork (void) 
{
  freeaddrinfo(servinfo);

  close(RecSockFD);
  close(SendSockFD);
}

void CloseCAN(void)
{
  close(Can0SockFD) ;
  close(Can1SockFD) ;
}

void ReInitCAN (void) 
{
  CloseCAN() ;
  fprintf (stderr,"Closing CAN\n") ;
  sleep(1) ;
  system ("ifconfig can0 down") ;
  system ("ifconfig can1 down") ;
  sleep (1) ;
  fprintf (stderr,"Restart CAN\n") ;
  system ("ifconfig can0 up") ;
  system ("ifconfig can1 up") ;
  sleep (3) ;
  fprintf (stderr,"Opening CAN\n") ;
  InitCAN () ;
}
  
  

// Receive all messages from LAN

int ReceiveFromUDP (struct CANCommand *Command)
{
  int i ;
  char *CANIDP ;
  ULONG CANID ;
  unsigned char buf[CANBUFLEN];
  size_t addr_len;
  int numbytes ;
  struct sockaddr_storage their_addr;
  struct sockaddr_in *tap ;
  int Source ;
  
  tap = (struct sockaddr_in*)&their_addr ;

  addr_len = sizeof their_addr;
  Source = GatewayNum ;


  if ((numbytes = relrecvfrom(RecSockFD, buf, CANBUFLEN-1 , 0,
			      (struct sockaddr_in *)tap,(socklen_t*) &addr_len,&Source)) == -1) {
    perror("Lost UDP network (no recvfrom)");
  }
    
  if ((numbytes==0)||(Source==GatewayNum)) { //either Ack or same Station
    Command->Len = 0 ;
    return (0) ;
  } ;
  // Decode UDP packet and fill CANCommand struct

  CANIDP = (char*)&CANID ;
  for (i=0;i<4;i++) CANIDP[i]=buf[i] ;

  GetSourceAddress(CANID,&(Command->FromLine),&(Command->FromAdd)) ;
  GetDestAddress(CANID,&(Command->ToLine),&(Command->ToAdd)) ;
  GetExtendedAddress (CANID,&(Command->Prio),&(Command->Repeat),&(Command->Group)) ;

  Command->Len = buf[4];

  for (i=0;i<Command->Len;i++) Command->Data[i] = buf[i+5] ;

  if ((Verbose==1)&&((NoTime==0)||((Command->Data[0]!=7)&&(Command->Data[0]!=16)))) {
    fprintf (logfd,"%s: ",LogTime()) ;
    fprintf (logfd,"IP:%s:%d, From: %d %d To: %d %d, Len: %d , Command: %s ",inet_ntoa(tap->sin_addr),tap->sin_port,
	     Command->FromLine, Command->FromAdd,
	     Command->ToLine, Command->ToAdd, Command->Len, ToCommand(Command->Data[0])) ;
    for (i=1;i<Command->Len;i++) fprintf (logfd,"%02x ",Command->Data[i]) ;
    fprintf (logfd,"\n") ;
  } ;
    
  return 0;
}


// Receive one Command from CAN

int ReceiveFromCAN (int socket, struct CANCommand *Command)
{
  int i ;
  struct can_frame frame ;
  int numbytes ;
  
  if ((numbytes = read(socket, &frame, sizeof(struct can_frame))) < 0) {
    perror("CANGateway: CAN raw socket read");
    ReInitCAN () ;
    return 0 ;
  } ;

  // Fill CANCommand struct

  GetSourceAddress(frame.can_id,&(Command->FromLine),&(Command->FromAdd)) ;
  GetDestAddress(frame.can_id,&(Command->ToLine),&(Command->ToAdd)) ;
  GetExtendedAddress (frame.can_id,&(Command->Prio),&(Command->Repeat),&(Command->Group)) ;

  for (i=0;i<frame.can_dlc;i++) Command->Data[i]=frame.data[i] ;
  Command->Len = frame.can_dlc;
    
  if (Verbose==1) {
    fprintf (logfd,"%s: ",LogTime()) ;
    fprintf (logfd,"CAN%d: %d %d, To: %d %d, Len: %d , Command: %s ",socket==Can0SockFD?0:1,
	     Command->FromLine, Command->FromAdd,
	     Command->ToLine, Command->ToAdd, Command->Len, ToCommand(Command->Data[0])) ;
    for (i=1;i<Command->Len;i++) fprintf (logfd,"%02x ",Command->Data[i]) ;
    fprintf (logfd,"\n") ;
  } ;

  return 0;
}

// Send out a CANCommand struct to the specified CAN socket

int SendToCAN (int socket, struct CANCommand *Command)
{
  int i ;
  struct can_frame frame ;
  int numbytes ;
  ULONG CANID ;

  CANID = BuildCANId(Command->Prio,Command->Repeat,Command->FromLine,Command->FromAdd,
		     Command->ToLine,Command->ToAdd,Command->Group) ;
  frame.can_id = CANID|CAN_EFF_FLAG ;
  frame.can_dlc = Command->Len ;
  for (i=0;i<Command->Len;i++) frame.data[i] = Command->Data[i] ;
  
  if ((numbytes = write(socket, &frame, sizeof(struct can_frame))) < 0) {
    perror("CANGateway: CAN raw socket write");
    ReInitCAN () ;
  } ;
  
  return 0;
}

// Send out a CANCommand struct to the LAN

int SendToUDP (struct CANCommand *Command)
{
  int i ;
  unsigned char Message [CANBUFLEN] ;
  char *CANIDP ;
  ULONG CANID ;
  int numbytes;  

  /* Nachricht zusammensetzen */
  CANID = BuildCANId(Command->Prio,Command->Repeat,Command->FromLine,Command->FromAdd,
		     Command->ToLine,Command->ToAdd,Command->Group) ;
  CANIDP = (char*)&CANID ;
  for (i=0;i<4;i++) Message[i] = CANIDP[i] ;

  Message[4] = Command->Len ;

  for (i=0;i<Command->Len;i++) Message[i+5] = Command->Data[i] ;
  for (;i<8;i++) Message[i+5] = 0 ;
  
  if ((numbytes = relsendto(SendSockFD, Message, 13, 0,
			    (struct sockaddr_in*)SendInfo->ai_addr, SendInfo->ai_addrlen,GatewayNum)) != 13) {
    perror("SendCANMessage to UDP");
  }

  return (0);
}

// CommandMatch validates if a Filter exists for the given CANCommand struct

struct CANCommand *CommandMatch(struct CANCommand *Command)
{
  struct CANCommand *Filter ;
  // Finds matching exchange entry; logic is the same as with most common CAN filters : 
  // Only the bits set in the mask are compared with each other

  for (Filter=FilterList;Filter!=NULL;Filter=Filter->Next) {
    if ((Command->FromLine&Filter->FromLineMask)!=(Filter->FromLine&Filter->FromLineMask)) continue ;
    if ((Command->FromAdd&Filter->FromAddMask)!=(Filter->FromAdd&Filter->FromAddMask)) continue ;
    if ((Command->ToLine&Filter->ToLineMask)!=(Filter->ToLine&Filter->ToLineMask)) continue ;
    if ((Command->ToAdd&Filter->ToAddMask)!=(Filter->ToAdd&Filter->ToAddMask)) continue ;
    if ((Command->Prio&Filter->PrioMask)!=(Filter->Prio&Filter->PrioMask)) continue ;
    if ((Command->Repeat&Filter->RepeatMask)!=(Filter->Repeat&Filter->RepeatMask)) continue ;
    if ((Command->Group&Filter->GroupMask)!=(Filter->Group&Filter->GroupMask)) continue ;
    if ((Command->Len&Filter->LenMask)!=(Filter->Len&Filter->LenMask)) continue ;
    if ((Command->Data[0]&Filter->DataMask[0])!=(Filter->Data[0]&Filter->DataMask[0])) continue ;
    if ((Command->Data[0]&Filter->DataMask[1])!=(Filter->Data[0]&Filter->DataMask[1])) continue ;
    if ((Command->Data[0]&Filter->DataMask[2])!=(Filter->Data[0]&Filter->DataMask[2])) continue ;
    if ((Command->Data[0]&Filter->DataMask[3])!=(Filter->Data[0]&Filter->DataMask[3])) continue ;
    if ((Command->Data[0]&Filter->DataMask[4])!=(Filter->Data[0]&Filter->DataMask[4])) continue ;
    if ((Command->Data[0]&Filter->DataMask[5])!=(Filter->Data[0]&Filter->DataMask[5])) continue ;
    if ((Command->Data[0]&Filter->DataMask[6])!=(Filter->Data[0]&Filter->DataMask[6])) continue ;
    if ((Command->Data[0]&Filter->DataMask[7])!=(Filter->Data[0]&Filter->DataMask[7])) continue ;
  }
  return (Filter) ;
}

// RouteCommand sends out the given CANCommand depending on receiving interface and routing tables
    
void RouteCommand (struct CANCommand *Command)
{
  if (NoRoute==1) return ;
  if (Command->Interface!=CIF_NET) {
    // Received by CAN, Send out to network
    SendToUDP(Command) ;
    if ((Verbose==1)&&((NoTime==0)||((Command->Data[0]!=7)&&(Command->Data[0]!=16)))) {
      fprintf (logfd,"%s: ",LogTime()) ;
      fprintf (logfd,"Routed to UDP\n") ;
    } ;
  } ;
  if (Command->Interface!=CIF_CAN0) {
    // Received by CAN1 or network, send to CAN0 if included in routing table
    if (RouteIF0[(int)Command->ToLine]!=0) {
      if (RouteIF0[(int)Command->FromLine]==0) {
	SendToCAN(Can0SockFD,Command) ;
	if ((Verbose==1)&&((NoTime==0)||((Command->Data[0]!=7)&&(Command->Data[0]!=16)))) {
	  fprintf (logfd,"%s: ",LogTime()) ;
	  fprintf (logfd,"Routed to CAN0\n") ;
	} ;
      } ;
    } ;
  } ;  
  if (Command->Interface!=CIF_CAN1) {
    // Received by CAN0 or network, send to CAN1 if included in routing table
    if (RouteIF1[(int)Command->ToLine]!=0) {
      if (RouteIF1[(int)Command->FromLine]==0) {
	SendToCAN(Can1SockFD,Command) ;
	if ((Verbose==1)&&((NoTime==0)||((Command->Data[0]!=7)&&(Command->Data[0]!=16)))) {
	  fprintf (logfd,"%s: ",LogTime()) ;
	  fprintf (logfd,"Routed to CAN1\n") ;
	} ;
      } ;
    } ;
  } ;  

}

// RewriteCommand exchanges the information in the Command at the location where the bits are set in the modifier mask
// with the information given in the modifier CANCommand.

void RewriteCommand (struct CANCommand *Command, struct CANCommand *Exchange, struct CANCommand *NewCommand)
{
  int i ;
  // Exchange all the bits that are set in the Exchange-Mask
  NewCommand->FromLine = (Command->FromLine&(~Exchange->FromLineMask))|(Exchange->FromLine&Exchange->FromLineMask) ;
  NewCommand->FromAdd = (Command->FromAdd&(~Exchange->FromAddMask))|(Exchange->FromAdd&Exchange->FromAddMask) ;
  NewCommand->ToLine = (Command->ToLine&(~Exchange->ToLineMask))|(Exchange->ToLine&Exchange->ToLineMask) ;
  NewCommand->ToAdd = (Command->ToAdd&(~Exchange->ToAddMask))|(Exchange->ToAdd&Exchange->ToAddMask) ;
  NewCommand->ToAdd = (Command->Prio&(~Exchange->PrioMask))|(Exchange->Prio&Exchange->PrioMask) ;
  NewCommand->ToAdd = (Command->Repeat&(~Exchange->RepeatMask))|(Exchange->Repeat&Exchange->RepeatMask) ;
  NewCommand->ToAdd = (Command->Group&(~Exchange->GroupMask))|(Exchange->Group&Exchange->GroupMask) ;
  NewCommand->Len = (Command->Len&(~Exchange->LenMask))|(Exchange->Len&Exchange->LenMask) ;
  for (i=0;i<8;i++) {
    NewCommand->Data[i] = (Command->Data[i]&(~Exchange->DataMask[i]))|(Exchange->Data[i]&Exchange->DataMask[i]) ;
  } ;
}

int8_t buff[ARTNET_DMX_LENGTH +1];

int length;

int dmx_callback(artnet_node n, int port, void *d) 
{
  uint8_t *data;
  struct can_frame frame ;
  ULONG CANID ;
  int numbytes ;
  int i,j ;
  
  data = artnet_read_dmx(n, port, &length);
  memset(buff, 0x00, ARTNET_DMX_LENGTH+1);
  memcpy(buff, data, length);

  // Send out CAN-Data
  for (i=0;i<ARTNET_DMX_LENGTH/2;) {
    if (CANBuffer[port][i].Line==0) break ; // Finished
    CANID = BuildCANId(0,0,0,253,CANBuffer[port][i].Line,CANBuffer[port][i].Add,0) ;
    frame.can_id = CANID|CAN_EFF_FLAG ;
    frame.can_dlc = 8 ;
    frame.data[0] = 41; // LOAD_TWO_LED ;
    frame.data[1] = CANBuffer[port][i].Start/3 ;
    for (j=0;j<6;j++) frame.data[i+2] = buff[CANBuffer[port][i].Map[j]] ;
    
    if (RouteIF0[(int)CANBuffer[port][i].Line]!=0) {
      if ((numbytes = write(Can0SockFD, &frame, sizeof(struct can_frame))) < 0) {
	perror("CANGateway: CAN raw socket write");
	ReInitCAN () ;
	return 0;
      } ;
    } ;
    if (RouteIF1[(int)CANBuffer[port][i].Line]!=0) {
      if ((numbytes = write(Can1SockFD, &frame, sizeof(struct can_frame))) < 0) {
	perror("CANGateway: CAN raw socket write");
	ReInitCAN () ;
	return 0 ;
      } ;
    } ;
  } ;
  return 0;
}

// main routine

int main (int argc, char*argv[])
{
  int i ;
  fd_set rdfs ;
  struct CANCommand Command ;
  struct CANCommand NewCommand ;
  struct CANCommand *Exchange ;
  struct timeval tv;


  logfd = stderr ;
  for (i=1;i<argc;i++) {
    if (strstr("-v",argv[i])!=NULL) Verbose = 1 ;
    if (strstr("-t",argv[i])!=NULL) NoTime = 1 ;
    if (strstr("-noroute",argv[i])!=NULL) NoRoute = 1 ;
    if (strstr("-f",argv[i])!=NULL) {
      logfd = fopen(argv[i+1],"w") ;
      if (logfd==NULL) {
	fprintf (stderr,"Could not open logfile %s\n",argv[i+1]) ;
	logfd = stderr ;
      } ;
      setvbuf (logfd,NULL,_IOLBF,0) ;
      i++ ;
    } ;
    if ((strstr("-?",argv[i])!=NULL)||
	(strstr("--help",argv[i])!=NULL)) {
      printf ("Usage: %s [-v] [-t] [-noroute] [--help] [-?]\n\n",argv[0]) ;
      printf ("       -v: Verbose\n") ;
      printf ("       -t: dont display time messages\n") ;
      printf ("       -noroute: Do not route messages\n") ;
      printf ("       -f FILE: log into file FILE\n") ;
      printf ("       -?, --help: display this message\n") ;
      exit(0) ;
    } ;
  } ;



  ReadConfig() ;

  printf ("Initializing %s at port %d\n",CAN_BROADCAST,CAN_PORT_NUM) ;
  printf ("Routing to Line0:\n"); 
  for (i=0;i<255;i++) if (RouteIF0[i]!=0) printf ("%d ",i) ;
  printf ("\n") ;
  printf ("Routing to Line1:\n"); 
  for (i=0;i<255;i++) if (RouteIF1[i]!=0) printf ("%d ",i) ;
  printf ("\n") ;
  
  if (InitNetwork ()>0) exit(0) ;
  if (InitCAN ()>0) exit(0) ;

  node1 = artnet_new(NULL, Verbose);
  artnet_set_short_name(node1, "Artnet -> CAN (1)");
  artnet_set_long_name(node1, "ArtNet to CAN convertor");
  artnet_set_node_type(node1, ARTNET_NODE);
  artnet_set_dmx_handler(node1, dmx_callback, NULL);
  for(i=0; i<4; i++) {
    artnet_set_port_addr(node1, i, ARTNET_OUTPUT_PORT, i);
    artnet_set_subnet_addr(node1, 0);
  } ;
  artnet_start(node1);
  artnetFD = artnet_get_sd(node1);

  
  // initialisation
  NewCommand.Next = 0 ;
  NewCommand.Interface = 0 ;
  NewCommand.Exchange= 0 ;


  for (;;) {
    // Main loop - wait for something to receive and then filter/exchange it and route it
    FD_ZERO(&rdfs) ;
    FD_SET(RecSockFD,&rdfs) ;
    FD_SET(Can0SockFD,&rdfs) ;
    FD_SET(Can1SockFD,&rdfs) ;
    FD_SET(artnetFD,&rdfs) ;

    tv.tv_sec = 0 ;
    tv.tv_usec = 10000 ; // will be deleted by select!


    if ((i=select(Can1SockFD+1,&rdfs,NULL,NULL,&tv))<0) {
      perror ("CANGateway: Could not select") ;
      exit (1) ;
    } ;

    gettimeofday(&Now,NULL) ;

    relworkqueue () ;

    if (i==0) {
      continue ; // was timeout only
    } ;
    
    Command.Interface = 0 ; // Delete old communication

    if (FD_ISSET(RecSockFD,&rdfs)) {
      ReceiveFromUDP (&Command) ;
      Command.Interface = CIF_NET ;
      if (Command.Len==0) continue ;
    } else if (FD_ISSET(Can0SockFD,&rdfs)) {
      ReceiveFromCAN (Can0SockFD,&Command) ;
      Command.Interface = CIF_CAN0 ;
    } else if (FD_ISSET(Can1SockFD,&rdfs)) {
      ReceiveFromCAN (Can1SockFD,&Command) ;
      Command.Interface = CIF_CAN0 ;
    } else if (FD_ISSET(artnetFD,&rdfs)) {
      artnet_read(node1,0); 
    } ;

    usleep(2000) ;

    if (Command.Interface!=0) {
      // Look-up if received Element should be exchanged with other element(s)
      Exchange = CommandMatch(&Command) ;
      if (Exchange!=NULL) {
	for (Exchange=Exchange->Exchange;Exchange!=NULL;Exchange=Exchange->Next) {
	  RewriteCommand(&Command,Exchange,&NewCommand) ;
	  RouteCommand(&NewCommand) ;
	} ;
      } else {
	// no exchange element
	RouteCommand(&Command) ;
      } ;
    } ;
  } ;
  
  // will currently never be reached...
  CloseNetwork () ;
  CloseCAN () ;
}

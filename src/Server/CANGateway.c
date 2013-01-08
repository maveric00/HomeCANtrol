/*
** CANGateway.c -- Gateway (and simple filter) functionality between UDP and CAN for Linux CAN (socketcan)
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
  "Channel on",
  "Channel off",
  "Channel toggle",
  "Shade up (full)",
  "Shade down (full)",
  "Shade up (short)",
  "Shade down (short)",
  "Send LEDPort",
  "Undefined (28)",
  "Undefined (29)",
  "LED off",
  "LED on",
  "Set to",
  "HSet to",
  "Load and store",
  "Set to G1",
  "Set to G2",
  "Set To G3",
  "Load Low",
  "Load Mid1",
  "Load Mid2",
  "Load High",
  "Start program",
  "Stop program",
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
  "Stop Sensor"
} ;

char CommandNum[40]; 

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
  if ((Command<1)||(Command>54)) {
    sprintf (CommandNum,"Out of Range: %d(0x%02x)",Command,Command) ;
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
int Verbose=0 ;
int NoTime=0 ;
int NoRoute=0 ;

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;

int RecSockFD;
int SendSockFD;
int Can0SockFD;
int Can1SockFD;

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

struct CANCommand *FilterList ;

char RouteIF0[255] ;
char RouteIF1[255] ;

// Read a CANCommand (from, to, length and data) including mask from configuration file

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
    
    // Read routing information; Format: "CAN0-Line: Line1 Line2 Line3 ..."
    // Messages targeted to Line1,... are sent out to CAN0

    if (strstr(Line,"CAN0-Line:")) {
      for (i=0;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      i++ ;
      for (;Line[i]!='\0';i++) {
	sscanf (&(Line[i]),"%d",&j) ;
	if ((j>0)&&(j<255)) RouteIF0[j]=1 ;
	for (;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      } ;
    } ;

    // Same for CAN1

    if (strstr(Line,"CAN1-Line:")) {
      for (i=0;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      i++ ;
      for (;Line[i]!='\0';i++) {
	sscanf (&(Line[i]),"%d",&j) ;
	if ((j>0)&&(j<255)) RouteIF1[j]=1 ;
	for (;(Line[i]!='\0')&&(Line[i]!=' ');i++) ;
      } ;
    } ;

    // If filter information available, set up Filter and possibly modification rules

    if (strstr(Line,"Exchange")) {
      ReadFilter(Conf) ;
    } ;
  } ;

  fclose (Conf) ;  

  // Set to default if no routing was configured
  for (i=0;(i<255)&&(RouteIF0[i]==0);i++) ;
  if (i==255) for(i=0;i<255;i++) RouteIF0[i] = 1 ;

  for (i=0;(i<255)&&(RouteIF1[i]==0);i++) ;
  if (i==255) for(i=0;i<255;i++) RouteIF1[i] = 1 ;
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

// Initialise the networks (LAN and CAN 0 and CAN 1)

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

  // Bind the socket to the Address
  
  if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
    // If address in use then try one lower - CANControl might be running
    fprintf (stderr,"Socket in use, trying one lower (CANControl is running)\n") ;
    close (RecSockFD) ;

    if ((RecSockFD = socket(AF_INET, SOCK_DGRAM,0)) == -1) {
      perror("CANGateway: Receive socket");
      exit(0);
    }

    memset(&RecAddr, 0, sizeof(struct sockaddr_in));
    RecAddr.sin_family      = AF_INET;
    RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
    RecAddr.sin_port        = htons(CAN_PORT_NUM-1);
    if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
      close(RecSockFD);
      perror("listener: bind");
    }
  } ;
  
  // Receive broadcast, also

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
} ;


// Receive all messages from LAN

int ReceiveFromUDP (struct CANCommand *Command)
{
  int i ;
  char *CANIDP ;
  ULONG CANID ;
  char buf[CANBUFLEN];
  size_t addr_len;
  int numbytes ;
  struct sockaddr_storage their_addr;
  struct sockaddr_in *tap ;
  
  tap = (struct sockaddr_in*)&their_addr ;

  addr_len = sizeof their_addr;
  if ((numbytes = recvfrom(RecSockFD, buf, CANBUFLEN-1 , 0,
			   (struct sockaddr *)tap, &addr_len)) == -1) {
    perror("Lost UDP network (no recvfrom)");
  }

  // Decode UDP packet and fill CANCommand struct

  CANIDP = (char*)&CANID ;
  for (i=0;i<4;i++) CANIDP[i]=buf[i] ;

  GetSourceAddress(CANID,&(Command->FromLine),&(Command->FromAdd)) ;
  GetDestAddress(CANID,&(Command->ToLine),&(Command->ToAdd)) ;
  GetExtendedAddress (CANID,&(Command->Prio),&(Command->Repeat),&(Command->Group)) ;

  Command->Len = buf[4];

  for (i=0;i<Command->Len;i++) Command->Data[i] = buf[i+5] ;

  if ((Verbose==1)&&((NoTime==0)||((Command->Data[0]!=7)&&(Command->Data[0]!=16)))) {
    fprintf (stderr,"IP:%s:%d, From: %d %d To: %d %d, Len: %d , Command: %s ",inet_ntoa(tap->sin_addr),tap->sin_port,
	     Command->FromLine, Command->FromAdd,
	     Command->ToLine, Command->ToAdd, Command->Len, ToCommand(Command->Data[0])) ;
    for (i=1;i<Command->Len;i++) fprintf (stderr,"%02x ",Command->Data[i]) ;
    fprintf (stderr,"\n") ;
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
  } ;

  // Fill CANCommand struct

  GetSourceAddress(frame.can_id,&(Command->FromLine),&(Command->FromAdd)) ;
  GetDestAddress(frame.can_id,&(Command->ToLine),&(Command->ToAdd)) ;
  GetExtendedAddress (frame.can_id,&(Command->Prio),&(Command->Repeat),&(Command->Group)) ;

  for (i=0;i<frame.can_dlc;i++) Command->Data[i]=frame.data[i] ;
  Command->Len = frame.can_dlc;
    
  if (Verbose==1) {
    fprintf (stderr,"CAN%d: %d %d, To: %d %d, Len: %d , Command: %s",socket==Can0SockFD?0:1,
	     Command->FromLine, Command->FromAdd,
	     Command->ToLine, Command->ToAdd, Command->Len, ToCommand(Command->Data[0])) ;
    for (i=1;i<Command->Len;i++) fprintf (stderr,"%02x ",Command->Data[i]) ;
    fprintf (stderr,"\n") ;
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
  } ;
  
  return 0;
}

// Send out a CANCommand struct to the LAN

int SendToUDP (struct CANCommand *Command)
{
  int i ;
  char Message [CANBUFLEN] ;
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
  
  if ((numbytes = sendto(SendSockFD, Message, 13, 0,
			 SendInfo->ai_addr, SendInfo->ai_addrlen)) != 13) {
    perror("SendCANMessage to UDP");
  }

  return (0);
}

// Close all requested connections

void CloseNetwork (void) 
{
  freeaddrinfo(servinfo);

  close(RecSockFD);
  close(SendSockFD);
  close(Can0SockFD) ;
  close(Can1SockFD) ;
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
      fprintf (stderr,"Routed to UDP\n") ;
    } ;
  } ;
  if (Command->Interface!=CIF_CAN0) {
    // Received by CAN1 or network, send to CAN0 if included in routing table
    if (RouteIF0[(int)Command->ToLine]!=0) {
      if (RouteIF0[(int)Command->FromLine]==0) {
	SendToCAN(Can0SockFD,Command) ;
	if ((Verbose==1)&&((NoTime==0)||((Command->Data[0]!=7)&&(Command->Data[0]!=16)))) {
	  fprintf (stderr,"Routed to CAN0\n") ;
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
	  fprintf (stderr,"Routed to CAN1\n") ;
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

// main routine

int main (int argc, char*argv[])
{
  int i ;
  fd_set rdfs ;
  struct CANCommand Command ;
  struct CANCommand NewCommand ;
  struct CANCommand *Exchange ;

  for (i=1;i<argc;i++) {
    if (strstr("-v",argv[i])!=NULL) Verbose = 1 ;
    if (strstr("-t",argv[i])!=NULL) NoTime = 1 ;
    if (strstr("-noroute",argv[i])!=NULL) NoRoute = 1 ;
    if ((strstr("-?",argv[i])!=NULL)||
	(strstr("--help",argv[i])!=NULL)) {
      printf ("Usage: %s [-v] [-noroute] [--help] [-?]\n\n",argv[0]) ;
      printf ("       -v: Verbose\n") ;
      printf ("       -noroute: Do not route messages\n") ;
      printf ("       -?, --help: display this message\n") ;
      exit(0) ;
    } ;
  } ;

  ReadConfig() ;

  printf ("Initializing %s at port %d\n",CAN_BROADCAST,CAN_PORT_NUM) ;
  
  InitNetwork () ;
  
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

    if ((i=select(Can1SockFD+1,&rdfs,NULL,NULL,NULL))<0) {
      perror ("CANGateway: Could not select") ;
      exit (1) ;
    } ;
    
    if (FD_ISSET(RecSockFD,&rdfs)) {
      ReceiveFromUDP (&Command) ;
      Command.Interface = CIF_NET ;
    } ;

    if (FD_ISSET(Can0SockFD,&rdfs)) {
      ReceiveFromCAN (Can0SockFD,&Command) ;
      Command.Interface = CIF_CAN0 ;
    } ;

    if (FD_ISSET(Can1SockFD,&rdfs)) {
      ReceiveFromCAN (Can1SockFD,&Command) ;
      Command.Interface = CIF_CAN0 ;
    } ; 

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
  
  // will currently never be reached...
  CloseNetwork () ;
}

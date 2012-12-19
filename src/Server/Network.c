/* Network.c: Definitionen fuer die CAN-Kommunikation ueber Ethernet 
   Es werden jeweils die max. moeglichen 8 Byte als UDP-Paket an die Broadcast-Adresse gesendet, das
   Routing uebernehmen die Ethernet-CAN-Umsetzer
*/

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include "libwebsocket/libwebsockets.h"
#include "ConfigNodes.h"
#include "XMLConfig.h"
#include "Network.h"
#define SERVER_INCLUDE 1
#include "../Apps/Common/mcp2515.h"

char CAN_PORT[20] ;
int CAN_PORT_NUM ;
char WS_PORT[20] ;
int WS_PORT_NUM ;
char COM_PORT[20] ;
int COM_PORT_NUM ;
char HTTP_PORT[20] ;
int HTTP_PORT_NUM ;
char CAN_BROADCAST[NAMELEN] ;

int RecSockFD;
int ComSockFD;
int SendSockFD;
int RecAjaxFD ;
int SendAjaxFD ;
int MaxSock ;
fd_set socketReadSet;
struct addrinfo *servinfo, *SendInfo ;

/* CAN-Funktionen */

/* Die 29-Bit CAN ID aus den Adress-Informationen zusammensetzen */

ULONG BuildCANId (char Prio, char Repeat, char FromLine, USHORT FromAdd, char ToLine, USHORT ToAdd, char Group)
{
  return ((Group&0x1)<<1|ToAdd<<2|(ToLine&0xf)<<10|FromAdd<<14|(FromLine&0xf)<<22|(Repeat&0x1)<<26|(Prio&0x3)<<27) ;
}

/* Ueberpruefungsfunktion, ob die Adresse Ziel der CAN-ID war */

int MatchAddress (ULONG CANId, char ToLine, USHORT ToAdd)
{
  ToLine = ToLine&0xf ;
  if (((CANId & (0xf<<10))==(ToLine<<10))&&(((CANId & (0xff<<2))==(ToAdd<<2)))) {
    return TRUE ;
  } else {
    return FALSE ;
  } ;
}

/* Ueberpruefungsfunktion, ob die Adresse Quelle der CAN-ID war */
int MatchSrcAddress (ULONG CANId, char FromLine, USHORT FromAdd)
{
  FromLine = FromLine&0xf ;
  if (((CANId & (0xf<<22))==(FromLine<<22))&&(((CANId & (0xff<<14))==(FromAdd<<14)))) {
    return TRUE ;
  } else {
    return FALSE ;
  } ;
}

/* Abspeichern der Quell-Adresse aus einer CAN-ID */

void GetSourceAddress (ULONG CANId, char *FromLine, USHORT *FromAdd)
{
  *FromLine = (char)((CANId>>22)&0xf) ;
  *FromAdd = (USHORT) ((CANId>>14)&0xff) ;
}

void GetDestinationAddress (ULONG CANId, char *ToLine, USHORT *ToAdd)
{
  *ToLine = (char)((CANId>>10)&0xf) ;
  *ToAdd = (USHORT) ((CANId>>2)&0xff) ;
}

/* Netzwerk-Funktionen */

/* Netzwerk-Interface initialisieren */

int InitNetwork(void)
{
  
  struct addrinfo hints;
  int rv;
  struct sockaddr_in RecAddr;
  int val ;
  
  val = (0==0) ;
  // Reserve Receive-Socket for CAN data
  if ((RecSockFD = socket(AF_INET, SOCK_DGRAM,0)) == -1) {
    perror("talker: socket");
    fprintf(stderr, "Could not get socket\n");
    return(1);
  }
  
  // Set binding informations

  memset(&RecAddr, 0, sizeof(struct sockaddr_in));
  RecAddr.sin_family      = AF_INET;
  RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
  RecAddr.sin_port        = htons(CAN_PORT_NUM);
  
  if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
    close(RecSockFD);
    perror("listener: bind");
  }
  
  // Mark it as Broadcast

  setsockopt(RecSockFD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;
  
  // Reserve Receive-Socket for Commands

  if ((ComSockFD = socket(AF_INET, SOCK_STREAM,0)) == -1) {
    perror("talker: command socket");
    fprintf(stderr, "Could not get socket\n");
    return(1);
  }
  
  // Set binding informations

  memset(&RecAddr, 0, sizeof(struct sockaddr_in));
  RecAddr.sin_family      = AF_INET;
  RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
  RecAddr.sin_port        = htons(COM_PORT_NUM);
  
  if (bind(ComSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
    close(ComSockFD);
    perror("listener: bind");
  }
  
  // Set up send-Socket for CAN data
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
  setsockopt(SendSockFD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;

  // Listen on Command Socket

  if (listen(ComSockFD,10)==-1) {
    fprintf (stderr,"listener: failed to listen on command port\n") ;
    return 2 ;
  }
  
  // Init select set
  
  FD_ZERO(&socketReadSet);
  FD_SET(RecSockFD,&socketReadSet);
  FD_SET(ComSockFD,&socketReadSet);

  // set maximum socket of select-set

  MaxSock = RecSockFD>ComSockFD?RecSockFD:ComSockFD ;

  return (0) ;


} ;


/* CAN Message empfangen (wartet so lange, bis eine kommt) */

int ReceiveCANMessage (ULONG *CANID, char *Len, unsigned char *Data)
{
  int i ;
  unsigned char *CANIDP ;
  unsigned char buf[CANBUFLEN];
  socklen_t addr_len;
  int numbytes ;
  struct sockaddr_storage their_addr;
  
  
  addr_len = sizeof their_addr;
  if ((numbytes = recvfrom(RecSockFD, buf, CANBUFLEN-1 , 0,
			   (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    fprintf(stderr,"Could not read from socket?\n");
    return(1);
  }


  CANIDP = (unsigned char*)CANID ;
  for (i=0;i<4;i++) CANIDP[i]=buf[i] ;
  *Len = buf[4];

  for (i=0;i<*Len;i++) Data[i] = buf[i+5] ;
  for (;i<8;i++) buf[i+5] = 0 ;
    
  return 0;
}

/* CAN-Nachricht senden */

int SendCANMessage (ULONG CANID, char Len, unsigned char *Data)
{
  int i ;
  unsigned char Message [CANBUFLEN] ;
  unsigned char *CANIDP ;
  int numbytes;  

  /* Nachricht zusammensetzen */
  CANIDP = (unsigned char*)&CANID ;
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

// Wartet auf Empfang und oeffne ggf. neue Command-Channels

struct ListItem *Connections=NULL ;

int CheckNetwork(int * error,int timeOut) // milliseconds
{
  fd_set localReadSet;
  struct sockaddr_storage remoteaddr ;
  socklen_t addrlen ;
  int newfd ;
  int NumSockets;
  int MaxSockOld ;
  struct timeval tv;
  int i ;
  char buf[NAMELEN*5] ;
  int nbytes ;
  struct ListItem *Connect ;
  char Answer[NAMELEN*4] ;
  
  localReadSet = socketReadSet ;
  
  if (timeOut) {
    tv.tv_sec  = timeOut / 1000;
    tv.tv_usec = (timeOut % 1000) * 1000;
  } else {
    tv.tv_sec  = 0;
    tv.tv_usec = 0;
  } ;
  
  if ((NumSockets=select(MaxSock+1,&localReadSet,NULL,NULL,&tv)) == -1) {
    *error = 1;
    return 0;
  } ;

  if (NumSockets==0) return (FALSE) ;

  *error = 0;
  
  MaxSockOld = MaxSock ;

  for (i=0;i<=MaxSockOld;i++) {
    if FD_ISSET(i,&localReadSet) {
      if (i==RecSockFD) continue ; // CAN data will be handled at caller
      if (i==ComSockFD) {
	// New connection request
	addrlen = sizeof(remoteaddr) ;
	newfd = accept (ComSockFD,(struct sockaddr*)&remoteaddr,&addrlen) ;
	if (newfd==-1) {
	  fprintf (stderr,"CheckNetwork: accept denied\n") ;
	} else {
	  // add to listening socket set
	  FD_SET (newfd,&socketReadSet) ;
	  MaxSock=newfd>MaxSock?newfd:MaxSock ;
	  Connect = CreateItem(Connections) ;
	  if (Connections==NULL) Connections = Connect ;
	  Connect->Number = newfd ;
	}
      } else {
	// Command was sent, read it
	if ((nbytes=recv(i,buf,sizeof(buf)-1,0))<=0) {
	  // error or connection closed 
	  if (nbytes!=0) {
	    fprintf (stderr,"CheckNetwork: error receiving\n") ;
	  } ;
	  close (i) ;
	  FD_CLR(i,&socketReadSet) ;
	  fprintf (stderr,"CheckNetwork: Connection closed\n") ;
	  if (i==MaxSock) {
	    // reduce maximum socket number ;
	    for (MaxSock--;!FD_ISSET(MaxSock,&socketReadSet);MaxSock--) ;
	  } ;
	  for (Connect=Connections;Connect!=NULL;Connect=Connect->Next) if (Connect->Number==i) break ;
	  if (Connect==Connections) Connections = Connect->Next ;
	  FreeItem(Connect) ;
	}  else {
	  for (Connect=Connections;Connect!=NULL;Connect=Connect->Next) if (Connect->Number==i) break ;
	  if (Connect==NULL) {
	    // received on unknown connection ???
	    fprintf (stderr,"Unknown Connection %d\n",i) ;
	  } else {
	    buf[nbytes] = 0 ;
	    strcat ((char*)Connect->Data.Command,buf) ; // Concatenate Command
	    if (strstr((char*)Connect->Data.Command,"\n")!=NULL) {
	      // Execute Command
	      if (HandleCommand ((char*)Connect->Data.Command,Answer)) {
		send (i,Answer,strlen(Answer),0);
	      }; 
	      Connect->Data.Command[0] = '\0' ;
	    } ;
	  } ;
	} ;
      } ;
    } ;
  } ;
  return FD_ISSET(RecSockFD,&localReadSet) != 0;
}


/* Aufraeum-Funktion */

void CloseNetwork (void) 
{
  freeaddrinfo(servinfo);

  close(RecSockFD);
  close(ComSockFD);
  close(SendSockFD);
}


void SendAction (struct Node *Action)
{
  tCommand Command ;
  ULONG CANID ;
  unsigned char Data[8]; 
  char Len ;
  int Linie,Knoten,Port ;
  static struct timeval Old ;
  struct timeval Now,Wait ;

  Wait.tv_sec=0;
  Wait.tv_usec=1000;

  while(1) {
    gettimeofday(&Now,NULL) ;
    if (timercmp(&Now,&Old,>)) break ;
  } ;
  timeradd(&Now,&Wait,&Old) ;

  Command = 0 ;
  switch (Action->Data.Aktion.Unit->Type) {
  case N_ONOFF:
    if (Action->Data.Aktion.Type==A_ON) { Command = CHANNEL_ON ; Action->Data.Aktion.Unit->Value=1 ; } ;
    if (Action->Data.Aktion.Type==A_OFF) { Command = CHANNEL_OFF ; Action->Data.Aktion.Unit->Value=0 ; } ;
    if (Action->Data.Aktion.Type==A_TOGGLE) { Command = CHANNEL_TOGGLE ; Action->Data.Aktion.Unit->Value=1-Action->Data.Aktion.Unit->Value ; } ;
    if (Action->Data.Aktion.Type==A_HEARTBEAT) Command = TIME ;
    if (GetNodeAdress(Action->Data.Aktion.Unit,&Linie,&Knoten,&Port)!=0) break ;
    CANID = BuildCANId(0,0,0,2,Linie,Knoten,0) ;
    Data[0] = Command ;
    Data[1] = (char)Port ;
    Len=2 ;
    break ;
  case N_SENSOR:
    if (Action->Data.Aktion.Type==A_ON) { Command = SET_PIN ; Data[2] = 0xFF ; Action->Data.Aktion.Unit->Value=1 ; } ;
    if (Action->Data.Aktion.Type==A_OFF) { Command = SET_PIN ; Data[2] = 0x00 ; Action->Data.Aktion.Unit->Value=0 ; } ;
    if (Action->Data.Aktion.Type==A_TOGGLE) { 
      Command = SET_PIN ; 
      Data[2] = Action->Data.Aktion.Unit->Value==0?0xFF:0x00 ; 
      Action->Data.Aktion.Unit->Value=1-Action->Data.Aktion.Unit->Value ; 
    } ;
    if (Action->Data.Aktion.Type==A_SEND_VAL) {
      Data[2] = Action->Data.Aktion.Unit->Value ;
    } ;
    if (Action->Data.Aktion.Type==A_HEARTBEAT) Command = TIME ;
    if (GetNodeAdress(Action->Data.Aktion.Unit,&Linie,&Knoten,&Port)!=0) break ;
    CANID = BuildCANId(0,0,0,2,Linie,Knoten,0) ;
    Data[0] = Command ;
    Data[1] = (char)Port ;
    Len=(Action->Data.Aktion.Type==A_HEARTBEAT)?2:3 ;
    break ;
  case N_SHADE:
    if (Action->Data.Aktion.Type==A_SHADE_UP_FULL) { Command = SHADE_UP_FULL ; Action->Data.Aktion.Unit->Value=0 ; } ;    
    if (Action->Data.Aktion.Type==A_SHADE_DOWN_FULL) { Command = SHADE_DOWN_FULL ; Action->Data.Aktion.Unit->Value=1 ; } ;
    if (Action->Data.Aktion.Type==A_SHADE_UP_SHORT) Command = SHADE_UP_SHORT ;
    if (Action->Data.Aktion.Type==A_SHADE_DOWN_SHORT) Command = SHADE_DOWN_SHORT ;
    if (GetNodeAdress(Action->Data.Aktion.Unit,&Linie,&Knoten,&Port)!=0) break ;
    CANID = BuildCANId(0,0,0,2,Linie,Knoten,0) ;
    Data[0] = Command ;
    Data[1] = (char)Port ;
    Len=2 ;
    break ;
  default:
    break ;
  } ;
  if (Command==0) return;

  SendCANMessage(CANID,Len,Data) ;
}



static int callback_http(struct libwebsocket_context *context,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user,
							   void *in, size_t len)
{
	char client_name[128];
	char client_ip[128];

	switch (reason) {
	case LWS_CALLBACK_HTTP:
		fprintf(stderr, "serving HTTP URI %s\n", (char *)in);

		/* send the script... when it runs it'll start websockets */

		if (libwebsockets_serve_http_file(wsi,
				  "Webpage/index.html", "text/html"))
			fprintf(stderr, "Failed to send HTTP file\n");
		break;

	/*
	 * callback for confirming to continue with client IP appear in
	 * protocol 0 callback since no websocket protocol has been agreed
	 * yet.  You can just ignore this if you won't filter on client IP
	 * since the default uhandled callback return is 0 meaning let the
	 * connection continue.
	 */

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:

		libwebsockets_get_peer_addresses((int)(long)user, client_name,
			     sizeof(client_name), client_ip, sizeof(client_ip));

		fprintf(stderr, "Received network connect from %s (%s)\n",
							client_name, client_ip);

		/* if we returned non-zero from here, we kill the connection */
		break;

	default:
		break;
	}

	return 0;
}

/*
 * this is just an example of parsing handshake headers, you don't need this
 * in your code unless you will filter allowing connections by the header
 * content
 */

static void
dump_handshake_info(struct lws_tokens *lwst)
{
	int n;
	static const char *token_names[WSI_TOKEN_COUNT] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",
		/*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
	};

	for (n = 0; n < WSI_TOKEN_COUNT; n++) {
		if (lwst[n].token == NULL)
			continue;

		fprintf(stderr, "    %s = %s\n", token_names[n], lwst[n].token);
	}
}

/* configuration protocol */

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 * for this example protocol we use it to individualize the count for each
 * connection.
 */

struct per_session_data_config {
  int DataToSend;
  char Unit[NAMELEN*4] ;
};

static int callback_config(struct libwebsocket_context *context,
			   struct libwebsocket *wsi,
			   enum libwebsocket_callback_reasons reason,
			   void *user, void *in, size_t len)
{
  int n;
  unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 +
		    LWS_SEND_BUFFER_POST_PADDING];
  unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
  struct per_session_data_config *pss = user;
  char Command[NAMELEN] ;
  char Objekt[NAMELEN*4] ;
  char Objekt2[NAMELEN] ;
  char Objekt3[NAMELEN] ;
  struct Node *This ;
  
  switch (reason) {
    
  case LWS_CALLBACK_ESTABLISHED:
    // Initialisiere Per Session Data
    pss->DataToSend = WS_UNDEF;
    break;
    
    /*
     * in this protocol, we just use the broadcast action as the chance to
     * send our own connection-specific data and ignore the broadcast info
     * that is available in the 'in' parameter
     */
    
  case LWS_CALLBACK_BROADCAST:
    if (pss->DataToSend!=WS_UNDEF) {
      // in dieser Session stehen Daten an, die gesendet werden sollen
      n= libwebsocket_write(wsi,in,len,LWS_WRITE_TEXT) ;
      if (n < 0) {
	fprintf(stderr, "ERROR writing to socket");
	return 1;
      }
    } ;
    break;
    
  case LWS_CALLBACK_RECEIVE:
    // Es wurde etwas empfangen
    sscanf (in,"%s %s %s %s",Command,Objekt,Objekt2,Objekt3) ;
    fprintf (stderr,"%s %s %s %s\n",Command,Objekt,Objekt2,Objekt3) ;
    if ((strcmp(Command,"Aktion")==0)||(strcmp(Command,"Action")==0)) {
      This = FindNode(Haus->Child,Objekt) ;
      ExecuteMakro (This) ;
    } ;
    if (strcmp(Command,"Status")==0) {
      This = FindNode(Haus->Child,Objekt) ;
      n = sprintf((char *)p, "Set %s %d", Objekt,This->Value);
      n = libwebsocket_write(wsi, p, n, LWS_WRITE_TEXT);
      if (n < 0) {
	fprintf(stderr, "ERROR writing to socket");
	return 1;
      } ;
    } ;      
    
  case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
    dump_handshake_info((struct lws_tokens *)(long)user);
    /* you could return non-zero here and kill the connection */
    break;
    
  default:
    break;
  }
  
  return 0;
}

/* list of supported protocols and callbacks */

struct libwebsocket_protocols web_protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		0			/* per_session_data_size */
	},
	{
		"config-protocol",
		callback_config,
		sizeof(struct per_session_data_config),
	},
	{
		NULL, NULL, 0		/* End of list */
	}
};

struct libwebsocket_context *web_context;

int InitWebsocket(void)
{
  const char *cert_path ;
  const char *key_path ;
  int port = WS_PORT_NUM;
  int opts = 0;
  const char *interface = NULL;


  cert_path = key_path = NULL;

  web_context = libwebsocket_create_context(port, interface, web_protocols,
					libwebsocket_internal_extensions,
					cert_path, key_path, -1, -1, opts);
  if (web_context == NULL) {
    fprintf(stderr, "libwebsocket init failed\n");
    return -1;
  }
  
  return (0) ;
} ;

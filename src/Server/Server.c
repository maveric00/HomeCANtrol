#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#define _XOPEN_SOURCE
#include <time.h>
#include "libwebsocket/libwebsockets.h"
#include "ConfigNodes.h"
#include "XMLConfig.h"
#define SERVER_INCLUDE 1
#include "../Apps/Common/mcp2515.h"
#include "Network.h"
#include "Mongoose/mongoose.h"

char *strptime(const char *s, const char *format, struct tm *tm);

/* Makro-Funktionen */
#define MAX_ACTIVEMACROS 255

struct MacroList ActiveMacros[MAX_ACTIVEMACROS] ;

int IsMakro (struct Node *This)
{
  return ((This->Type==N_ACTION)||(This->Type==N_DELAY)||(This->Type==N_TIMER)||
	  (This->Type==N_CALL)||(This->Type==N_TASK)||
	  (This->Type==N_IF)||(This->Type==N_SET)||(This->Type==N_REPEAT)) ;
} ;

void ExecuteMakro (struct Node *Makro)
{
  struct Node *This ;
  int i ;

  if (Makro==NULL) return ;

  if (Makro->Type!=N_MACRO) return ;

  for (This=Makro->Child;This!=NULL;This=This->Next) 
    if (IsMakro(This)) break ;
  
  Makro->Data.MakroStep = This ;

  for (i=0;i<MAX_ACTIVEMACROS;i++) if (ActiveMacros[i].Macro==NULL) break ;
  
  ActiveMacros[i].Macro = Makro ;
}

int MakroVergleich (int a, int b, int Vergleich)
{
  if (Vergleich==-1) return (a<b) ;
  if (Vergleich==1) return (a>b) ;
  return (a==b); 
}

int TimeCompare(struct tm *tm1, struct tm *tm2)
{
  if (tm1->tm_hour<tm2->tm_hour) return (1) ;
  if (tm1->tm_min<tm2->tm_min) return (1) ;
  if (tm1->tm_sec<=tm2->tm_sec) return (1) ;
  return (0) ;
}

void StepMakros (void)
{
  struct Node *This,*That,*Caller ;
  struct timeval Now ;
  struct tm tm;
  struct tm *LTime ;
  time_t tim ;
  int i,j ;

  for (i=0;i<MAX_ACTIVEMACROS;i++) {
    if (ActiveMacros[i].Macro!=NULL) {
      This = ActiveMacros[i].Macro  ;
      // Wurde Makro beendet?
      if (This->Data.MakroStep==NULL) { 
	ActiveMacros[i].Macro = NULL ;
	// Schauen, ob dieses Makro immer ausgefuehrt werden soll, dann wieder auf Anfang
        for (That=Haus->Child;That!=NULL;That=That->Next) if (That->Type==N_ALWAYS) break ;
	if (That!=NULL) That=That->Child ;
	for (;That!=NULL;That=That->Next) 
	  if ((That->Type==N_TASK)&&(FindNode(Haus->Child,That->Data.UnitName)==This)) break ;
	if (That!=NULL) { // Makro soll immer ausgefuehrt werden
	  ExecuteMakro(This);
	} ;
      } else {
	if (ActiveMacros[i].DelayType!=0) {
	  // Auf Makro-Ende warten
	  if (ActiveMacros[i].DelayType == W_MACRO) {
	    for (j=0;j<MAX_ACTIVEMACROS;j++) if (ActiveMacros[j].Macro==ActiveMacros[i].Delay.WaitNode) break ;
	    if (j==MAX_ACTIVEMACROS) ActiveMacros[i].DelayType=0 ; // Wird nicht mehr ausgefuehrt ;
	  } else if (ActiveMacros[i].DelayType==W_DELAY) {
	  } else if (ActiveMacros[i].DelayType==W_TIME) {
	    gettimeofday(&Now,NULL) ;
	    if (timercmp(&Now,&ActiveMacros[i].Delay.WaitTime,>)) ActiveMacros[i].DelayType=0 ;
	  } ;
	} else {
	  // Naechste Aktion durchfuehren
	  That = This->Data.MakroStep ;
	  if (That->Type==N_ACTION) {
	    SendAction (That) ;
	  } else if (That->Type==N_DELAY) {
	    gettimeofday(&Now,NULL) ;
	    timeradd(&Now,&That->Data.Time,&ActiveMacros[i].Delay.WaitTime) ;
	    ActiveMacros[i].DelayType=W_TIME ;
	  } else if (That->Type==N_TIMER) {
	    ActiveMacros[i].DelayType=W_TIME ;
	    time (&tim) ;
	    LTime = localtime(&tim); 
	    if(strptime(That->Data.Wert.UnitName,"%d.%n%b%n%Y%n%H:%M:%S",&tm)!=NULL) {
	    } else if(strptime(That->Data.Wert.UnitName,"%A%n%H:%M:%S",&tm)!=NULL) {
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      j = tm.tm_wday-LTime->tm_wday ;
	      if (j<0) j+=7 ;
	      if ((j==0)&&(TimeCompare(&tm,LTime)==1)) j=7;
	      tm.tm_mday += j ;
	    } else if(strptime(That->Data.Wert.UnitName,"%H:%M:%S",&tm)!=NULL) {
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      if (TimeCompare(&tm,LTime)==1) tm.tm_mday++ ;
	    } else if(strptime(That->Data.Wert.UnitName,"%M:%S",&tm)!=NULL) {
	      tm.tm_hour = LTime->tm_hour ;
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      if (TimeCompare(&tm,LTime)==1) tm.tm_hour++ ;
	    } else if(strptime(That->Data.Wert.UnitName,"%x%n%X",&tm)!=NULL) {
	    } else if(strptime(That->Data.Wert.UnitName,"%X",&tm)!=NULL) {
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      if (TimeCompare(&tm,LTime)==1) tm.tm_mday++ ;
	    } else {
	      fprintf (stderr,"Unbekanntes Datumsformat: %s\n",That->Data.Wert.UnitName);
	    } ;
	    tm.tm_isdst = LTime->tm_isdst ;
	    ActiveMacros[i].Delay.WaitTime.tv_sec = mktime(&tm) ; 
	    ActiveMacros[i].Delay.WaitTime.tv_usec = 0 ; 
	  } else if (That->Type==N_CALL) {
	    Caller = FindNode(Haus->Child,That->Data.UnitName) ;
	    if ((Caller!=NULL)&&(Caller->Type==N_MACRO)) {
	      ExecuteMakro(Caller) ;
	      // Und warten, bis Untermakro fertig
	      ActiveMacros[i].DelayType=W_MACRO ;
	      ActiveMacros[i].Delay.WaitNode = Caller ;
	    } ;
	  } else if (That->Type==N_TASK) {
	    Caller = FindNode(Haus->Child,That->Data.UnitName) ;
	    if ((Caller!=NULL)&&(Caller->Type==N_MACRO)) {
	      ExecuteMakro(Caller) ;
	    } ;
	  } else if ((That->Type==N_IF)||(That->Type==N_REPEAT)) {
	    Caller = FindNode(Haus->Child,That->Data.Wert.UnitName) ;
	    if ((Caller!=NULL)&&(MakroVergleich(Caller->Value,That->Data.Wert.Wert,That->Data.Wert.Vergleich))) {
	      // Vergleich ist wahr, also ausfuehren
	      for (That=That->Child;That!=NULL;That=That->Next)
		if (IsMakro(That)) break ;
	      This->Data.MakroStep = That ;
	      // zeigt schon auf neues, nicht mehr hochzaehlen
	      continue; 
	    } ;
	  } else if (That->Type==N_SET) {
	    Caller = FindNode(Haus->Child,That->Data.Wert.UnitName) ;
	    Caller->Value = That->Data.Wert.Wert ;
	  } ;
	  // Naechsten Schritt suchen
	  Caller = That->Parent ;
	  for (That=That->Next;That!=NULL;That=That->Next) 
	    if (IsMakro(That)) break ;
	  if ((That==NULL)&&(Caller->Type==N_IF)) {
	    This->Data.MakroStep = Caller->Next ;
	  } else if ((That==NULL)&&(Caller->Type==N_REPEAT)) {
	    This->Data.MakroStep = Caller ;
	  } else {
	    This->Data.MakroStep = That ;
	  } ;
	} ;
      } ;
    } ;
  } ;
}

void InitAlways(void)
{
  struct Node *That ;
  struct Node *This ;
  int i;
  
  for (That=Haus->Child;That!=NULL;That=That->Next) if (That->Type==N_ALWAYS) break ;
  if (That!=NULL) That=That->Child ;
  for (;That!=NULL;That=That->Next) 
    if (That->Type==N_TASK)
      ExecuteMakro(FindNode(Haus->Child,That->Data.UnitName)) ;

  for (That=Haus->Child;That!=NULL;That=That->Next) if (That->Type==N_STARTUP) break ;
  if (That!=NULL) That=That->Child ;
  for (;That!=NULL;That=That->Next) 
    if (That->Type==N_TASK) {
      // Nur, wenn nicht schon durch "Immer" aktiv ist eintragen
      This=FindNode(Haus->Child,That->Data.UnitName) ;
      for (i=0;i<MAX_ACTIVEMACROS;i++) if (ActiveMacros[i].Macro==This) break ;
      if (i==MAX_ACTIVEMACROS) ExecuteMakro(This) ; 
    } ;

  for (That=Haus->Child;That!=NULL;That=That->Next) if (That->Type==N_ACTIVE) break ;
  if (That!=NULL) That=That->Child ;
  for (;That!=NULL;That=That->Next) 
    if (That->Type==N_TASK) {
      // Nur, wenn nicht schon durch "Immer" aktiv ist eintragen
      This=FindNode(Haus->Child,That->Data.UnitName) ;
      for (i=0;i<MAX_ACTIVEMACROS;i++) if (ActiveMacros[i].Macro==This) break ;
      if (i==MAX_ACTIVEMACROS) ExecuteMakro(This) ; 
    } ;
}



void HandleCANRequest(void)
{
  ULONG CANID ;
  char Len ;
  unsigned char Data[8] ; 
  struct Node *This ;
  char ToLine ;
  USHORT ToAdd ;
  char FromLine ;
  USHORT FromAdd ;
  int Port ;
  int Mask,MaskIndex ;
  unsigned char websocket_buf[LWS_SEND_BUFFER_PRE_PADDING + NAMELEN*4 + LWS_SEND_BUFFER_POST_PADDING];
  char Temp[NAMELEN]; 
  char* p ;

  p = (char*)&(websocket_buf[LWS_SEND_BUFFER_PRE_PADDING]) ;

  ReceiveCANMessage(&CANID,&Len,Data) ;

  GetSourceAddress(CANID,&FromLine,&FromAdd) ;
  GetDestinationAddress(CANID,&ToLine,&ToAdd) ;
  if (ToLine!=0) {
    // Server ist nicht Ziel
    if ((Data[0]==CHANNEL_ON)||(Data[0]==CHANNEL_OFF)||(Data[0]==CHANNEL_TOGGLE)||
	(Data[0]==SHADE_UP_FULL)||(Data[0]==SHADE_DOWN_FULL)) {
      // Schaltkommando bzw. Rolladen -> Wert zwischenspeichern
      Port = Data[1] ;
      This= FindNodeAdress(Haus,ToLine,ToAdd,Port,NULL) ;
      if (This!=NULL) {
	if (Data[0]==CHANNEL_ON) { 
	  This->Value = 1 ;
	} else if ((Data[0]==CHANNEL_OFF)||(Data[0]==SHADE_UP_FULL)) {
	  This->Value = 0 ;
	} else if (Data[0]==SHADE_DOWN_FULL) {
	  This->Value = 100 ;
	} else if (Data[0]==CHANNEL_TOGGLE) {
	  This->Value = 1-This->Value ;
	} ;
      } ;
    } ;
  } else {

    if (Data[0]==START_PROG) {
      // Starte Makro auf dem Server

      Port = Data[1] ;
      This = FindNodeAdress(Haus,ToLine,ToAdd,Port,NULL) ;
      ExecuteMakro (This) ;
    } ;

    if (Data[0]==(SEND_STATUS|SUCCESSFULL_RESPONSE)) {
      // Status-Informationen empfangen, hier auswerten

      This=FindNodeAdress(Haus,FromLine,FromAdd,255,NULL) ;
      if ((This->Type==N_ONOFF)||(This->Type==N_SHADE)) {
	// Es ist ein Relais-Knoten, also die Informationen eintragen...
	Mask = 1 ;
	MaskIndex = 1 ;
	for (Port=1;Port<=10;Port++) {
	  This=FindNodeAdress(Haus,FromLine,FromAdd,Port,NULL) ;
	  if (This!=NULL) {
	    if ((This->Type==N_SHADE)&&(Port<6)) {
	      This->Value = Data[Port+2] ;
	    } else {
	      This->Value = ((Data[MaskIndex]&Mask)!=0)? 100:0 ;
	    } ;
	    Mask <<=1 ;
	    if (Port==6) {
	      MaskIndex = 2 ;
	      Mask = 1 ;
	    } ;
	    strcpy (p,"Set ") ;
	    // Hier wird ein Nebeneffekt von FullObjectName ausgenutzt: der Objektname wird an den 
	    // vorhandenen String angehaengt.
	    FullObjectName(This,p) ;
	    sprintf (Temp," %d",This->Value) ;
	    strcat (p,Temp) ;
	    libwebsockets_broadcast(&web_protocols[PROTOCOL_CONTROL],(unsigned char*)p,strlen(p)) ;
	  } ;
	} ;
      } ;
    } ;
    


    if (Data[0]==(SET_VAR|SUCCESSFULL_RESPONSE)) {
      // EEprom write handshake; send out next byte

      SendConfigByte (FromLine,FromAdd) ;
    } ;


if ((Data[0]==UPDATE_REQ)||(Data[0]==WRONG_NUMBER_RESPONSE)||
	(Data[0]==(SUCCESSFULL_RESPONSE|IDENTIFY))||
	(Data[0]==(ERROR_RESPONSE|IDENTIFY))||
	(Data[0]==IDENTIFY)||
	(Data[0]==(SUCCESSFULL_RESPONSE|SET_ADDRESS))||
	(Data[0]==(ERROR_RESPONSE|SET_ADDRESS))||
	(Data[0]==SET_ADDRESS)||
	(Data[0]==(SUCCESSFULL_RESPONSE|DATA))||
	(Data[0]==(ERROR_RESPONSE|DATA))||
	(Data[0]==DATA)||
	(Data[0]==(SUCCESSFULL_RESPONSE|START_APP))||
	(Data[0]==(ERROR_RESPONSE|START_APP))||
	(Data[0]==START_APP)) {
      // Firmware-Update; send out next data

      SendFirmwareByte(FromLine,FromAdd,Data,Len) ;
    } ;
    
  } ;
}

int HandleCommand (char *Command, char *Answer,int Socket)
{
  int Line,Add,Port ;
  int i ;
  char Makro[NAMELEN*4] ;
  char Com[NAMELEN*4] ;
  char Obj[NAMELEN*4] ;
  struct Node *This ;
  struct EEPROM EEprom ;

  
  if (strstr(Command,"Config")) {
    sscanf (Command,"Config %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      MakeConfig (Line,Add,&EEprom) ;
      WriteConfig (&EEprom) ;
      SendConfig(&EEprom) ;
      sprintf (Answer,"Update Config\n") ;
    } ;
  } ;

  if (strstr(Command,"Update")) {
    sscanf (Command,"Update %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      SendFirmware(Line,Add) ;
      sprintf (Answer,"Update Firmware\n") ;
    }
  } ;

  if (strstr(Command,"Reload")) {
    if (ReadConfig()!=0) {
      sprintf (Answer,"Error in Configuration\n") ;
      return(TRUE) ;
    } ;
    sprintf (Answer,"Updated Configuration\n") ;
  } ;

  if (strstr(Command,"Call")) {
    sscanf (Command,"Call %s",Makro) ;
    This = FindNode(Haus->Child,Makro) ;
    if (This!=NULL) {
      ExecuteMakro (This) ;
      sprintf (Answer,"Macro executed\n") ;
    } else {
      sprintf (Answer,"Macro %s not found\n",Makro) ;
    } ;
  } ;
  
  if (strstr(Command,"List")) {
    // List all active macros
    for (i=0;i<MAX_ACTIVEMACROS;i++) {
      if (ActiveMacros[i].Macro!=NULL) {
	Makro[0]='\0' ;
	FullObjectName(ActiveMacros[i].Macro,Makro) ;
	sprintf (Answer,"Macro: %s\n",Makro) ;
	send(Socket,Answer,strlen(Answer),0) ;
      } ;
    } ;
    Answer[0]='\0' ;
  } ;

  Answer[0]='\0' ;

  sscanf (Command,"%s %s",Com,Obj) ;

  if ((strcmp(Com,"An")==0)||(strcmp(Com,"On")==0)) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(CHANNEL_ON,Line,Add,Port) ;
    } ;
  } ;
  if ((strcmp(Com,"Aus")==0)||(strcmp(Com,"Off")==0)) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(CHANNEL_OFF,Line,Add,Port) ;
    } ;
  } ;
  if (strcmp(Com,"Toggle")==0) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(CHANNEL_TOGGLE,Line,Add,Port) ;
    } ;
  } ;
  if ((strcmp(Com,"Hoch")==0)||(strcmp(Com,"Up")==0)) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(SHADE_UP_FULL,Line,Add,Port) ;
    } ;
  } ;
  if ((strcmp(Com,"Runter")==0)||(strcmp(Com,"Down")==0)) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(SHADE_DOWN_FULL,Line,Add,Port) ;
    } ;
  } ;
  if ((strcmp(Com,"KurzHoch")==0)||(strcmp(Com,"ShortUp")==0)) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(SHADE_UP_SHORT,Line,Add,Port) ;
    } ;
  } ;
  if ((strcmp(Com,"KurzRunter")==0)||(strcmp(Com,"ShortDown")==0)) {
    This = FindNode(Haus->Child,Obj) ;
    if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
      SendCommand(SHADE_DOWN_SHORT,Line,Add,Port) ;
    } ;
  } ;
  
  
  return (TRUE) ;
}

static void *Handle_Webserver(enum mg_event event, struct mg_connection *conn) 
{
  // const struct mg_request_info *request_info = mg_get_request_info(conn); 

  // Everything handled in mongoose
  return NULL ;
}



/* Hauptprogramm */

struct Node *Ergebnis[MAX_ADD_PER_NODE] ;

int main(int argc, char **argv) 
{
  int Error ;

  struct mg_context *ctx ;

  char *web_options[] = {
    "listening_ports", "8080",
    "document_root", "./Webpage",
    "url_rewrite_patterns","/NodeConf=./NodeConf",
    NULL
  };

  strcpy (CAN_PORT,"13247") ;
  CAN_PORT_NUM = 13247 ;
  strcpy (WS_PORT,"13248") ;  
  WS_PORT_NUM = 13248 ;
  strcpy (COM_PORT,"13249") ;
  COM_PORT_NUM = 13249 ;
  strcpy (HTTP_PORT,"8080") ;
  HTTP_PORT_NUM = 8080 ;
  strcpy (CAN_BROADCAST,"127.0.0.1") ;

  // XML-Konfiguration lesen

  if (ReadConfig()!=0) {
    fprintf (stderr,"Fehler in der Konfiguration\n") ;
    exit(-1) ;
  } ;

  web_options[1] = HTTP_PORT ;

  // Netzwerk staren

  if (InitNetwork()!=0) {
    fprintf (stderr,"Fehler im Netzwerk\n") ;
    exit(-1) ;
  } ;

  // Makros starten

  InitAlways() ;

  // Websockets starten

  if (InitWebsocket()!=0) {
    fprintf (stderr,"Fehler in Websocket\n") ;
    exit(-1) ;
  } ;

  // Webserver starten

  if ((ctx=mg_start(&Handle_Webserver,NULL,(const char**) web_options))==NULL) {
    fprintf (stderr,"Fehler im Webserver\n") ;
    exit(-1) ;
  } ;

  fprintf (stderr,"Starte Server\n") ;

  // Endlosschleife : Netzwerkverkehr abarbeiten und Makros ausfuehren
  
  while(1) {
    if (CheckNetwork(&Error,10)) {
      // Netzwerk empfangen
      HandleCANRequest() ;
    }

    
    libwebsocket_service(web_context,0);    
    // Alle regelmaessigen Tasks abarbeiten
    // Makros abarbeiten
    StepMakros() ;
  } ;

  CloseNetwork () ;
}  /* End of main */

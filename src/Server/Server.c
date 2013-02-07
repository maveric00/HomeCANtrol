#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define _XOPEN_SOURCE
#include <time.h>
#include "libwebsocket/libwebsockets.h"
#include "ConfigNodes.h"
#include "XMLConfig.h"
#define SERVER_INCLUDE 1
#include "../Apps/Common/mcp2515.h"
#include "Network.h"
#include "Mongoose/mongoose.h"
#include "RelUDP.h"

char *strptime(const char *s, const char *format, struct tm *tm);

/* Makro-Funktionen */
#define MAX_ACTIVEMACROS 255

struct MacroList ActiveMacros[MAX_ACTIVEMACROS] ;
struct SeqList *ActiveSeq[MAX_ACTIVEMACROS] ;

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

  for (i=0;i<MAX_ACTIVEMACROS-1;i++) if (ActiveMacros[i].Macro==NULL) break ;
  
  ActiveMacros[i].Macro = Makro ;
}

void ExecuteSeq (struct Node *Action)
{
  struct SeqList *This ;
  int i ;
  
  for (This=Sequences;This!=NULL;This=This->Next)
    if (strcmp(This->Name,Action->Data.Aktion.Sequence)==0) break ;
  
  if (This==NULL) {
    fprintf (stderr,"Unknown sequence %s\n",Action->Data.Aktion.Sequence) ;
    return ;
  } ;

  if (This->Current!=NULL) {
    fprintf (stderr,"Sequence %s already running\n",This->Name) ;
    return ;
  } ;

  This->Current = This->First ;
  This->Action = Action ;
  for (i=0;i<MAX_ACTIVEMACROS-1;i++) if (ActiveSeq[i]==NULL) break ;
  ActiveSeq[i]=This ;
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

void StepSeq (void)
{
  int i,j ;
  ULONG CANID ;
  unsigned char Data[8]; 
  char Len ;
  int Linie,Knoten,Port ;  
  struct Sequence *This ;
  struct Sequence *Current ;

  for (i=0;i<MAX_ACTIVEMACROS;i++) {
    if (ActiveSeq[i]==NULL) continue ;
    if (ActiveSeq[i]->Current==NULL) {
      ActiveSeq[i]=NULL ;
      continue ;
    } ;
    Current = ActiveSeq[i]->Current ;
    switch (Current->Command) {
    case S_NOP:
    default:
      ActiveSeq[i]->Current = Current->Next ;
      break ;
    case S_GOTO:
      for (This=ActiveSeq[i]->First;This!=NULL;This=This->Next) if (This->LineNumber==Current->Para) break ;
      if (This==NULL) {
	fprintf (stderr,"Line number %d of Goto not found in sequence %s\n",Current->Para,ActiveSeq[i]->Name) ;
      } ;
      ActiveSeq[i]->Current = This ;
      break ;
    case S_DELAY:
      if (Current->CurrVal==0) {
	Current->CurrVal=Current->Para*5+1 ;
      } else if (Current->CurrVal==1) {
	ActiveSeq[i]->Current = Current->Next ;
      } else {
	Current->CurrVal-- ;
      } ;
      break ;
    case S_DIM:
      // Output Elements
      if (GetNodeAdress(ActiveSeq[i]->Action->Data.Aktion.Unit,&Linie,&Knoten,&Port)==0) {
	CANID = BuildCANId(0,0,0,2,Linie,Knoten,0) ;
	Data[0] = LOAD_LED ;
	for (j=0;j<Current->DataLen;j+=3) {
	  Data[1] = j/3 ;
	  Data[2] = Current->Data[j]; 
	  Data[3] = Current->Data[j+1]; 
	  Data[4] = Current->Data[j+2]; 
	  Len=5 ;
	  SendCANMessage(CANID,Len,Data) ;
	} ;
	Data[0] = OUT_LED ;
	Data[1] = Current->Para ;
	Len = 2 ;
	SendCANMessage(CANID,Len,Data) ;
      }; 
      ActiveSeq[i]->Current = ActiveSeq[i]->Current->Next ;
      break ;
    } ;
  };
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

  ReceiveCANMessage(&CANID,&Len,Data) ;
  
  if (Len==0) return; // Nothing has been received 

  p = (char*)&(websocket_buf[LWS_SEND_BUFFER_PRE_PADDING]) ;

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
  static struct Node *MenuCurrent = NULL ;
  
  if (MenuCurrent==NULL) MenuCurrent = Haus ;

  Obj[0] = '\0' ;
  Com[0] = '\0' ;

  sscanf(Command,"%s %s",Com,Obj) ;
  
  // Send current location and available Childs...
  
  if ((strcmp(Com,"Help")==0)||(strcmp(Com,"Hilfe")==0)) {
    sprintf (Answer,"Available Commands:\r\n"
	     "Help/Hilfe: This help\r\n"
	     "Config (Line)(Adress):         Configure node with parameters\r\n"
	     "Update (Line)(Adress):         Update node with firmware stored in NodeConf\r\n"
	     "Reload:                        Reload CANControl configuration\r\n"
	     "List:                          List all running macros\r\n"
	     "Exit:                          Exit this communication\r\n"
	     "An/On [Object]:                Switch on Object or current location\r\n"
	     "Aus/Off [Object]:              Switch off object or current location\r\n"
	     "Toggle [Object]:               Toggle object or current location\r\n"
	     "Hoch/Up [Object]:              Open shades completely\r\n"
             "Runter/Down [Object]:          Close shades completely\r\n"
	     "KurzHoch/ShortUp [Object]:     Open shades slightly\r\n"
	     "KurzRunter/ShortDown [Object]: Close shades slightly\r\n\r\n"
	     "[Number]:                      Select child object\r\n"
	     "Parent:                        Select parent object\r\n"
	     "Top:                           Select top level\r\n"); 
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Command: ") ;
    return(TRUE) ;
  }
  
  if (strcmp(Com,"Parent")==0) {
    MenuCurrent = MenuCurrent->Parent!=NULL?MenuCurrent->Parent:MenuCurrent ;
  }
  
  if (strcmp(Com,"Top")==0) {
    MenuCurrent = Haus ;
  }
  
  i = 0 ;
  sscanf (Com,"%d",&i) ;
  if (i==99) {
    if (MenuCurrent->Parent!=NULL) MenuCurrent=MenuCurrent->Parent ;
  } else if (i>0) {
    for (This=MenuCurrent->Child;(i>1)&&(This!=NULL);This=This->Next,i--) ;
    if (This==NULL) {
      sprintf (Answer,"Selection out of range\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
    } else {
      MenuCurrent = This ;
    }; 
  } ;

  Makro[0]='\0' ;  
  if (MenuCurrent==Haus) {
    strcpy (Makro,"Top") ;
  } else {
    FullObjectName(MenuCurrent,Makro) ;
  } ;
  sprintf (Answer,"Location: %s\r\n",Makro) ;
  send(Socket,Answer,strlen(Answer),0) ;
  sprintf (Answer,"Available childs:\r\n") ;
  send(Socket,Answer,strlen(Answer),0) ;
  for (This = MenuCurrent->Child,i=1;This!=NULL;This=This->Next,i++) {
    Makro[0]='\0' ;
    FullObjectName(This,Makro) ;
    sprintf (Answer,"%d: %s\r\n",i,Makro) ;
    send(Socket,Answer,strlen(Answer),0) ;
  } ;
  sprintf (Answer,"99: One level up\r\n") ;
  send(Socket,Answer,strlen(Answer),0) ;
  
  if (strlen(Com)==0) {
    sprintf (Answer,"Command: ") ;
    return (TRUE) ;  
  } ;
  
  Answer[0]='\0' ;
  
  
  if (strcmp(Com,"Config")==0) {
    sscanf (Command,"Config %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      MakeConfig (Line,Add,&EEprom) ;
      WriteConfig (&EEprom) ;
      SendConfig(&EEprom) ;
      sprintf (Answer,"Update Config\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      sprintf (Answer,"Command: ") ;
      return (TRUE); 
    } ;
  } ;
  
  if (strcmp(Com,"Update")==0) {
    printf ("Update command\n") ;
    sscanf (Command,"Update %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      printf ("Line %d, Add %d\n",Line,Add) ;
      SendFirmware(Line,Add) ;
      sprintf (Answer,"Update Firmware\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      sprintf (Answer,"Command: ") ;
      return (TRUE); 
    } ;
    printf ("No Add read\n") ;
  } ;

  if (strcmp(Com,"TC")==0) {
    printf ("Toggle command\n") ;
    sscanf (Command,"TC %d %d %d",&Line,&Add,&Port) ;
    if ((Line!=0)&&(Add!=0)) {
      SendCommand(CHANNEL_TOGGLE,Line,Add,Port) ;
      return (TRUE); 
    } ;
    printf ("No Add read\n") ;
  } ;
  
  if (strcmp(Com,"Reload")==0) {
    if (ReadConfig()!=0) {
      sprintf (Answer,"Error in Configuration\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      sprintf (Answer,"Command: ") ;
      return(TRUE) ;
    } ;
    sprintf (Answer,"Updated Configuration\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Command: ") ;
    return (TRUE); 
  } ;
  
  if (strcmp(Com,"List")==0) {
    // List all active macros
    for (i=0;i<MAX_ACTIVEMACROS;i++) {
      if (ActiveMacros[i].Macro!=NULL) {
	Makro[0]='\0' ;
	FullObjectName(ActiveMacros[i].Macro,Makro) ;
	sprintf (Answer,"Macro: %s\r\n",Makro) ;
	send(Socket,Answer,strlen(Answer),0) ;
      } ;
    } ;
    sprintf (Answer,"Command: ") ;
    return (TRUE); 
  } ;
  
  if (strcmp(Com,"Exit")==0) {
    return (FALSE) ;
  } ;

  if (strlen(Obj)!=0) {
    This = FindNode(Haus->Child,Obj) ;  
    if (This==NULL) {
      strcpy(Answer,"Object not found\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      sprintf (Answer,"Command: ") ;
      return(TRUE) ;
    } ;
  } else {
    This = MenuCurrent ;
  } ;
  
  if ((strcmp(Com,"Action")==0)||(strcmp(Com,"Aktion")==0)) {  
    ExecuteMakro (This) ;
    sprintf (Answer,"Macro %s executed\r\n",This->Name) ;
  } ;
  
  if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
    sprintf (Answer,"%s: %s\r\n",Com,This->Name) ;
    if ((strcmp(Com,"An")==0)||(strcmp(Com,"On")==0)) {
      SendCommand(CHANNEL_ON,Line,Add,Port) ;
    } else if ((strcmp(Com,"Aus")==0)||(strcmp(Com,"Off")==0)) {
      SendCommand(CHANNEL_OFF,Line,Add,Port) ;
    } else if (strcmp(Com,"Toggle")==0) {
      SendCommand(CHANNEL_TOGGLE,Line,Add,Port) ;
    } else if ((strcmp(Com,"Hoch")==0)||(strcmp(Com,"Up")==0)) {
      SendCommand(SHADE_UP_FULL,Line,Add,Port) ;
    } else if ((strcmp(Com,"Runter")==0)||(strcmp(Com,"Down")==0)) {
      SendCommand(SHADE_DOWN_FULL,Line,Add,Port) ;
    } else if ((strcmp(Com,"KurzHoch")==0)||(strcmp(Com,"ShortUp")==0)) {
      SendCommand(SHADE_UP_SHORT,Line,Add,Port) ;
    } else if ((strcmp(Com,"KurzRunter")==0)||(strcmp(Com,"ShortDown")==0)) {
      SendCommand(SHADE_DOWN_SHORT,Line,Add,Port) ;
    } else {
      sprintf (Answer,"Unknown command: %s\r\n",Com) ;
    }
  } else {
    // sprintf (Answer,"%s does not have an adress\r\n",Obj) ;
  } ;    

  send(Socket,Answer,strlen(Answer),0) ;
  sprintf (Answer,"Command: ") ;
  return (TRUE) ;
}

static void *Handle_Webserver(enum mg_event event, struct mg_connection *conn) 
{
  const struct mg_request_info *ri = mg_get_request_info(conn); 
  char data[NAMELEN*4] ;
  char Com[NAMELEN*4] ;
  char Obj[NAMELEN*4] ;
  struct Node *This ;
  int Line,Add,Port ;
  int i,j ;
  
  if (event==MG_NEW_REQUEST) {
    printf ("%s\n",ri->uri) ;
    if (!strcmp(ri->uri, "/Action")) {      
      // User has submitted a form, show submitted data and a variable value 
      if (ri->query_string==NULL) return (NULL) ;
      
      for (i=0,j=0;(ri->query_string[i]!='\0')&&(ri->query_string[i]!='.')&&(j<NAMELEN*4-1);i++,j++) {
	if (ri->query_string[i]=='+') {
	  data[j]=' ' ;
	} else if (ri->query_string[i]=='%') {
	  data[j]='?' ;
	  if ((ri->query_string[i+1]=='2')&&(toupper((int)ri->query_string[i+2])=='F')) data[j]='/' ;
	  if ((ri->query_string[i+1]=='3')&&(toupper((int)ri->query_string[i+2])=='A')) data[j]=':' ;
	  i+=2 ;
	} else {
	  data[j]=ri->query_string[i] ;
	} ;
      }
      data[j] = '\0' ;
      Obj[0] = '\0' ;
      Com[0] = '\0' ;
      printf ("Data: %s\n",data) ;
      
      sscanf(data,"%s %s",Com,Obj) ;
      
      printf ("%s %s\n",Com,Obj) ;

      if ((strlen(Com)==0)||strlen(Obj)==0) return NULL ;

      This = FindNode(Haus->Child,Obj) ;  

      if ((strcmp(Com,"Action")==0)||(strcmp(Com,"Aktion")==0)) {  
	ExecuteMakro (This) ;
      } ;
      
      if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
	if ((strcmp(Com,"An")==0)||(strcmp(Com,"On")==0)) {
	  SendCommand(CHANNEL_ON,Line,Add,Port) ;
	} else if ((strcmp(Com,"Aus")==0)||(strcmp(Com,"Off")==0)) {
	  SendCommand(CHANNEL_OFF,Line,Add,Port) ;
	} else if (strcmp(Com,"Toggle")==0) {
	  SendCommand(CHANNEL_TOGGLE,Line,Add,Port) ;
	} else if ((strcmp(Com,"Hoch")==0)||(strcmp(Com,"Up")==0)) {
	  SendCommand(SHADE_UP_FULL,Line,Add,Port) ;
	} else if ((strcmp(Com,"Runter")==0)||(strcmp(Com,"Down")==0)) {
	  SendCommand(SHADE_DOWN_FULL,Line,Add,Port) ;
	} else if ((strcmp(Com,"KurzHoch")==0)||(strcmp(Com,"ShortUp")==0)) {
	  SendCommand(SHADE_UP_SHORT,Line,Add,Port) ;
	} else if ((strcmp(Com,"KurzRunter")==0)||(strcmp(Com,"ShortDown")==0)) {
	  SendCommand(SHADE_DOWN_SHORT,Line,Add,Port) ;
	}
      } ;    
      
      // Give return command
      
      mg_printf (conn,"HTTP/1.0 200 OK\r\n"
		 "Content-type: text/html\r\n\r\n"
		 "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\r\n"
		 "<html><head><title>Action</title></head><body>\r\n"
		 "<script type=\"text/javascript\">\r\n"
		 "history.back();\r\n"
		 "</script>"
		 "</html>") ;      
      return ""; 
    }  ;

  } ;

  // Let mongoose handle everything else...
  return NULL; 

}



/* Hauptprogramm */

struct Node *Ergebnis[MAX_ADD_PER_NODE] ;

int main(int argc, char **argv) 
{
  int Error ;

  struct mg_context *ctx ;
  struct timeval Now,Wait,Old ;

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
    //    exit(-1) ;
  } ;

  fprintf (stderr,"Starte Server\n") ;

  Wait.tv_sec=0;
  Wait.tv_usec=9000;
  gettimeofday(&Old,NULL) ;

  // Endlosschleife : Netzwerkverkehr abarbeiten und Makros ausfuehren
  
  while(1) {
    if (CheckNetwork(&Error,10)) {
      // Netzwerk empfangen
      HandleCANRequest() ;
    } ;
    
    gettimeofday(&Now,NULL) ;
    
    if (timercmp(&Now,&Old,>)) { // if UDP packets are coming in very fast, do not step macros for each
      
      relworkqueue () ;
      
      libwebsocket_service(web_context,0);    
      // Alle regelmaessigen Tasks abarbeiten
      // Makros abarbeiten
      StepMakros() ;
      StepSeq() ;
      timeradd(&Now,&Wait,&Old) ;
    } ;
  } ;

  CloseNetwork () ; // Will never be reached...
}  /* End of main */

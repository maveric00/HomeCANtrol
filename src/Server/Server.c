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

char TimeString[40] ;
char CommandNum[40]; 
FILE *logfd ;
int Verbose=0 ;
int NoTime=0 ;

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

char *LogTime(void) 
{
  struct timeval CurrTime ;
  struct tm *LTime ;

  gettimeofday(&CurrTime,NULL) ;
  LTime = localtime(&(CurrTime.tv_sec)) ;
  sprintf (TimeString,"%02d.%02d, %02d:%02d:%02d.%03d",LTime->tm_mday,LTime->tm_mon,LTime->tm_hour,LTime->tm_min,LTime->tm_sec,(int)(CurrTime.tv_usec/1000)) ;
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

int IsMakro (struct Node *This)
{
  return ((This->Type==N_ACTION)||(This->Type==N_DELAY)||(This->Type==N_TIMER)||
	  (This->Type==N_CALL)||(This->Type==N_TASK)||
	  (This->Type==N_IF)||(This->Type==N_SET)||(This->Type==N_REPEAT)||
	  (This->Type==N_ELSE)||(This->Type==N_WAITFOR)) ;
} ;

void ExecuteMakro (struct Node *Makro)
{
  struct Node *This ;
  int i ;

  if (Makro==NULL) return ;

  if ((Makro->Type!=N_MACRO)&&(Makro->Type!=N_REACT)) return ;

  for (This=Makro->Child;This!=NULL;This=This->Next) 
    if (IsMakro(This)) break ;
  
  Makro->Data.MakroStep = This ;

  for (i=0;i<MAX_ACTIVEMACROS-1;i++) if (ActiveMacros[i].Macro==NULL) break ;
  
  ActiveMacros[i].Macro = Makro ;
  ActiveMacros[i].DelayType= 0 ;
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

int TimeCompare(struct tm *tm1, struct tm *tm2)
{
  if (tm1->tm_mday>tm2->tm_mday) return (0) ;
  if (tm1->tm_mday<tm2->tm_mday) return (1) ;
  if (tm1->tm_hour>tm2->tm_hour) return (0) ;
  if (tm1->tm_hour<tm2->tm_hour) return (1) ;
  if (tm1->tm_min>tm2->tm_min) return (0) ;
  if (tm1->tm_min<tm2->tm_min) return (1) ;
  if (tm1->tm_sec<=tm2->tm_sec) return (1) ;
  return (0) ;
}

void StepMakros (void)
{
  struct Node *This,*That,*Caller ;
  struct timeval CurrTime ;
  struct tm tm;
  struct tm *LTime ;
  time_t tim ;
  int i,j ;

  for (i=0;i<MAX_ACTIVEMACROS;i++) {
    if (ActiveMacros[i].Macro!=NULL) {
      This = ActiveMacros[i].Macro  ;
      // Wurde Makro beendet?
      if ((This->Data.MakroStep==NULL)&&(ActiveMacros[i].DelayType==0)) { 
	// Makro wurde im letzten Schritt beendet und ein eventuelles Delay ist abgearbeitet
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
	  } else if ((ActiveMacros[i].DelayType==W_DELAY)||(ActiveMacros[i].DelayType==W_TIME)) {
	    gettimeofday(&CurrTime,NULL) ;
	    if (timercmp(&CurrTime,&ActiveMacros[i].Delay.WaitTime,>)) {
	      ActiveMacros[i].DelayType=0 ;
	    } ;
	  } else if (ActiveMacros[i].DelayType==W_VALUE) {
	    That = ActiveMacros[i].Delay.WaitNode ;
	    if (CalcValue(That->Data.Wert.Wert)) {
	      ActiveMacros[i].DelayType=0 ;
	    }; 
	  };
	} else {
	  That = This->Data.MakroStep ;
	  // Naechste Aktion durchfuehren
	  if (That->Type==N_ACTION) {
	    SendAction (That) ;
	  } else if (That->Type==N_DELAY) {
	    gettimeofday(&CurrTime,NULL) ;
	    timeradd(&CurrTime,&That->Data.Time,&ActiveMacros[i].Delay.WaitTime) ;
	    ActiveMacros[i].DelayType=W_TIME ;
	  } else if (That->Type==N_WAITFOR) {
	    ActiveMacros[i].Delay.WaitNode=That ;
	    ActiveMacros[i].DelayType=W_VALUE ;
	  } else if (That->Type==N_TIMER) {
	    ActiveMacros[i].DelayType=W_TIME ;
	    time (&tim) ;
	    LTime = localtime(&tim); 
	    if(strptime(That->Data.Wert.Wert,"%d.%n%b%n%Y%n%H:%M:%S",&tm)!=NULL) {
	    } else if(strptime(That->Data.Wert.Wert,"%A%n%H:%M:%S",&tm)!=NULL) {
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      j = tm.tm_wday-LTime->tm_wday ;
	      if (j<0) j+=7 ;
	      if ((j==0)&&(TimeCompare(&tm,LTime)==1)) j=7;
	      tm.tm_mday += j ;
	    } else if(strptime(That->Data.Wert.Wert,"%H:%M:%S",&tm)!=NULL) {
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      if ((That->Value==0)&&(TimeCompare(&tm,LTime)==1)) tm.tm_mday++ ;
	    } else if(strptime(That->Data.Wert.Wert,"%M:%S",&tm)!=NULL) {
	      tm.tm_hour = LTime->tm_hour ;
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      if (TimeCompare(&tm,LTime)==1) tm.tm_hour++ ;
	    } else if(strptime(That->Data.Wert.Wert,"%x%n%X",&tm)!=NULL) {
	    } else if(strptime(That->Data.Wert.Wert,"%X",&tm)!=NULL) {
	      tm.tm_mday = LTime->tm_mday ;
	      tm.tm_mon = LTime->tm_mon ;
	      tm.tm_year = LTime->tm_year ;
	      if (TimeCompare(&tm,LTime)==1) tm.tm_mday++ ;
	    } else {
	      fprintf (stderr,"Unbekanntes Datumsformat: %s\n",That->Data.Wert.Wert);
	    } ;
	    tm.tm_isdst = LTime->tm_isdst ;
	    if (That->Value==1) { // Relativ (vor) zum Aufgang 
	      tm.tm_hour = SunRise.tm_hour-tm.tm_hour ;
	      tm.tm_min = SunRise.tm_min-tm.tm_min ;
	      for (;mktime(&tm)<(tim+600);) tm.tm_mday++ ;
	    } else if (That->Value==2) { // Relativ (nach) zum Aufgang 
	      tm.tm_hour = SunRise.tm_hour+tm.tm_hour ;
	      tm.tm_min = SunRise.tm_min+tm.tm_min ;
	      for (;mktime(&tm)<(tim+600);) tm.tm_mday++ ;
	    } else if (That->Value==3) { // Relativ (vor) zum Untergang 
	      tm.tm_hour = SunSet.tm_hour-tm.tm_hour ;
	      tm.tm_min = SunSet.tm_min-tm.tm_min ;
	      for (;mktime(&tm)<(tim+600);) tm.tm_mday++ ;
	    } else if (That->Value==4) { // Relativ (nach) zum Untergang
	      tm.tm_hour = SunSet.tm_hour+tm.tm_hour ;
	      tm.tm_min = SunSet.tm_min+tm.tm_min ;
	      for (;mktime(&tm)<(tim+600);) tm.tm_mday++ ;
	    } ;
	    ActiveMacros[i].Delay.WaitTime.tv_sec = mktime(&tm) ; 
	    ActiveMacros[i].Delay.WaitTime.tv_usec = 0 ; 
	  } else if (That->Type==N_CALL) {
	    Caller = FindNode(Haus->Child,That->Data.UnitName) ;
	    if ((Caller!=NULL)&&(Caller->Type==N_MACRO)) {
	      ExecuteMakro(Caller) ;
	      // Und warten, bis Untermakro fertig
	      ActiveMacros[i].DelayType=W_MACRO ;
	      ActiveMacros[i].Delay.WaitNode = Caller ;
	    } else {
	      fprintf (stderr,"Macro %s not found / not a macro\n",That->Data.UnitName) ;
	    };
	  } else if (That->Type==N_TASK) {
	    Caller = FindNode(Haus->Child,That->Data.UnitName) ;
	    if ((Caller!=NULL)&&(Caller->Type==N_MACRO)) {
	      ExecuteMakro(Caller) ;
	    } else {
	      fprintf (stderr,"Macro %s not found / not a macro\n",That->Data.UnitName) ;
	    } ;
	  } else if ((That->Type==N_IF)||(That->Type==N_REPEAT)) {
	    if (CalcValue(That->Data.Wert.Wert)) {
	      // Vergleich ist wahr, also ausfuehren
	      for (That=That->Child;That!=NULL;That=That->Next)
		if (IsMakro(That)) break ;
	      This->Data.MakroStep = That ;
	      // zeigt schon auf neues, nicht mehr hochzaehlen
	      continue; 
	    } else {
	      // Pruefen, ob ein Else-Zweig vorhanden ist...
	      Caller = That ;
	      for (That=That->Child;That!=NULL;That=That->Next) 
		if (That->Type==N_ELSE) break ;
	      if ((That!=NULL)&&(That->Next!=NULL)) {
		This->Data.MakroStep = That->Next ;
	      } else {
		That = Caller ;
	      } ;
	    } ;
	  } else if (That->Type==N_SET) {
	    Caller = FindNode(Haus->Child,That->Data.Wert.UnitName) ;
	    if (Caller==NULL) { 
	      fprintf (stderr,"Unknown variable: %s\n",That->Data.Wert.UnitName) ;
	    } else {
	      Caller->Value = CalcValue(That->Data.Wert.Wert) ;
	    } ;
	  } else if (That->Type==N_ELSE) {
	    /* Bis zum Ende des Else-Zweiges durchgehen (bis Ende des IF-Blocks) */
	    for (;That->Next!=NULL;That=That->Next) ;
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
  int i,j,k,LEDNum ;
  ULONG CANID ;
  unsigned char Data[8]; 
  unsigned char Vals[MAX_WSLEDS*3] ;
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
    case S_COUNTUP:
      ActiveSeq[i]->Counter = 1 ;
      ActiveSeq[i]->UpDown = 0 ;
      ActiveSeq[i]->LineNumber = Current->LineNumber ;
      ActiveSeq[i]->Current = Current->Next ;
      break ;
    case S_COUNTDOWN:
      ActiveSeq[i]->Counter = Current->Para ;
      ActiveSeq[i]->UpDown = 1 ;
      ActiveSeq[i]->LineNumber = Current->LineNumber ;
      ActiveSeq[i]->Current = Current->Next ;
      break ;
    case S_COUNTEND:
      for (This=ActiveSeq[i]->First;This!=NULL;This=This->Next) if (This->LineNumber==ActiveSeq[i]->LineNumber) break ;
      if (ActiveSeq[i]->UpDown==0) {
	ActiveSeq[i]->Counter+=Current->Para;
	if (ActiveSeq[i]->Counter<=This->Para) {
	  ActiveSeq[i]->Current = This->Next ;
	} else {
	  ActiveSeq[i]->Current = Current->Next ;
	} ;
      } else {
	ActiveSeq[i]->Counter-=Current->Para;
	if (ActiveSeq[i]->Counter>0) {
	  ActiveSeq[i]->Current = This->Next ;
	} else {
	  ActiveSeq[i]->Current = Current->Next ;
	} ;
      } ;
      break ;
    case S_GOTO:
      for (This=ActiveSeq[i]->First;This!=NULL;This=This->Next) if (This->LineNumber==Current->Para) break ;
      if (This==NULL) {
	fprintf (stderr,"Line number %d of Goto not found in sequence %s\n",Current->Para,ActiveSeq[i]->Name) ;
      } ;
      ActiveSeq[i]->Current = This ;
      break ;
    case S_SETVAR:
      if ((ActiveSeq[i]->Current->Para<10)&&(ActiveSeq[i]->Current->GlobalVar!=NULL)) {
	ActiveSeq[i]->Vars[ActiveSeq[i]->Current->Para] = ActiveSeq[i]->Current->GlobalVar->Value ;
      } ;
      ActiveSeq[i]->Current = ActiveSeq[i]->Current->Next ;
      break ;
    case S_DELAY:
      if (Current->CurrVal==0) {
	Current->CurrVal=Current->Para*10+1 ; // Para ist Delay in 0,1 Sekunden, diese Routine wird ungefaehr
	                                      // alle 10 ms aufgerufen -> Faktor 10
      } else if (Current->CurrVal==1) {
	Current->CurrVal=0 ;
	ActiveSeq[i]->Current = Current->Next ;
      } else {
	Current->CurrVal-- ;
      } ;
      break ;
    case S_DIM:
    case S_DIMH:
      // Output Elements
      if (GetNodeAdress(ActiveSeq[i]->Action->Data.Aktion.Unit,&Linie,&Knoten,&Port)==0) {
	CANID = BuildCANId(0,0,0,2,Linie,Knoten,0) ;
	Data[0] = LOAD_LED ;
	ActiveSeq[i]->DataLen = Current->DataLen ;
	for (j=0;j<Current->DataLen;j++) 
	  if (Current->Var[j]<10) Current->Data[j]=ActiveSeq[i]->Vars[Current->Var[j]] ;

	// HSV-Werte nach RGB wandeln
	for (k=0;k<Current->DataLen;k+=3) {
	  Vals[k]=Current->Data[k] ;
	  Vals[k+1]=Current->Data[k+1] ;
	  Vals[k+2]=Current->Data[k+2] ;
	  if (Current->Command==S_DIMH) 
	    hsv_to_rgb (Vals[k],Vals[k+1],Vals[k+2],&(Vals[k]),&(Vals[k+1]),&(Vals[k+2])) ;
	} ;
	
	for (j=0;j<Current->DataLen;j+=3) {
	  Data[1] = j/3 ;
	  Data[2] = ActiveSeq[i]->Data[j] = Vals[j]; 
	  Data[3] = ActiveSeq[i]->Data[j+1] = Vals[j+1]; 
	  Data[4] = ActiveSeq[i]->Data[j+2] = Vals[j+2]; 
	  Data[5] = Current->Para ;
	  Len=6 ;
	  SendCANMessage(CANID,Len,Data) ;
	} ;
      }; 
      ActiveSeq[i]->Current = ActiveSeq[i]->Current->Next ;
      break ;
    case S_SINGLE:
    case S_SINGLEH:
      // Update Data
      if (Current->LED<0) {
	if (Current->LED==-10) {
	  LEDNum = j = (ActiveSeq[i]->Counter-1)*3 ;
	} else {
	  LEDNum = j = (ActiveSeq[i]->Vars[-(Current->LED)]-1)*3 ;
	} ;
      } else {
	LEDNum = j = (Current->LED-1)*3 ;
      } ;
      
      for (j=0;j<Current->DataLen;j++) 
	if (Current->Var[j]<10) Current->Data[j]=ActiveSeq[i]->Vars[Current->Var[j]] ;
      

      // HSV-Werte nach RGB wandeln

      for (k=0;k<Current->DataLen;k+=3) {
	Vals[k]=Current->Data[k] ;
	Vals[k+1]=Current->Data[k+1] ;
	Vals[k+2]=Current->Data[k+2] ;
	if (Current->Command==S_SINGLEH) 
	  hsv_to_rgb (Vals[k],Vals[k+1],Vals[k+2],&(Vals[k]),&(Vals[k+1]),&(Vals[k+2])) ;
      } ;


      for (k=0;(k<Current->DataLen)&&(j<MAX_WSLEDS*3);k++,j++) {
	ActiveSeq[i]->Data[j] = Current->Data[k] ;
      } ;

      if (j>ActiveSeq[i]->DataLen) ActiveSeq[i]->DataLen = j ;

      
      // Output Elements
      if (GetNodeAdress(ActiveSeq[i]->Action->Data.Aktion.Unit,&Linie,&Knoten,&Port)==0) {
	CANID = BuildCANId(0,0,0,2,Linie,Knoten,0) ;
	Data[0] = LOAD_LED ;
	for (j=0;j<Current->DataLen;j+=3) {
	  Data[1] = LEDNum/3+j/3 ;
	  Data[2] = Vals[j]; 
	  Data[3] = Vals[j+1]; 
	  Data[4] = Vals[j+2]; 
	  Data[5] = Current->Para ;
	  Len=6 ;
	  SendCANMessage(CANID,Len,Data) ;
	} ;
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

void EveryDay(void)
{
  struct Node *That ;
  struct Node *This ;
  int i;

  CalcSun () ;

  for (That=Haus->Child;That!=NULL;That=That->Next) if (That->Type==N_DAILY) break ;
  if (That!=NULL) That=That->Child ;
  for (;That!=NULL;That=That->Next) 
    if (That->Type==N_TASK) {
      // Nur, wenn nicht schon aktiv ist eintragen
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
  int i ;
  struct NodeList *NL ;

  ReceiveCANMessage(&CANID,&Len,Data) ;

  if (Len==0) return; // Nothing has been received 

  GetSourceAddress(CANID,&FromLine,&FromAdd) ;
  GetDestinationAddress(CANID,&ToLine,&ToAdd) ;

  if ((Verbose==1)&&((NoTime==0)||((Data[0]!=7)&&(Data[0]!=16)))) {
    fprintf (logfd,"%s: ",LogTime()) ;
    fprintf (logfd," Received From: %d %d To: %d %d, Len: %d , Command: %s ",FromLine, FromAdd,
	     ToLine, ToAdd, Len, ToCommand(Data[0])) ;
    for (i=1;i<Len;i++) fprintf (logfd,"%02x ",Data[i]) ;
    fprintf (logfd,"\n") ;
  } ;

  p = (char*)&(websocket_buf[LWS_SEND_BUFFER_PRE_PADDING]) ;

  
  // Nachsehen, ob auf diese Kommando eine Reaktion folgen soll
  for (NL=Reactions; NL!=NULL;NL=NL->Next) {
    if (((NL->Node->Data.Reaction.From.Linie&NL->Node->Data.Reaction.FromMask.Linie)==
	 (FromLine&NL->Node->Data.Reaction.FromMask.Linie)) &&
	((NL->Node->Data.Reaction.From.Knoten&NL->Node->Data.Reaction.FromMask.Knoten)==
	 (FromAdd&NL->Node->Data.Reaction.FromMask.Knoten)) &&
	((NL->Node->Data.Reaction.To.Linie&NL->Node->Data.Reaction.ToMask.Linie)==
	 (ToLine&NL->Node->Data.Reaction.ToMask.Linie)) &&
	((NL->Node->Data.Reaction.To.Knoten&NL->Node->Data.Reaction.ToMask.Knoten)==
	 (ToAdd&NL->Node->Data.Reaction.ToMask.Knoten))) {
      // Adressen stimmen ueberein, nun noch den Datenbereich ueberpruefen
      for (i=0;i<8;i++) {
	if (((NL->Node->Data.Reaction.Data[i]&NL->Node->Data.Reaction.DataMask[i])!=
	     (Data[i]&NL->Node->Data.Reaction.DataMask[i]))) break ;
      } ;
      if (i==8) { // alles war gleich
	ExecuteMakro (NL->Node) ;
      } ;
    } ;
  } ;
  
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
    } else if (Data[0]==(SEND_STATUS|SUCCESSFULL_RESPONSE)) {
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
    } else if (Data[0]==(SET_VAR|SUCCESSFULL_RESPONSE)) {
      // EEprom write handshake; send out next byte

      SendConfigByte (FromLine,FromAdd) ;
    } else if (Data[0]==(READ_VAR|SUCCESSFULL_RESPONSE)) {
      // EEprom write handshake; send out next byte

      ReadConfigByte (FromLine,FromAdd,Data[3]) ;
    } else if ((Data[0]==UPDATE_REQ)||(Data[0]==WRONG_NUMBER_RESPONSE)||
	((Data[0]&COMMAND_MASK)==IDENTIFY)||
	((Data[0]&COMMAND_MASK)==SET_ADDRESS)||
	((Data[0]&COMMAND_MASK)==DATA)||
	((Data[0]&COMMAND_MASK)==START_APP)) {
      // Firmware-Update; send out next data
      
      SendFirmwareByte(FromLine,FromAdd,Data,Len) ;
    } else if ((Data[0]==ANALOG_VAL)||(Data[0]==LIGHT_VAL)) { // Sensorwerte abspeichern
      This = FindNodeAdress(Haus,FromLine,FromAdd,1,NULL) ;
      if (This!=NULL) {
	This->Value = Data[1]+(Data[2]<<8) ;
      } else {
	fprintf (stderr,"Configuration of sensor %d.%d is wrong (node %d.%d not found)\n",FromLine,FromAdd,ToLine,ToAdd) ;
      }
    } ;
  } ;
}

int HandleCommand (char *Command,int Socket)
{
  int Line,Add,Port ;
  int ToLine,ToAdd ;
  int i ;
  char Makro[NAMELEN*4] ;
  char Com[NAMELEN*4] ;
  char Obj[NAMELEN*4] ;
  char Answer[NAMELEN] ;
  struct Node *This ;
  struct EEPROM EEprom ;
  static struct Node *MenuCurrent = NULL ;
  
  if (MenuCurrent==NULL) MenuCurrent = Haus ;

  Obj[0] = '\0' ;
  Com[0] = '\0' ;

  // Convert Command to lower case
  for (i=0;(Command[i]!=' ')&&(Command[i]!='\0');i++) Command[i]=tolower(Command[i]) ;

  sscanf(Command,"%s %s",Com,Obj) ;
  
  // Send current location and available Childs...
  
  if ((strcmp(Com,"help")==0)||(strcmp(Com,"hilfe")==0)) {
    sprintf (Answer,"Available Commands:\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Help/Hilfe: This help\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Add (Line)(Adress):            Update and configure new node\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Config (Line)(Adress):         Configure node with parameters\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Update (Line)(Adress):         Update node with firmware stored in NodeConf\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Reload:                        Reload CANControl configuration\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"List:                          List all running macros\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Exit:                          Exit this communication\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"TC (Line) (Adress) (Port):     Issue toggle on given adress\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"SC (Line) (Adress) (Command) (Data1) (Data2) (Data3): Issue command on given adress\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"An/On [Object]:                Switch on Object or current location\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Aus/Off [Object]:              Switch off object or current location\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Toggle [Object]:               Toggle object or current location\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Hoch/Up [Object]:              Open shades completely\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Runter/Down [Object]:          Close shades completely\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"KurzHoch/ShortUp [Object]:     Open shades slightly\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"KurzRunter/ShortDown [Object]: Close shades slightly\r\n\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"[Number]:                      Select child object\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Parent:                        Select parent object\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    sprintf (Answer,"Top:                           Select top level\r\n\r\n"); 
    send(Socket,Answer,strlen(Answer),0) ;
    return(TRUE) ;
  }
  
  if (strcmp(Com,"parent")==0) {
    MenuCurrent = MenuCurrent->Parent!=NULL?MenuCurrent->Parent:MenuCurrent ;
  }
  
  if (strcmp(Com,"top")==0) {
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
    if ((This->Type==N_ADRESS)||(This->Type==N_REACT)||
	(This->Type==N_DELAY)||(This->Type==N_TIMER)||
	(This->Type==N_CALL)||(This->Type==N_TASK)||
	(This->Type==N_ALWAYS)||(This->Type==N_ACTIVE)||
	(This->Type==N_STARTUP)||(This->Type==N_IF)||
	(This->Type==N_ELSE)||(This->Type==N_SET)||
	(This->Type==N_REPEAT)||(This->Type==N_WAITFOR)||
	(This->Type==N_LANGUAGE)||(This->Type==N_BROADCAST)||
	(This->Type==N_PORT)||(This->Type==N_FIRMWARE)||
	(This->Type==N_SEQUENCE)||(This->Type==N_PROGRAM)||
	(This->Type==N_LOC)||(strlen(Makro)==0)) continue ;
    sprintf (Answer,"%d: %s\r\n",i,Makro) ;
    send(Socket,Answer,strlen(Answer),0) ;
  } ;
  sprintf (Answer,"99: One level up\r\n") ;
  send(Socket,Answer,strlen(Answer),0) ;
  
  if ((strlen(Com)==0)||(isdigit(Com[0]))) {
    return (TRUE) ;  
  } ;
  
  Answer[0]='\0' ;
  
  if (strcmp(Com,"add")==0) {
    sscanf (Command,"add %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      struct Node *ANodes[MAX_ADD_PER_NODE*4] ;
      int ANumber ;
   
      ANumber = 0 ; 

      CollectAdress(Haus,Line,Add,ANodes,&ANumber) ;
      // Create Configuration for the Board; bootstrap firmware will ask for it
      MakeConfig (Line,Add,&EEprom) ;
      WriteConfig (&EEprom) ;
      // Send out Bootstrap firmware
      if (ANodes[0]->Type!=N_EXTENDED) {
	if ((ANodes[0]->Type==N_SENSOR)||
	    (ANodes[0]->Type==N_SENS2)||
	    (ANodes[0]->Type==N_LIGHT)) {
	  SendFirmware(0xf,0xFF) ;
	} else {
	  SendFirmware(0xF,0xFD) ;
	} ;
      } else {
	SendFirmware(0xF,0xFE) ;
      } ;
      SendConfig(&EEprom,0xF,0xFF) ;
      // Line up application firmware delivery; board will be reset after configuration
      // which restarts bootloader requesting firmware.
      SendFirmware(Line,Add) ;
      sprintf (Answer,"Adding Node\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return (TRUE); 
    } ;
  } ;
  
  if (strcmp(Com,"config")==0) {
    sscanf (Command,"config %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      MakeConfig (Line,Add,&EEprom) ;
      WriteConfig (&EEprom) ;
      SendConfig(&EEprom,Line,Add) ;
      sprintf (Answer,"Update Config\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return (TRUE); 
    } ;
  } ;

  if (strcmp(Com,"readconfig")==0) {
    sscanf (Command,"readconfig %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      ReadConfigStart(Line,Add) ;
      sprintf (Answer,"Reading Config\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return (TRUE); 
    } ;
  } ;

  if (strcmp(Com,"changeadress")==0) {
    sscanf (Command,"changeadress %d %d to %d %d",&Line,&Add,&ToLine,&ToAdd) ;
    if ((Line!=0)&&(Add!=0)) {
      ChangeAdress(Line,Add,ToLine,ToAdd) ;
      sprintf (Answer,"Changing Adress\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return (TRUE); 
    } ;
  } ;
  
  if (strcmp(Com,"update")==0) {
    sscanf (Command,"update %d %d",&Line,&Add) ;
    if ((Line!=0)&&(Add!=0)) {
      SendFirmware(Line,Add) ;
      sprintf (Answer,"Update Firmware\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return (TRUE); 
    } ;
  } ;

  if (strcmp(Com,"tc")==0) {
    sscanf (Command,"tc %d %d %d",&Line,&Add,&Port) ;
    if ((Line!=0)&&(Add!=0)) {
      SendCommand(CHANNEL_TOGGLE,Line,Add,Port) ;
      return (TRUE); 
    } ;
  } ;


  if (strcmp(Com,"led")==0) {
    int LED,r,g,b,w ;
    sscanf (Command,"led %d %d %d %d %d %d %d",&Line,&Add,&LED,&r,&g,&b,&w) ;
    if ((Line!=0)&&(Add!=0)) {
      SendLEDCommand(SET_TO,Line,Add,LED,r,g,b,w,0) ;
      return (TRUE); 
    } ;
  } ;


  if (strcmp(Com,"dled")==0) {
    int LED,r,g,b,w ;
    sscanf (Command,"dled %d %d %d %d %d %d %d %d",&Line,&Add,&LED,&r,&g,&b,&w,&i) ;
    if ((Line!=0)&&(Add!=0)) {
      SendLEDCommand(DIM_TO,Line,Add,LED,r,g,b,w,i) ;
      return (TRUE); 
    } ;
  } ;

  if (strcmp(Com,"hled")==0) {
    int LED,r,g,b,w ;
    sscanf (Command,"hled %d %d %d %d %d %d %d",&Line,&Add,&LED,&r,&g,&b,&w) ;
    if ((Line!=0)&&(Add!=0)) {
      SendLEDCommand(HSET_TO,Line,Add,LED,r,g,b,w,0) ;
      return (TRUE); 
    } ;
  } ;

  if (strcmp(Com,"hws")==0) {
    int LED,r,g,b;
    unsigned char rc,gc,bc ;
    sscanf (Command,"hws %d %d %d %d %d %d",&Line,&Add,&LED,&r,&g,&b) ;
    hsv_to_rgb(r,g,b,&rc,&gc,&bc) ;
    if ((Line!=0)&&(Add!=0)) {
      SendLEDCommand(LOAD_LED,Line,Add,LED,rc,gc,bc,0,0) ;
      SendLEDCommand(OUT_LED,Line,Add,LED,0,0,0,0,0) ;
      return (TRUE); 
    } ;
  } ;
  
  if (strcmp(Com,"reload")==0) {
    if (ReadConfig()!=0) {
      sprintf (Answer,"Error in Configuration\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return(TRUE) ;
    } ;
    sprintf (Answer,"Updated Configuration\r\n") ;
    send(Socket,Answer,strlen(Answer),0) ;
    return (TRUE); 
  } ;
  
  if (strcmp(Com,"list")==0) {
    // List all active macros
    for (i=0;i<MAX_ACTIVEMACROS;i++) {
      if (ActiveMacros[i].Macro!=NULL) {
	Makro[0]='\0' ;
	FullObjectName(ActiveMacros[i].Macro,Makro) ;
	if (ActiveMacros[i].DelayType==W_MACRO) {
	  sprintf (Answer,"Macro: %s waiting for macro %s\r\n",Makro,ActiveMacros[i].Delay.WaitNode->Name) ;
	} else if ((ActiveMacros[i].DelayType==W_DELAY)||(ActiveMacros[i].DelayType==W_TIME)) {
	  struct tm *LTime ;
	  LTime=localtime(&ActiveMacros[i].Delay.WaitTime.tv_sec) ;
	  sprintf (Answer,"Macro: %s waiting until Day %d, Hour %d, minute %d\r\n",Makro,LTime->tm_mday,LTime->tm_hour,LTime->tm_min) ;
	} else{
	  sprintf (Answer,"Macro: %s\r\n",Makro) ;
	} ;
	send(Socket,Answer,strlen(Answer),0) ;
      } ;
    } ;
    return (TRUE); 
  } ;

  if (strcmp(Com,"show")==0) {
    sprintf (Answer,"Value: %d\r\n",MenuCurrent->Value) ;
    send(Socket,Answer,strlen(Answer),0) ;
    return (TRUE) ;
  } ;
  
  if (strcmp(Com,"exit")==0) {
    return (FALSE) ;
  } ;

  if (strlen(Obj)!=0) {
    This = FindNode(Haus->Child,Obj) ;  
    if (This==NULL) {
      strcpy(Answer,"Object not found\r\n") ;
      send(Socket,Answer,strlen(Answer),0) ;
      return(TRUE) ;
    } ;
  } else {
    This = MenuCurrent ;
  } ;
  
  if ((strcmp(Com,"action")==0)||(strcmp(Com,"aktion")==0)) {  
    ExecuteMakro (This) ;
    sprintf (Answer,"Macro %s executed\r\n",This->Name) ;
  } ;
  
  if (GetNodeAdress(This,&Line,&Add,&Port)==0) {
    sprintf (Answer,"%s: %s\r\n",Com,This->Name) ;
    if ((strcmp(Com,"an")==0)||(strcmp(Com,"on")==0)) {
      SendCommand(CHANNEL_ON,Line,Add,Port) ;
    } else if ((strcmp(Com,"aus")==0)||(strcmp(Com,"off")==0)) {
      SendCommand(CHANNEL_OFF,Line,Add,Port) ;
    } else if (strcmp(Com,"toggle")==0) {
      SendCommand(CHANNEL_TOGGLE,Line,Add,Port) ;
    } else if ((strcmp(Com,"hoch")==0)||(strcmp(Com,"up")==0)) {
      SendCommand(SHADE_UP_FULL,Line,Add,Port) ;
    } else if ((strcmp(Com,"runter")==0)||(strcmp(Com,"down")==0)) {
      SendCommand(SHADE_DOWN_FULL,Line,Add,Port) ;
    } else if ((strcmp(Com,"kurzhoch")==0)||(strcmp(Com,"shortup")==0)) {
      SendCommand(SHADE_UP_SHORT,Line,Add,Port) ;
    } else if ((strcmp(Com,"kurzrunter")==0)||(strcmp(Com,"shortdown")==0)) {
      SendCommand(SHADE_DOWN_SHORT,Line,Add,Port) ;
    } else {
      if (strlen(Answer)==0) sprintf (Answer,"Unknown command: %s\r\n",Com) ;
    }
  } else {
    if (strlen(Answer)==0) sprintf (Answer,"Unknown object: %s\r\n",Obj) ;
  } ;    

  send(Socket,Answer,strlen(Answer),0) ;
  return (TRUE) ;
}

static const char delimiter[] = " ,;." ;

int Handle_NaturalCommand (char *Command)
{
  NodeType Com ;
  char CC[NAMELEN*4] ;
  char *NextWord ;
  char Polite ;
  char Name ;
  int Line,Add,Port ;

  struct Node *Actual ;
  struct Node *Floor ;
  struct Node *Room ;
  struct Node *Item ;

  // Parse each word and see, if it is an required element

  Com = Polite = Name = 0 ;
  Floor = Room = Item = NULL ;

  // Search for floor

  strcpy (CC,Command) ;
  NextWord = strtok (CC,delimiter) ;
  while (NextWord) {
    Actual = FindNode(Haus->Child,NextWord) ;
    if (Actual!=NULL) {
      if (Actual->Type==N_STRUCTURE) {
	Floor = Actual ;
	break ;
      } ;
    } ;
    NextWord = strtok (NULL,delimiter) ;
  } ;  
  
  // search for Room
  
  strcpy (CC,Command) ;
  NextWord = strtok (CC,delimiter) ;
  while (NextWord) {
    if (Floor) {
      Actual = FindNode(Floor->Child,NextWord) ;
    } else if (DefaultFloor) {
      Actual = FindNode(DefaultFloor->Child,NextWord) ;
    } else {
      Actual = FindGlobalNode (Haus->Child,NextWord) ;
    } ;
    if (Actual!=NULL) {
      if (Actual->Type==N_STRUCTURE) {
	Room = Actual ;
	break ;
      } ;
    } ;
    NextWord = strtok (NULL,delimiter) ;
  } ;  

  // Find Action
  
  strcpy (CC,Command) ;
  NextWord = strtok (CC,delimiter) ;
  while (NextWord) {
    if (strstr(NextWord,"an")||strstr(NextWord,"on")||
	strstr(NextWord,"anmachen")||strstr(NextWord,"anschalten")) {
      Com = CHANNEL_ON ;
    } else if (strstr(NextWord,"aus")||strstr(NextWord,"off")||
	       strstr(NextWord,"ausmachen")||strstr(NextWord,"ausschalten")) {
      Com = CHANNEL_OFF ;
    } else if (strstr(NextWord,"hoch")||strstr(NextWord,"up")||
	       strstr(NextWord,"auf")||strstr(NextWord,"aufmachen")||strstr(NextWord,"open")) {
      Com = SHADE_UP_FULL ;
    } else if (strstr(NextWord,"runter")||strstr(NextWord,"down")||
	       strstr(NextWord,"herunter")||strstr(NextWord,"schließen")||
	       strstr(NextWord,"zu")||strstr(NextWord,"close")) {
      Com = SHADE_DOWN_FULL;
    } else if ((strstr(NextWord,"bitte"))||(strstr(NextWord,"please"))) {
      Polite = 1 ;
    } else if (strstr(NextWord,Haus->Name)) {
      Name = 1 ;
    }  ;
    NextWord = strtok(NULL,delimiter) ;
  } ;
  
  strcpy (CC,Command) ;
  NextWord = strtok (CC,delimiter) ;
  Actual = NULL ;
  while ((NextWord)&&(Actual==NULL)) {
    if (Room) {
      Actual = FindNode(Room->Child,NextWord) ;
      if ((Actual!=NULL)&&
	  (Actual->Type!=N_ONOFF)&&(Actual->Type!=N_SHADE)&&
	  (Actual->Type!=N_SENSOR)&&(Actual->Type!=N_BAD)&&
	  (Actual->Type!=N_LED)&&(Actual->Type!=N_SENS2)&&
	  (Actual->Type!=N_LIGHT)&&(Actual->Type!=N_EXTENDED)) Actual = NULL ;
    } else {
      // Check for global macro
      Actual = FindNode(Haus->Child,NextWord) ;
      if ((Actual!=NULL)&&(Actual->Type!=N_MACRO)) Actual = NULL;
    } ;
    NextWord = strtok(NULL,delimiter) ;
  } ;

  Item = Actual ;

  
  if ((Item!=NULL)&&(Polite!=0)&&(Name!=0)&&(Com!=0)) {
    if (IsMakro(Item)) Com=A_CALL ;
    // Full command has been given, execute
    if (IsMakro(Item)) {
      fprintf (stderr,"Natural voice commands macro: %s\n",Item->Name) ;
      ExecuteMakro (Item) ;
    } else {
      if (GetNodeAdress(Item,&Line,&Add,&Port)==0) {
	fprintf (stderr,"Natural voice commands %d on Line %d, Add %d, Port %d\n",Com,Line,Add,Port) ;
	SendCommand(Com,Line,Add,Port) ;
      } ;
    }  ;
  } ;
  return (TRUE) ;
}

static void *Handle_Webserver(enum mg_event event, struct mg_connection *conn) 
{
  const struct mg_request_info *ri = mg_get_request_info(conn); 
  char data[NAMELEN*4] ;
  char pos[NAMELEN*4] ;
  char Com[NAMELEN*4] ;
  char Obj[NAMELEN*4] ;
  struct Node *This ;
  int Line,Add,Port ;
  int XPos,YPos ;
  int i,j ;

  if (event==MG_NEW_REQUEST) {
    if (!strcmp(ri->uri, "/Action")) {      
      // User has submitted a form, show submitted data and a variable value 
      if (ri->query_string==NULL) return (NULL) ;
      
      for (i=0,j=0;(ri->query_string[i]!='\0')&&(ri->query_string[i]!='.')&&(ri->query_string[i]!='=')&&(j<NAMELEN*4-1);i++,j++) {
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


      if ((ri->query_string[i]!='\0')&&(ri->query_string[i]=='.')&&(ri->query_string[i+1]=='x')) {
	for (i+=3,j=0;(ri->query_string[i]!='\0')&&(ri->query_string[i]!='&')&&(j<NAMELEN*4-1);i++,j++) pos[j]=ri->query_string[i] ;
	pos[j]='\0' ;
	sscanf (pos,"%d",&XPos) ;
	for (;(ri->query_string[i]!='\0')&&(ri->query_string[i]!='=')&&(j<NAMELEN*4-1);i++);
	if (ri->query_string[i]!='\0') {
	  for (i++,j=0;(ri->query_string[i]!='\0')&&(ri->query_string[i]!='&')&&(j<NAMELEN*4-1);i++,j++) pos[j]=ri->query_string[i] ;
	  pos[j]='\0' ;

	  sscanf (pos,"%d",&YPos) ;
	}; 
      } ; 
      
      
      sscanf(data,"%s %s",Com,Obj) ;

      if ((strlen(Com)==0)||strlen(Obj)==0) return NULL ;

      This = FindNode(Haus->Child,Obj) ;  

      if ((strcmp(Com,"Action")==0)||(strcmp(Com,"Aktion")==0)) {  
	ExecuteMakro (This) ;
      } ;

      if ((strcmp(Com,"SetX")==0)||(strcmp(Com,"SetzeX")==0)) {  
	if (This!=NULL) This->Value=XPos ;
      } ;

      if ((strcmp(Com,"SetY")==0)||(strcmp(Com,"SetzeY")==0)) {  
	if (This!=NULL) This->Value=YPos ;
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

struct timeval Now ;

/* Hauptprogramm */

struct Node *Ergebnis[MAX_ADD_PER_NODE] ;

int main(int argc, char **argv) 
{
  int Error ;

  struct mg_context *ctx ;
  struct timeval Wait,Old ;
  struct tm *timeinfo ;
  struct timeval Day ;
  int i ;

  char *web_options[] = {
    "listening_ports", "8080",
    "document_root", "./Webpage",
    "url_rewrite_patterns","/NodeConf=./NodeConf",
    "num_threads","6",
    NULL
  };

  logfd = stderr ;
  for (i=1;i<argc;i++) {
    if (strstr("-v",argv[i])!=NULL) Verbose = 1 ;
    if (strstr("-t",argv[i])!=NULL) NoTime = 1 ;
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
      printf ("Usage: %s [-v] [-t] [-f FILE] [-noroute] [--help] [-?]\n\n",argv[0]) ;
      printf ("       -v: Verbose\n") ;
      printf ("       -t: dont display time messages\n") ;
      printf ("       -f FILE: log into file FILE\n") ;
      printf ("       -?, --help: display this message\n") ;
      exit(0) ;
    } ;
  } ;    

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
  EveryDay () ;
  Day.tv_sec = time(NULL) ;
  Day.tv_usec = 0 ;

  timeinfo = localtime(&Day.tv_sec) ;
  timeinfo->tm_mday++ ;
  timeinfo->tm_hour = 0 ;
  Day.tv_sec = mktime(timeinfo) ;

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
    if (Now.tv_sec>Day.tv_sec) { // another day gone
      EveryDay () ;
      timeinfo = localtime(&Day.tv_sec) ;
      timeinfo->tm_mday++ ;
      Day.tv_sec = mktime(timeinfo) ;
    }
    
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include <math.h>
#include "libwebsocket/libwebsockets.h"
#include "ConfigNodes.h"
#include "XMLConfig.h"
#define SERVER_INCLUDE 1
#include "../Apps/Common/mcp2515.h"
#include "Network.h"

#define BUFF_SIZE 10240

//#define DEBUG 1

struct Node *Haus=NULL ;
struct Node *DefaultFloor=NULL ;
struct Node *Current=NULL ;
struct SeqList *Sequences=NULL ;
struct NodeList *Reactions=NULL ;
double West,North ;
struct tm SunSet ;
struct tm SunRise ;


int MakroNummer;
int Depth;

struct TypSel Types[] = {
  {"Haus",N_STRUCTURE},
  {"Home",N_STRUCTURE},
  {"Etage",N_STRUCTURE},
  {"Floor",N_STRUCTURE},
  {"Zimmer",N_STRUCTURE},
  {"Room",N_STRUCTURE},
  {"Gruppe",N_GROUP},
  {"Group",N_GROUP},
  {"Element",N_ELEMENT},
  {"Schalter",N_ONOFF},
  {"Switch",N_ONOFF},
  {"Rollo",N_SHADE},
  {"Shade",N_SHADE},
  {"Adresse",N_ADRESS},
  {"Adress",N_ADRESS},
  {"Sensor",N_SENSOR},
  {"Bad",N_BAD},
  {"Bath",N_BAD},
  {"LED",N_LED},
  {"Taster",N_SENS2},
  {"Button",N_SENS2},
  {"Lichtsensor",N_LIGHT},
  {"Lightsensor",N_LIGHT},
  {"Multifunktion",N_EXTENDED},
  {"Multifunction",N_EXTENDED},
  {"Aktion",N_ACTION},
  {"Action",N_ACTION},
  {"Makro",N_MACRO},
  {"Macro",N_MACRO},
  {"Reaction",N_REACT},
  {"Reaktion",N_REACT},
  {"Warte",N_DELAY},
  {"Wait",N_DELAY},
  {"Timer",N_TIMER},
  {"Sub",N_CALL},
  {"Sub",N_CALL},
  {"Gleichzeitig",N_TASK},
  {"Task",N_TASK},
  {"Immer",N_ALWAYS},
  {"Always",N_ALWAYS},
  {"Aktiv",N_ACTIVE},
  {"Active",N_ACTIVE},
  {"Start",N_STARTUP},
  {"Startup",N_STARTUP},
  {"Taeglich",N_DAILY},
  {"Daily",N_DAILY},
  {"Wenn",N_IF},
  {"Sonst",N_ELSE},
  {"If",N_IF},
  {"Else",N_ELSE},
  {"Variable",N_VAR},
  {"Var",N_VAR},
  {"Setze",N_SET},
  {"Set",N_SET},
  {"Solange",N_REPEAT},
  {"While",N_REPEAT},
  {"Warte_Auf",N_WAITFOR},
  {"Wait_for",N_WAITFOR},
  {"Sprache",N_LANGUAGE},
  {"Language",N_LANGUAGE},
  {"Port",N_PORT},
  {"Broadcast",N_BROADCAST},
  {"Firmware",N_FIRMWARE},
  {"Seq",N_SEQUENCE},
  {"Program",N_PROGRAM},
  {"Programm",N_PROGRAM},
  {"Ort",N_LOC},
  {"Location",N_LOC},
  {"Einfach",S_SIMPLE},
  {"Simple",S_SIMPLE},
  {"KurzLang",S_SHORTLONG},
  {"ShortLong",S_SHORTLONG},
  {"Rollo_KL",S_SHADE_SHORTLONG},
  {"Shade_SL",S_SHADE_SHORTLONG},
  {"Rollo_Einfach",S_SHADE_SIMPLE},
  {"Shade_Simple",S_SHADE_SIMPLE},
  {"Monoflop",S_MONO},
  {"BWM_OC",S_BWM},
  {"Movement_OC",S_BWM},
  {"BWM",S_BWM2},
  {"Movement",S_BWM2},
  {"R_Monoflop",S_RETMONO},
  {"Analog",S_ANALOG},
  {"Ausgang",S_OUTPUT},
  {"Output",S_OUTPUT},
  {"PWM",S_PWM},
  {"WSData",S_WSDATA},
  {"WSClock",S_WSCLOCK},
  {"I2CLichtsensor",S_LIGHT},
  {"I2CLightsensor",S_LIGHT},
  {"An",A_ON},
  {"On",A_ON},
  {"Aus",A_OFF},
  {"Off",A_OFF},
  {"Toggle",A_TOGGLE},
  {"Hoch",A_SHADE_UP_FULL},
  {"Up",A_SHADE_UP_FULL},
  {"KurzHoch",A_SHADE_UP_SHORT},
  {"ShortUp",A_SHADE_UP_SHORT},
  {"Runter",A_SHADE_DOWN_FULL},
  {"Down",A_SHADE_DOWN_FULL},
  {"KurzRunter",A_SHADE_DOWN_SHORT},
  {"ShortDown",A_SHADE_DOWN_SHORT},
  {"Wert",A_SEND_VAL},
  {"Value",A_SEND_VAL},
  {"Herzschlag",A_HEARTBEAT},
  {"Heartbeat",A_HEARTBEAT},
  {"Rufe",A_CALL},
  {"Call",A_CALL},
  {"Sequenz",A_SEQUENCE},
  {"Sequence",A_SEQUENCE},
  {"RGBLED",A_LEDSET},
  {"HSVLED",A_LEDHSET},
  {"DIMRGB",A_LEDDIM},
  {"DIMHSV",A_LEDHDIM},
  {"StarteLED",A_STARTLED},
  {"LEDStart",A_STARTLED},
  {"StoppLED",A_STOPLED},
  {"LEDStop",A_STOPLED},
  {"LEDProgramm",A_PROGLED},
  {"ProgramLED",A_PROGLED},
  {NULL,0} 
} ;

NodeType FillType(const char *This)
{
  int i ;
  for (i=0;Types[i].Name!=NULL;i++) {
    if (strcmp(This,Types[i].Name)==0) break ;
  }
  return (Types[i].Type) ;
}

int ParseError ;

void XMLCALL start(void *data, const char *el, const char **attr) 
{
  int i,j,Time;
  char Val[5] ;
  struct Node *This ;
  struct NodeList *NL ;


  if (Current==NULL) {
    Current = CreateNode() ;
    Haus = Current ;
  } else {
    Current = NewChild (Current) ;
  } ;

  strncpy (Current->TypeDef,el,NAMELEN) ;
  Current->Type = FillType (Current->TypeDef) ;

  if (Current->Type==N_REACT) {
    // In die Liste der Reactions einhängen
    if (Reactions==NULL) {
      Reactions = malloc (sizeof (struct NodeList)) ;
      Reactions->Next = NULL ;
      Reactions->Node = Current ;
    } else {
      for (NL=Reactions;NL->Next!=NULL;NL=NL->Next) ;
      NL->Next = malloc (sizeof(struct NodeList)) ;
      NL = NL->Next ;
      NL->Next = NULL ;
      NL->Node = Current ;
    } ;
  } ;

  if (Current->Type==N_DMX) {
    strcpy(Current->Data.DMX.ID,"Art-Net") ;
    Current->Data.DMX.OpOutputLow=0 ;
    Current->Data.DMX.OpOutputHigh=0 ;
    Current->Data.DMX.ProtVerHi=0 ;
    Current->Data.DMX.ProtVerLo=14 ;
    Current->Data.DMX.Sequence=0 ;
    Current->Data.DMX.Pyhsical=0 ;
    Current->Data.DMX.SubUni=0 ;
    Current->Data.DMX.Net=0 ;
    Current->Data.DMX.LengthHi=0x1 ;
    Current->Data.DMX.Length=0xFF ;
    for (i=0;i<512;i++) Current->Data.DMX.Data[i]=0 ;
  }


  for (i = 0; attr[i]; i += 2) {
    if (strcmp(attr[i],"name")==0) {
      strncpy(Current->Name,attr[i+1],NAMELEN) ;
      continue; 
    } ;
    if ((strcmp(attr[i],"wert")==0)||(strcmp(attr[i],"value")==0)) {
      sscanf(attr[i+1],"%d",&(Current->Value)) ;
    } ;
    switch (Current->Type) {
    case N_STRUCTURE:
      if ((strcmp(attr[i],"default")==0)||(strcmp(attr[i],"default")==0)) {
	DefaultFloor = Current ;
      }  ;
      break ;
    case N_GROUP:
      if ((strcmp(attr[i],"nummer")==0)||(strcmp(attr[i],"number")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Group.Number)) ;
      } ;
    case N_ADRESS:
      if ((strcmp(attr[i],"linie")==0)||(strcmp(attr[i],"line")==0)||
	  (strcmp(attr[i],"universum")==0)||(strcmp(attr[i],"universe")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Linie)) ;
      } ;
      if ((strcmp(attr[i],"knoten")==0)||(strcmp(attr[i],"node")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Knoten)) ;
      } ;
      if (strcmp(attr[i],"port")==0) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Port)) ;
      } ;
      if (strcmp(attr[i],"dmx")==0) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.DMX)) ;
      } ;
      if (strcmp(attr[i],"offset")==0) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Offset)) ;
      } ;
      break ;
    case N_ACTION:
      if ((strcmp(attr[i],"kommando")==0)||(strcmp(attr[i],"command")==0)) {
	Current->Data.Aktion.Type = FillType(attr[i+1]) ;
      }
      if ((strcmp(attr[i],"objekt")==0)||(strcmp(attr[i],"object")==0)) {
	strncpy(Current->Data.Aktion.UnitName,attr[i+1],NAMELEN*4) ;
      } ;
      if ((strcmp(attr[i],"sequenz")==0)||(strcmp(attr[i],"sequence")==0)) {
	strncpy(Current->Data.Aktion.Sequence,attr[i+1],NAMELEN) ;
      } ;
      if ((strcmp(attr[i],"autonom")==0)||(strcmp(attr[i],"standalone")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Aktion.StandAlone)) ;
      } ;
      if ((strcmp(attr[i],"kurz")==0)||(strcmp(attr[i],"short")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Aktion.Short)) ;
      } ;
      if ((strcmp(attr[i],"R")==0)||(strcmp(attr[i],"H")==0)) {
	sscanf(attr[i+1],"%hhi",&(Current->Data.Aktion.R)) ;
      } ;
      if ((strcmp(attr[i],"G")==0)||(strcmp(attr[i],"S")==0)) {
	sscanf(attr[i+1],"%hhi",&(Current->Data.Aktion.G)) ;
      } ;
      if ((strcmp(attr[i],"B")==0)||(strcmp(attr[i],"V")==0)) {
	sscanf(attr[i+1],"%hhi",&(Current->Data.Aktion.B)) ;
      } ;
      if (strcmp(attr[i],"W")==0) {
	sscanf(attr[i+1],"%hhi",&(Current->Data.Aktion.W)) ;
      } ;
      if ((strcmp(attr[i],"dauer")==0)||(strcmp(attr[i],"time")==0)) {
	sscanf(attr[i+1],"%hhi",&(Current->Data.Aktion.Delay)) ;
      } ;
      if ((strcmp(attr[i],"schritt")==0)||(strcmp(attr[i],"step")==0)) {
	sscanf(attr[i+1],"%hhi",&(Current->Data.Aktion.Step)) ;
      } ;
      break ;
    case N_SENSOR:
    case N_SENS2:
    case N_LIGHT:
    case N_EXTENDED:
      if ((strcmp(attr[i],"typ")==0)||(strcmp(attr[i],"type")==0)) {
	Current->Data.Sensor.SensorTyp = FillType(attr[i+1]);
      } ;
      if ((strcmp(attr[i],"lang")==0)||(strcmp(attr[i],"long")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Lang)) ;
      } ;
      if ((strcmp(attr[i],"ende")==0)||(strcmp(attr[i],"end")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Ende)) ;
      } ;
      if (strcmp(attr[i],"reset")==0) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Reset)) ;
      } ;
      if ((strcmp(attr[i],"dauer")==0)||(strcmp(attr[i],"time")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Reset)) ;
      } ;
      if ((strcmp(attr[i],"intervall")==0)||(strcmp(attr[i],"interval")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Intervall)) ;
      } ;
      if ((strcmp(attr[i],"led")==0)||(strcmp(attr[i],"led")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.WSNum)) ;
      } ;
      if ((strcmp(attr[i],"virtled")==0)||(strcmp(attr[i],"virtled")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.VirtWSNum)) ;
      } ;
      if ((strcmp(attr[i],"power1")==0)||(strcmp(attr[i],"power1")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Power1)) ;
      } ;
      if ((strcmp(attr[i],"power2")==0)||(strcmp(attr[i],"power2")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Power2)) ;
      } ;
      break ;
    case N_BAD:
      if ((strcmp(attr[i],"dauer")==0)||(strcmp(attr[i],"duration")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Lang)) ;
      } ;
      break ;
    case N_SHADE:
      if ((strcmp(attr[i],"lang")==0)||(strcmp(attr[i],"long")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Rollo.Lang)) ;
      } ;
      if ((strcmp(attr[i],"kurz")==0)||(strcmp(attr[i],"short")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Rollo.Kurz)) ;
      } ;
      if ((strcmp(attr[i],"vertauschen")==0)||(strcmp(attr[i],"swap")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Rollo.Swap)) ;
      } ;
      break ;
    case N_DELAY:
      if ((strcmp(attr[i],"zeit")==0)||(strcmp(attr[i],"time")==0)) {
	sscanf (attr[i+1],"%d",&Time) ;
	Current->Data.Time.tv_sec = Time/1000 ;
	Current->Data.Time.tv_usec = Time%1000 * 1000 ;
      } ;
      break ;
    case N_TIMER:
      if ((strcmp(attr[i],"zeit")==0)||(strcmp(attr[i],"time")==0)) {
	strncpy(Current->Data.Wert.Wert,attr[i+1],NAMELEN*2) ;
      } ;
      if ((strcmp(attr[i],"relativ")==0)||(strcmp(attr[i],"relative")==0)) {
	if ((strcmp(attr[i+1],"vor aufgang")==0)||(strcmp(attr[i],"before sunrise")==0)) {
	  Current->Value = 1 ;
	} else if ((strcmp(attr[i+1],"nach aufgang")==0)||(strcmp(attr[i],"after sunrise")==0)) {
	  Current->Value = 2 ;
	} else 	if ((strcmp(attr[i+1],"vor untergang")==0)||(strcmp(attr[i],"before sunset")==0)) {
	  Current->Value = 3 ;
	} else 	if ((strcmp(attr[i+1],"nach untergang")==0)||(strcmp(attr[i],"after sunset")==0)) {
	  Current->Value = 4 ;
	} ;
      } ;
      break ;
    case N_CALL:
    case N_TASK:
      if ((strcmp(attr[i],"makro")==0)||(strcmp(attr[i],"macro")==0)) {
	strncpy(Current->Data.UnitName,attr[i+1],NAMELEN*4) ;
      } ;
      break ;
    case N_IF:
    case N_SET:
    case N_REPEAT:
    case N_WAITFOR:
    case N_ELEMENT:
      if ((strcmp(attr[i],"objekt")==0)||(strcmp(attr[i],"object")==0)) {
	strncpy(Current->Data.Wert.UnitName,attr[i+1],NAMELEN*4) ;
      } ;
      if ((strcmp(attr[i],"wert")==0)||(strcmp(attr[i],"value")==0)) {
	strncpy(Current->Data.Wert.Wert,attr[i+1],NAMELEN*2) ;
      } ;
      break ;
    case N_VAR:
      if ((strcmp(attr[i],"wert")==0)||(strcmp(attr[i],"value")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Value)) ;
      } ;
      break ;
    case N_LANGUAGE:
      if ((strcmp(attr[i],"ist")==0)||(strcmp(attr[i],"is")==0)) {
	switch (attr[i+1][0]) {
	case 'd':
	  setlocale(LC_TIME,"de_DE") ;
	  break ;
	default:
	  break ;
	} ;
      } ;
    case N_PORT:
      if (strcmp(attr[i],"CAN")==0) {
	strncpy(CAN_PORT,attr[i+1],19) ;
	sscanf (CAN_PORT,"%d",&CAN_PORT_NUM) ;
      } ;
      if (strcmp(attr[i],"WS")==0) {
	strncpy(WS_PORT,attr[i+1],19) ;
	sscanf (WS_PORT,"%d",&WS_PORT_NUM) ;
      } ;
      if (strcmp(attr[i],"COM")==0) {
	strncpy(COM_PORT,attr[i+1],19) ;
	sscanf (COM_PORT,"%d",&COM_PORT_NUM) ;
      } ;
      if (strcmp(attr[i],"VOICE")==0) {
	strncpy(VOICE_PORT,attr[i+1],19) ;
	sscanf (VOICE_PORT,"%d",&VOICE_PORT_NUM) ;
      } ;
      if (strcmp(attr[i],"HTTP")==0) {
	strncpy(HTTP_PORT,attr[i+1],19) ;
	sscanf (HTTP_PORT,"%d",&HTTP_PORT_NUM) ;
      } ;
      break ;
    case N_BROADCAST:
      if (strcmp(attr[i],"IP")==0) {
	strncpy(CAN_BROADCAST,attr[i+1],NAMELEN-1) ;
      } ;
      break ;
    case N_FIRMWARE:
      if (strcmp(attr[i],"id")==0) {
	sscanf(attr[i+1],"%d",&(Current->Value)) ;
      } ;
      break ;
    case N_SEQUENCE:
      if ((strcmp(attr[i],"file")==0)||(strcmp(attr[i],"datei")==0)) {
	ReadSequence (Current->Name,(char*)attr[i+1]) ;
      } ;
      break ;
    case N_LOC:
      if (strcmp(attr[i],"west")==0) {
	sscanf (attr[i+1],"%lf",&West) ;
      } ;
      if ((strcmp(attr[i],"north")==0)||(strcmp(attr[i],"nord")==0)) {
	sscanf (attr[i+1],"%lf",&North) ;
      } ;
      break ;
    case N_PROGRAM:
      if (strcmp(attr[i],"port")==0) {
	sscanf(attr[i+1],"%hhd",&(Current->Data.Program.Port)) ;
      } ;
      if (strcmp(attr[i],"data")==0) {
	Val[0] = '0' ;
	Val[1] = 'x' ;
	Val[4] = '\0' ;
	for (j=0;j<50;j++) Current->Data.Program.Data[j]=0 ;
	for (j=0;attr[i+1][j]!='\0';j+=2) {
	  Val[2] = attr[i+1][j] ;
	  Val[3] = attr[i+1][j+1] ;
	  sscanf (Val,"%hhx",&(Current->Data.Program.Data[j/2])) ;
	} ;
      }; 
      break ;
    case N_REACT:
      if ((strcmp(attr[i],"von")==0)||(strcmp(attr[i],"from")==0)) {
	sscanf(attr[i+1],"%i.%i",&(Current->Data.Reaction.From.Linie),
	       &(Current->Data.Reaction.From.Knoten)) ;
      } ;
      if ((strcmp(attr[i],"von_maske")==0)||(strcmp(attr[i],"from_mask")==0)) {
	sscanf(attr[i+1],"%i.%i",&(Current->Data.Reaction.FromMask.Linie),
	       &(Current->Data.Reaction.FromMask.Knoten)) ;
      } ;
      if ((strcmp(attr[i],"nach")==0)||(strcmp(attr[i],"to")==0)) {
	sscanf(attr[i+1],"%i.%i",&(Current->Data.Reaction.To.Linie),
	       &(Current->Data.Reaction.To.Knoten)) ;
      } ;
      if ((strcmp(attr[i],"nach_maske")==0)||(strcmp(attr[i],"to_mask")==0)) {
	sscanf(attr[i+1],"%i.%i",&(Current->Data.Reaction.ToMask.Linie),
	       &(Current->Data.Reaction.ToMask.Knoten)) ;
      } ;
      if ((strcmp(attr[i],"daten")==0)||(strcmp(attr[i],"data")==0)) {
	for (j=0;j<8;j++) Current->Data.Reaction.Data[j] = 0  ;
	sscanf(attr[i+1],"%hhi %hhi %hhi %hhi %hhi %hhi %hhi %hhi",
	       &(Current->Data.Reaction.Data[0]),&(Current->Data.Reaction.Data[1]),
	       &(Current->Data.Reaction.Data[2]),&(Current->Data.Reaction.Data[3]),
	       &(Current->Data.Reaction.Data[4]),&(Current->Data.Reaction.Data[5]),
	       &(Current->Data.Reaction.Data[6]),&(Current->Data.Reaction.Data[7])) ;
      } ;	
      if ((strcmp(attr[i],"daten_maske")==0)||(strcmp(attr[i],"data_mask")==0)) {
	for (j=0;j<8;j++) Current->Data.Reaction.DataMask[j] = 0  ;
	sscanf(attr[i+1],"%hhi %hhi %hhi %hhi %hhi %hhi %hhi %hhi",
	       &(Current->Data.Reaction.DataMask[0]),&(Current->Data.Reaction.DataMask[1]),
	       &(Current->Data.Reaction.DataMask[2]),&(Current->Data.Reaction.DataMask[3]),
	       &(Current->Data.Reaction.DataMask[4]),&(Current->Data.Reaction.DataMask[5]),
	       &(Current->Data.Reaction.DataMask[6]),&(Current->Data.Reaction.DataMask[7])) ;
      } ;	
      break ;
    default:
      break ;
    } ;
  }
  // Ueberpruefen auf Semantik
  // Namensgleichheit
  for (This=Current->Prev;This!=NULL;This=This->Prev) {
    if ((strlen(Current->Name)>0)&&(strcmp(This->Name,Current->Name)==0)) {
      // Name wurde schon verwendet
      ParseError = 1 ;
      fprintf (stderr,"Name doppelt verwendet: ") ;
      for (This=Current;This!=NULL;This=This->Parent) fprintf (stderr,"%s ",This->Name) ;
      fprintf (stderr,"\n") ;
      break ;
    }
  } ;
  // Einzelelemente
  if (Current->Type==N_ADRESS) {
    if ((This=FindNodeAdress(Haus,Current->Data.Adresse.Linie,Current->Data.Adresse.Knoten,Current->Data.Adresse.Port,Current))!=NULL) {
      ParseError = 1 ;
      fprintf (stderr,"Adresse wurde doppelt verwendet: Linie %d, Knoten %d, Port %d\n",
	       Current->Data.Adresse.Linie,Current->Data.Adresse.Knoten,Current->Data.Adresse.Port) ;
      fprintf (stderr,"Erste Verwendung: ") ;
      for (;This!=NULL;This=This->Parent) fprintf (stderr,"%s ",This->Name) ;
      fprintf (stderr,"\nZweite Verwendung: ") ;
      for (This=Current;This!=NULL;This=This->Parent) fprintf (stderr,"%s ",This->Name) ;
      fprintf (stderr,"\n") ;
    } ;
    // Schauen, ob im Parent schon eine Adresse eingetragen ist; ggf diese ebenfalls
    // ueberschreiben (Ein Element sollte nur eine Adresse haben...)
    // Dient auch der Möglichkeit, Makros nachträglich mit einer Adresse zu versehen,
    // um die Konfiguration von Aktionen im Sensor nicht von der Reihenfolge der Makros
    // abhaengig zu haben.
    for (This=Current->Prev;This!=NULL;This=This->Prev)
      if (This->Type==N_ADRESS) {
	This->Data.Adresse.Linie = Current->Data.Adresse.Linie ;
	This->Data.Adresse.Knoten = Current->Data.Adresse.Knoten ;
	This->Data.Adresse.Port = Current->Data.Adresse.Port ;
      } ;
  } ;
  
  // Element nachbearbeiten
  // Makros nummerieren 
  if (Current->Type==N_MACRO) {
    This = NewChild(Current) ;
    This->Type = N_ADRESS ;
    This->Data.Adresse.Linie = 0 ;
    This->Data.Adresse.Knoten = (MakroNummer>>8)+50 ;
    This->Data.Adresse.Port = (MakroNummer&0xFF) ;
    MakroNummer++ ;
  } ;

  Depth++;
}  /* End of start handler */

void XMLCALL end(void *data, const char *el) 
{
  Current = Current->Parent ;
  Depth--;
}  /* End of end handler */

void UpdateActions (struct Node *Root)
{
  struct Node *This ;
  
  for (This=Root;This!=NULL;This=This->Next) {
    if (This->Type==N_ACTION) {
      This->Data.Aktion.Unit = FindNode(Haus->Child,This->Data.Aktion.UnitName) ;
      if (This->Data.Aktion.Unit==NULL) {
	ParseError = 1 ;
	fprintf (stderr,"Konnte Aktor %s nicht finden\n",This->Data.Aktion.UnitName) ;
      } ;
    } ;
    // Rekursiv alle updaten
    UpdateActions(This->Child) ;
  } ;
}

void AddConstants (struct Node *Root)
{
  struct Node *This ;

  This = NewChild(Root) ;
  strcpy (This->Name,"wahr") ;
  This->Type=N_VAR ;
  This->Value = (1==1) ;

  This = NewChild(Root) ;
  strcpy (This->Name,"true") ;
  This->Type=N_VAR ;
  This->Value = (1==1) ;

  This = NewChild(Root) ;
  strcpy (This->Name,"falsch") ;
  This->Type=N_VAR ;
  This->Value = (1==0) ;

  This = NewChild(Root) ;
  strcpy (This->Name,"false") ;
  This->Type=N_VAR ;
  This->Value = (1==0) ;
} ;

/* Parse a document from the open file descriptor 'fd' until the parse
   is complete (the document has been completely parsed, or there's
   been an error), or the parse is stopped.  Return non-zero when
   the parse is merely suspended.
*/
int parse_xml(XML_Parser p, int fd)
{
  for (;;) {
    int bytes_read;
    enum XML_Status status;
    
    void *buff = XML_GetBuffer(p, BUFF_SIZE);
    if (buff == NULL) {
      /* handle error... */
      ParseError = 1 ;
      fprintf (stderr,"No Buffer\n"); 
      return 0;
    }
    bytes_read = read(fd, buff, BUFF_SIZE);
    if (bytes_read < 0) {
      /* handle error... */
      ParseError = 1 ;
      fprintf (stderr,"File Error\n"); 
      return 0;
    }
    status = XML_ParseBuffer(p, bytes_read, bytes_read == 0);
    switch (status) {
    case XML_STATUS_ERROR:
      /* handle error... */
      ParseError = 1 ;
      fprintf (stderr,"Parse Error: %s at Line %d, Column %d \n",XML_ErrorString(XML_GetErrorCode(p)),(int)XML_GetCurrentLineNumber(p),(int)XML_GetCurrentColumnNumber(p)); 
      return 0;
    case XML_STATUS_SUSPENDED:
      return 1;
    case XML_STATUS_OK:
      break ;
    }
    if (bytes_read == 0)
      return 0;
  }
}

int ReadConfig(void) 
{
  int InFile ;
  XML_Parser p;
  
  ParseError = 0 ;

  FreeNode (Haus) ;
  FreeNode (DMXData) ;
  

  p = XML_ParserCreate(NULL);
  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    return(1);
  }
  
  InFile = open("NodeConf/Config.xml",O_RDONLY) ;
  if (InFile==-1) {
    fprintf(stderr,"Konnte Datei nicht oeffnen") ;
    return(1) ;
  }
 
  Current = NULL; 
  
  XML_SetElementHandler(p, start, end);
  
  parse_xml(p,InFile) ;

  UpdateActions(Haus->Child) ;
  AddConstants (Haus->Child) ;

  XML_ParserFree(p);
  close(InFile) ;

  return (ParseError) ;
} 

void ReadTrippleVals (char *Line, int InitParas, struct Sequence *This)
{
  char Val[5] ;
  int i,j ;
  
  Val[0] = '0' ;
  Val[1] = 'x' ;
  Val[4] = '\0' ;
  
  // Die initialen Parameter (Zeilennummer, Kommando,Parameter,...) ueberspringen
  for (i=0,j=0;j<InitParas;j++) {
    for (;i<strlen(Line)&&Line[i]!=' ';i++) ;
    for (;i<strlen(Line)&&Line[i]==' ';i++) ; // i is on next Parameter
  } ;
  
  // Whitespace und Initialparameter loeschen
  
  for (j=0;i<strlen(Line);i++) {
    Line[j]=Line[i] ;
    if ((Line[i]!=' ')&&(Line[i]!='\t')&&(Line[i]!='\r')&&(Line[i]!='\n')) j++ ;
  } ;
  Line[j]='\0' ;
  
  // Werte einlesen
  
  for (i=0;i<strlen(Line);i+=2) {
    if (Line[i]!='$') {
      Val[2] = Line[i] ;
      Val[3] = Line[i+1] ;
      sscanf (Val,"%hhx",&(This->Data[i/2])) ;
    } else {
      Val[2] = '0' ;
      Val[3] = Line[i+1] ;
      sscanf (Val,"%hhx",&(This->Var[i/2])) ;
    } ;
  } ;
  This->DataLen = i/2 ;
}

void hsv_to_rgb (unsigned char h, unsigned char s, unsigned char v,unsigned char *r, unsigned char *g, unsigned char *b)
{
  unsigned char i, f;
  unsigned char x,y,z ;
  unsigned int p, q, t;
  
  x = h; y=s ; z=v ;
  if( s == 0 ) {	
    *r = *g = *b = z;
  } else {
    i=x/43;
    f=x%43;
    p = (z * (255 - y))/256;
    q = (z * ((10710 - (y * f))/42))/256;
    t = (z * ((10710 - (y * (42 - f)))/42))/256;
    
    switch( i ) {
    case 0:
      *r = z; *g = t; *b = p; 
      break;
    case 1:
      *r = q; *g = z; *b = p; 
      break;
    case 2:
      *r = p; *g = z; *b = t; 
      break;
    case 3:
      *r = p; *g = q; *b = z; 
      break;			
    case 4:
      *r = t; *g = p; *b = z; 
      break;				
    case 5:
      *r = z; *g = p; *b = q; 
      break;
    }
  }
}

void ReadSequence (char *Name, char *FileName) 
{
  FILE *InFile ;
  struct SeqList *List ;
  struct Sequence *This,*That ;
  char Line[NAMELEN] ;
  char Line2[NAMELEN] ;
  char Command[NAMELEN] ;
  char UnitName[NAMELEN*4] ;
  char Val[10] ;
  int i,j ;

#ifdef DEBUG
  printf ("\n\nNew file: %s\n",FileName) ;
#endif

  InFile = fopen(FileName,"r") ;
  if (InFile==NULL) {
    fprintf (stderr,"File %s not found for sequence %s\n",FileName,Name) ;
    return ;
  } ;

  if (Sequences==NULL) {
    Sequences = malloc (sizeof(struct SeqList)) ;
    if (Sequences==NULL) {
      fprintf (stderr,"Out of memory\n") ;
      exit(-1) ;
    } ;
    List = Sequences ;
  } else {
    for (List=Sequences;List->Next!=NULL;List=List->Next) ;
    List->Next = malloc (sizeof(struct SeqList)) ;
    if (List->Next==NULL) {
      fprintf (stderr,"Out of memory\n") ;
      exit(-1) ;
    } ;
    List=List->Next ;
  } ;

  strcpy (List->Name,Name) ;
  This = NULL ;

  while (fgets(Line2,NAMELEN,InFile)!=NULL) {
    // Kommentarzeilen ausblenden
    if (Line2[0]=='%') continue ;

    // Ganze Zeile zu Grossbuchstaben wandeln

    for (i=0;i<strlen(Line2);i++) Line[i]=toupper(Line2[i]); 
    Line[i]='\0' ;

    // Neuen Sequenzschritt allokieren
    if (This==NULL) {
      List->First = malloc(sizeof(struct Sequence)) ;
      This=List->First ;
    } else {
      This->Next = malloc(sizeof(struct Sequence)) ;
      This=This->Next ;
    } ;
    if (This==NULL) {
      fprintf (stderr,"Out of memory\n") ;
      exit(0) ;
    } ;
    
    // Initialisiere Sequenz-Schritt
    This->Next = NULL ;
    This->CurrVal = 0 ;
    for (i=0;i<MAX_WSLEDS*3;i++) {
      This->Data[i]=0 ;
      This->Var[i]=10 ;
    } ;
    
#ifdef DEBUG
    printf ("%s",Line) ;    
#endif

    // Lese Zeilennummer, Kommando und Parameter ein
    sscanf (Line,"%d %s %d",&(This->LineNumber),Command,&(This->Para)) ;

    if ((strcmp(Command,"DIM")==0)||(strcmp(Command,"DIM_H")==0)||
	(strcmp(Command,"SINGLE")==0)||(strcmp(Command,"SINGLE_H")==0)||
	(strcmp(Command,"PDIM")==0)||(strcmp(Command,"PDIM_H")==0)||
	(strcmp(Command,"PSINGLE")==0)||(strcmp(Command,"PSINGLE_H")==0)) {
      if ((strcmp(Command,"SINGLE")==0)||(strcmp(Command,"SINGLE_H")==0)||
	  (strcmp(Command,"PSINGLE")==0)||(strcmp(Command,"PSINGLE_H")==0)) {

#ifdef DEBUG
    	printf ("Single Command\n"); 
#endif

	if ((strcmp(Command,"SINGLE")==0)||(strcmp(Command,"PSINGLE")==0)) {
	  This->Command=S_SINGLE ;
	} else {
	  This->Command=S_SINGLEH ;
	} ;

	sscanf (Line,"%d %s %d %s",&(This->LineNumber),Command,&(This->Para),Val) ;
	if (Val[0]=='$') {
	  if (Val[1] == '$') {
	    This->LED = -10 ;
	  } else {
	    This->LED = -(Val[1]-'0') ;
	    if ((This->LED<-10)||(This->LED>0)) This->LED=0 ;
	  } ;
	} else {
	  sscanf (Val,"%d",&(This->LED)) ;
	} ;
	j = 4 ;
      } else {
#ifdef DEBUG
    	printf ("Dim Command\n"); 
#endif
	if ((strcmp(Command,"DIM")==0)||(strcmp(Command,"PDIM")==0)) {
	  This->Command=S_DIM ;
	} else {
	  This->Command=S_DIMH ;
	} ;
	j = 3 ;
      } ;
      
      ReadTrippleVals (Line,j,This) ;

      if (This->DataLen%3!=0) {
	fprintf (stderr,"Data not in RGB format in Sequence %s, Line %d\n",Name,This->LineNumber) ;
      } ;
      
      // Entsprechendes Delay hinzufuegen
      if (Command[0]!='P') {
#ifdef DEBUG
    	printf ("Adding Delay\n"); 
#endif
	This->Next = malloc(sizeof(struct Sequence)) ;
	That=This->Next ;
	if (That==NULL) {
	  fprintf (stderr,"Out of memory\n") ;
	  exit(0) ;
	} ;
	That->Command=S_DELAY ;
	That->LineNumber = This->LineNumber ;
	That->Para = This->Para ;
	This = That ;
      } ;
    } else if (strcmp(Command,"DELAY")==0) {
#ifdef DEBUG
    	printf ("Delay Command\n"); 
#endif
      This->Command=S_DELAY ;
    } else if (strcmp(Command,"GOTO")==0) {
#ifdef DEBUG
    	printf ("Goto Command\n"); 
#endif
      This->Command=S_GOTO ;
    } else if (strcmp(Command,"COUNT_UP")==0) {
#ifdef DEBUG
    	printf ("Count up Command\n"); 
#endif
      This->Command=S_COUNTUP ;
    } else if (strcmp(Command,"COUNT_DOWN")==0) {
#ifdef DEBUG
    	printf ("Count down Command\n"); 
#endif
      This->Command=S_COUNTDOWN ;
    } else if (strcmp(Command,"COUNT_END")==0) {
#ifdef DEBUG
    	printf ("Count end Command\n"); 
#endif
      This->Command=S_COUNTEND ;
    } else if (strcmp(Command,"SET_VAR")==0) {
#ifdef DEBUG
    	printf ("Set Var Command\n"); 
#endif
      This->Command=S_SETVAR ;
      sscanf (Line2,"%d %s %d %s",&(This->LineNumber),Command,&(This->Para),UnitName) ;
      This->GlobalVar = FindNode(Haus->Child,UnitName) ;
      if (This->GlobalVar==NULL) printf ("Unknown variable: %s\n",UnitName) ;
    } ;
  } ;
}

#define LEFT_ASSOC 0
#define RIGHT_ASSOC 1

struct Operator {
  char Op ;
  char Prec ;
  char Assoc ;
} ;

struct Operator Operators[] = {
  {'=',0,RIGHT_ASSOC},
  {'>',0,RIGHT_ASSOC},
  {'<',0,RIGHT_ASSOC},
  {'+',5,LEFT_ASSOC},
  {'-',5,LEFT_ASSOC},
  {'|',5,LEFT_ASSOC},
  {'&',5,LEFT_ASSOC},
  {'*',10,LEFT_ASSOC},
  {'/',10,LEFT_ASSOC},
  {'%',10,LEFT_ASSOC},
  {'^',15,RIGHT_ASSOC},
  {'\0',0,LEFT_ASSOC}
} ;

const char *CalcFunctions[]={
  "sin",
  "cos",
  "tan",
  "sqrt",
  "sqr",
  "rnd",
  "not",
  "nicht",
  ""
} ;

int IsOperator (char* Token)
{
  int i ;
  for (i=0;Operators[i].Op!='\0';i++) if (Operators[i].Op==Token[0]) break ;
 
  if (Operators[i].Op=='\0') return (-1) ;

  if ((Token[0]=='-')&&isdigit(Token[1])) return (-1) ; // Negative Zahl

  return (i) ;
}

int IsAssociative (char *Expression, char *Token, char Type)
{
  int i ;
  
  i = IsOperator (Token) ;
  if (i==-1) {
    printf ("Invalid Token %s in Expression %s\n",Token,Expression) ;
    return (-1) ;
  }
  return (Operators[i].Assoc==Type) ;
}

int ComparePrec (char *Expression, char *Token1, char *Token2) 
{
  int i,j ;
  if ((i=IsOperator(Token1))==-1) {
    printf ("Invalid Token %s in Expression %s\n",Token1,Expression) ;
    return (-1) ;
  } ;
  if ((j=IsOperator(Token2))==-1) {
    printf ("Invalid Token %s in Expression %s\n",Token2,Expression) ;
    return (-1) ;
  } ;
  return (Operators[i].Prec-Operators[j].Prec) ;
} 

char CalcStack [NAMELEN*8] ;
char *CalcStackPointer ;
char OutStack [NAMELEN*8] ;
char *OutStackPointer ;

void CalcPush (char *Exp,char *CS,char **CSP)
{
  strcpy (*CSP,Exp) ;
  (*CSP) += strlen(Exp)+1 ;
} 

char *CalcPop (char *CS, char **CSP)
{
  if ((*CSP)==CS) return (CS) ;
  (*CSP)-- ;
  for (;(*CSP)[-1]!='\0'&&((*CSP)!=CS);(*CSP)--) ;
  return (*CSP) ;
}

int CalcStackEmpty(char *CS, char **CSP)
{
  return (*CSP==CS) ;
}

char *CalcPeek (char *CS, char **CSP)
{
  char *a ;
  if ((*CSP)==CS) return (CS) ;
  for (a=(*CSP)-1;a[-1]!='\0'&&(a!=CS);a--) ;
  return (a) ;
}

int TokenPointer ;

int iswhitespace(char a)
{
  return ((a=='\n')||(a=='\t')||(a==' ')||(a=='\r')) ;
} 

int GetNextToken(char *Expression, char *Token)
{
  int i,j ;
  /* Leerzeichen ueberspringen */
  for (;iswhitespace(Expression[TokenPointer]);TokenPointer++) ;

  if (Expression[TokenPointer]=='\0') return (-1) ;

  i = IsOperator(&(Expression[TokenPointer])) ;
  if (i<0) {
    if (Expression[TokenPointer]=='(') {
      Token[0] = Expression[TokenPointer] ;
      Token[1] = 0 ;
      i=20 ;
    } else if (Expression[TokenPointer]==')') {
      Token[0] = Expression[TokenPointer] ;
      Token[1] = 0 ;
      i=21 ;
    } else {
      for (j=0;(!iswhitespace(Expression[TokenPointer])&&
		((IsOperator(&(Expression[TokenPointer]))<0)||(Expression[TokenPointer]=='/'))&&
		(Expression[TokenPointer]!='(')&&(Expression[TokenPointer]!=')')&&(Expression[TokenPointer]!='\0'));
	   TokenPointer++,j++) Token[j]=Expression[TokenPointer] ;
      Token[j]='\0' ;
      if (Expression[TokenPointer]=='(') {
	i=23 ;
      } else {
	TokenPointer-- ;
	i=22 ;
      } ;
    } ;
  } else {
    Token[0] = Expression[TokenPointer] ;
    Token[1] = 0 ;
  } ;
  TokenPointer++ ; 
  return (i);
}

int IsFunction (char *Token)
{
  int i;
  
  for (i=0;strlen(CalcFunctions[i])>0;i++) if (strcmp(Token,CalcFunctions[i])==0) break ;
  if (strlen(CalcFunctions[i])==0) return FALSE ;
  return TRUE ;
}

int CalcVar (char *Name)
{
  struct Node *Var ;
  struct tm *tim ;
  time_t CurrentTime ;
  
  if ((strcmp(Name,"heute")==0)||(strcmp(Name,"today")==0)) {
    time (&CurrentTime) ;
    tim = gmtime(&CurrentTime) ;
    return (tim->tm_wday) ;
  } ;
  if ((strcmp(Name,"sonntag")==0)||(strcmp(Name,"sunday")==0)) {
    return(0) ;
  } ;
  if ((strcmp(Name,"montag")==0)||(strcmp(Name,"monday")==0)) {
    return(1) ;
  } ;
  if ((strcmp(Name,"dienstag")==0)||(strcmp(Name,"tuesday")==0)) {
    return(2) ;
  } ;
  if ((strcmp(Name,"mittwoch")==0)||(strcmp(Name,"wensday")==0)) {
    return(3) ;
  } ;
  if ((strcmp(Name,"donnerstag")==0)||(strcmp(Name,"thursday")==0)) {
    return(4) ;
  } ;
  if ((strcmp(Name,"freitag")==0)||(strcmp(Name,"friday")==0)) {
    return(5) ;
  } ;
  if ((strcmp(Name,"samstag")==0)||(strcmp(Name,"saturday")==0)) {
    return(6) ;
  } ;

  Var = FindNode(Haus->Child,Name) ;
  if (Var!=NULL) {
    return(Var->Value) ;
  } else {
    printf ("Unknown variable: %s\n",Name) ;
    return(0) ;
  } ;
}

int CalcValue (char *Expression)
{
  int i ;
  char Token[NAMELEN]; 
  char *NextItem ;
  int Stack[NAMELEN] ;
  int SP ;
  char Tok ;
  
  for (i=0;i<NAMELEN*8;i++) CalcStack[i]=0 ;
  CalcStackPointer = CalcStack ;
  for (i=0;i<NAMELEN*8;i++) OutStack[i]=0 ;
  OutStackPointer = OutStack ;
  TokenPointer = 0 ;

  for (;(Tok=GetNextToken(Expression,Token))>=0;) {
    if (Tok<20) {
      while (!CalcStackEmpty(CalcStack,&CalcStackPointer) && 
	     ((IsOperator(CalcPeek(CalcStack,&CalcStackPointer))>=0))) {
	if (((IsAssociative(Expression,Token,LEFT_ASSOC)&&ComparePrec(Expression,Token,CalcPeek(CalcStack,&CalcStackPointer))<=0)||
	     (IsAssociative(Expression,Token,RIGHT_ASSOC)&&ComparePrec(Expression,Token,CalcPeek(CalcStack,&CalcStackPointer))<0))) {
	  CalcPush(CalcPop(CalcStack,&CalcStackPointer),OutStack,&OutStackPointer) ;
	  continue ;
	} ;
	break ;
      } ;
      CalcPush(Token,CalcStack,&CalcStackPointer) ;
    } else if (Tok==20) {
      CalcPush(Token,CalcStack,&CalcStackPointer) ;
    } else if (Tok==21) {
      while (!CalcStackEmpty(CalcStack,&CalcStackPointer)&&strcmp(CalcPeek(CalcStack,&CalcStackPointer),"(")) {
	CalcPush(CalcPop(CalcStack,&CalcStackPointer),OutStack,&OutStackPointer) ;
      } ;
      CalcPop(CalcStack,&CalcStackPointer) ;
      if(IsFunction(CalcPeek(CalcStack,&CalcStackPointer))) {
	CalcPush(CalcPop(CalcStack,&CalcStackPointer),OutStack,&OutStackPointer) ;
      } ;
    } else if (Tok==23) {
      CalcPush(Token,CalcStack,&CalcStackPointer) ;
      CalcPush("(",CalcStack,&CalcStackPointer) ;
    } else {
      CalcPush(Token,OutStack,&OutStackPointer) ;
    } 
  } ;
  while (!CalcStackEmpty(CalcStack,&CalcStackPointer)) CalcPush(CalcPop(CalcStack,&CalcStackPointer),OutStack,&OutStackPointer) ;
  
  SP=-1 ;


  for (NextItem=OutStack;NextItem!=OutStackPointer;) {
    for (i=0;(*NextItem)!='\0';NextItem++,i++) Token[i]=(*NextItem) ;
    Token[i]='\0' ; NextItem++ ;

    if (IsOperator(Token)>=0) {
      switch (Token[0]) {
      case '=':
	Stack[SP-1]=(Stack[SP-1]==Stack[SP]) ;
	break ;
      case '>':
	Stack[SP-1]=(Stack[SP-1]>Stack[SP]) ;
	break ;
      case '<':
	Stack[SP-1]=(Stack[SP-1]<Stack[SP]) ;
	break ;
      case '+':
	Stack[SP-1]=(Stack[SP-1]+Stack[SP]) ;
	break ;
      case '-':
	Stack[SP-1]=(Stack[SP-1]-Stack[SP]) ;
	break ;
      case '|':
	Stack[SP-1]=(Stack[SP-1]||Stack[SP]) ;
	break ;
      case '&':
	Stack[SP-1]=(Stack[SP-1]&&Stack[SP]) ;
	break ;
      case '*':
	Stack[SP-1]=(Stack[SP-1]*Stack[SP]) ;
	break ;
      case '/':
	Stack[SP-1]=(Stack[SP-1]/Stack[SP]) ;
	break ;
      case '%':
	Stack[SP-1]=(Stack[SP-1]%Stack[SP]) ;
	break ;
      case '^':
	break ;
      default:
	printf ("Unknown Opcode") ;
	break ;
      }; 
      SP-- ;
    } else if (IsFunction(Token)) {
      if (strcmp(Token,"sin")==0) {
	Stack[SP]=(int)(1000.0*sin(((double)Stack[SP])*3.1415/180)+0.5) ;
      } else if (strcmp(Token,"cos")==0) {
	Stack[SP]=(int)(1000.0*cos(((double)Stack[SP])*3.1415/180)+0.5) ;
      } else if (strcmp(Token,"tan")==0) {
	Stack[SP]=(int)(1000.0*tan(((double)Stack[SP])*3.1415/180)+0.5) ;
      } else if (strcmp(Token,"sqrt")==0) {
	Stack[SP]=(int)sqrt((double)Stack[SP]) ;
      } else if (strcmp(Token,"sqr")==0) {
	Stack[SP]=Stack[SP]*Stack[SP] ;
      } else if (strcmp(Token,"rnd")==0) {
	Stack[SP]=(int)((double)random()*(double)Stack[SP]/(double)RAND_MAX) ;
      } else if (strcmp(Token,"not")==0) {
	Stack[SP]=!(Stack[SP]) ;
      } else if (strcmp(Token,"nicht")==0) {
	Stack[SP]=!(Stack[SP]) ;
      } else {
	printf ("Unknown function\n") ;
      } ;
    } else if (isalpha(Token[0])) {
      // Variable einlesen
      SP++ ;
      Stack[SP] = CalcVar (Token) ;
    } else {
      SP++ ;
      sscanf (Token,"%d",&Stack[SP]) ;
    } ;
  }
  return (Stack[0]) ;
}  

#define pi 3.1415926536

double SDec(double Day)
{
  return (0.409526325277017*sin(0.0169060504029192*(Day-80.0856919827619)));
}

double TimeDiff(double dec, double lat)
{
  return (12.0*acos((sin(-0.0145444074) - sin(lat)*sin(dec)) / (cos(lat)*cos(dec)))/pi);
}

double timeeq(double Day)
{
  return (-0.170869921174742*sin(0.0336997028793971 * Day + 0.465419984181394) - 0.129890681040717*sin(0.0178674832556871*Day - 0.167936777524864));
}

double CalcSunrise(double Day, double lat)
{
  double dec ;
  dec = SDec(Day) ;
  return (12-TimeDiff(dec,lat)-timeeq(Day)) ;
}

double CalcSunset(double Day, double lat)
{
  double dec ;
  dec = SDec(Day) ;
  return (12+TimeDiff(dec,lat)-timeeq(Day)) ;
}

void CalcSun (void)
{
  time_t CurrentTime ;
  struct tm *tim ;
  double TimeZone ;
  double Hour ;
  double Day ;

  time (&CurrentTime) ;
  

  tim = gmtime(&CurrentTime) ;
  TimeZone = (double)tim->tm_hour ;
  tim = localtime(&CurrentTime) ;
  TimeZone = (double)tim->tm_hour-TimeZone ;
  if ((TimeZone<0)&&(West<0)) TimeZone+=24 ;
  if ((TimeZone>0)&&(West>=0)) TimeZone-=24 ;
  Day = (double)tim->tm_yday ;
  
  SunRise.tm_mday = SunSet.tm_mday = tim->tm_mday ;
  SunRise.tm_mon = SunSet.tm_mon = tim->tm_mon ;
  SunRise.tm_year = SunSet.tm_year = tim->tm_year ;
  SunRise.tm_wday = SunSet.tm_wday = tim->tm_wday ;
  SunRise.tm_yday = SunSet.tm_yday = tim->tm_yday ;
  SunRise.tm_isdst = SunSet.tm_isdst = tim->tm_isdst ;
  Hour = CalcSunrise(Day,North*pi/180.0)+West/15.0+TimeZone ;
  SunRise.tm_hour = (int)Hour ;
  SunRise.tm_min = (int)((Hour-(double)SunRise.tm_hour)*60) ;
  SunRise.tm_sec = 0 ;
  Hour = CalcSunset(Day,North*pi/180.0)+West/15.0+TimeZone ;
  SunSet.tm_hour = (int)Hour ;
  SunSet.tm_min = (int)((Hour-(double)SunSet.tm_hour)*60) ;
  SunSet.tm_sec = 0 ;
}
  

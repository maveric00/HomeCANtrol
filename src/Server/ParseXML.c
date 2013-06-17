#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <expat.h>
#include <string.h>
#include <locale.h>
#include <ctype.h>
#include "libwebsocket/libwebsockets.h"
#include "ConfigNodes.h"
#include "XMLConfig.h"
#define SERVER_INCLUDE 1
#include "../Apps/Common/mcp2515.h"
#include "Network.h"

#define BUFF_SIZE 10240

struct Node *Haus=NULL ;
struct Node *Current=NULL ;
struct SeqList *Sequences=NULL ;
struct NodeList *Reactions=NULL ;

int MakroNummer;
int Depth;

struct TypSel Types[] = {
  {"Haus",N_STRUCTURE},
  {"Home",N_STRUCTURE},
  {"Etage",N_STRUCTURE},
  {"Floor",N_STRUCTURE},
  {"Zimmer",N_STRUCTURE},
  {"Room",N_STRUCTURE},
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
  {"Wenn",N_IF},
  {"If",N_IF},
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
  {"Einfach",S_SIMPLE},
  {"Simple",S_SIMPLE},
  {"KurzLang",S_SHORTLONG},
  {"ShortLong",S_SHORTLONG},
  {"Rollo_KL",S_SHADE_SHORTLONG},
  {"Shade_SL",S_SHADE_SHORTLONG},
  {"Rollo_Einfach",S_SHADE_SIMPLE},
  {"Shade_Simple",S_SHADE_SIMPLE},
  {"Monoflop",S_MONO},
  {"BWM",S_BWM},
  {"Movement",S_BWM},
  {"R_Monoflop",S_RETMONO},
  {"Analog",S_ANALOG},
  {"Ausgang",S_OUTPUT},
  {"Output",S_OUTPUT},
  {"WSData",S_WSDATA},
  {"WSClock",S_WSCLOCK},
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
      Reactions = malloc (sizeof (NodeList)) ;
      Reactions->Next = NULL ;
      Reactions->Node = Current ;
    } else {
      for (NL=Reactions;NL->Next!=NULL;NL=NL->Next) ;
      NL->Next = malloc (sizeof(NodeList)) ;
      NL = NL->Next ;
      NL->Next = NULL ;
      NL->Node = Current ;
    } ;
  } ;

  
  for (i = 0; attr[i]; i += 2) {
    if (strcmp(attr[i],"name")==0) {
      strncpy(Current->Name,attr[i+1],NAMELEN) ;
      continue; 
    } ;
    if ((strcmp(attr[i],"wert")==0)||(strcmp(attr[i],"value")==0)) {
      sscanf(attr[i+1],"%d",&(Current->Value)) ;
    } ;
    switch (Current->Type) {
    case N_ADRESS:
      if ((strcmp(attr[i],"linie")==0)||(strcmp(attr[i],"line")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Linie)) ;
      } ;
      if ((strcmp(attr[i],"knoten")==0)||(strcmp(attr[i],"node")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Knoten)) ;
      } ;
      if (strcmp(attr[i],"port")==0) {
	sscanf(attr[i+1],"%d",&(Current->Data.Adresse.Port)) ;
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
      break ;
    case N_SENSOR:
    case N_SENS2:
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
      if ((strcmp(attr[i],"intervall")==0)||(strcmp(attr[i],"interval")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Sensor.Intervall)) ;
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
	strncpy(Current->Data.Wert.UnitName,attr[i+1],NAMELEN*4) ;
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
      if ((strcmp(attr[i],"objekt")==0)||(strcmp(attr[i],"object")==0)) {
	strncpy(Current->Data.Wert.UnitName,attr[i+1],NAMELEN*4) ;
      } ;
      if ((strcmp(attr[i],"wert")==0)||(strcmp(attr[i],"value")==0)) {
	sscanf(attr[i+1],"%d",&(Current->Data.Wert.Wert)) ;
      } ;
      if ((strcmp(attr[i],"vergleich")==0)||(strcmp(attr[i],"comparison")==0)) {
	switch (attr[i+1][0]) {
	case '<':
	  Current->Data.Wert.Vergleich = -1 ;
	  break ;
	case '>':
	  Current->Data.Wert.Vergleich = 1 ;
	  break ;
	case '=':
	default:
	  Current->Data.Wert.Vergleich = 0 ;
	  break ;
	} ;
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
	sscanf(attr[i+1],"%i %i %i %i %i %i %i %i",
	       &(Current->Data.Reaction.Data[0]),&(Current->Data.Reaction.Data[1]),
	       &(Current->Data.Reaction.Data[2]),&(Current->Data.Reaction.Data[3]),
	       &(Current->Data.Reaction.Data[4]),&(Current->Data.Reaction.Data[5]),
	       &(Current->Data.Reaction.Data[6]),&(Current->Data.Reaction.Data[7])) ;
      } ;	
      if ((strcmp(attr[i],"daten_maske")==0)||(strcmp(attr[i],"data_mask")==0)) {
	for (j=0;j<8;j++) Current->Data.Reaction.DataMask[j] = 0  ;
	sscanf(attr[i+1],"%i %i %i %i %i %i %i %i",
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
    Val[2] = Line[i] ;
    Val[3] = Line[i+1] ;
    sscanf (Val,"%hhx",&(This->Data[i/2])) ;
  } ;
  This->DataLen = i/2 ;
}

void hsv_to_rgb (unsigned char h, unsigned char s, unsigned char v,unsigned char *r, unsigned char *g, unsigned char *b)
{
  unsigned char i, f;
  unsigned int p, q, t;
  
  if( s == 0 ) {	
    *r = *g = *b = v;
  } else {
    i=h/43;
    f=h%43;
    p = (v * (255 - s))/256;
    q = (v * ((10710 - (s * f))/42))/256;
    t = (v * ((10710 - (s * (42 - f)))/42))/256;
    
    switch( i ) {
    case 0:
      *r = v; *g = t; *b = p; 
      break;
    case 1:
      *r = q; *g = v; *b = p; 
      break;
    case 2:
      *r = p; *g = v; *b = t; 
      break;
    case 3:
      *r = p; *g = q; *b = v; 
      break;			
    case 4:
      *r = t; *g = p; *b = v; 
      break;				
    case 5:
      *r = v; *g = p; *b = q; 
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
  char Command[NAMELEN] ;
  char Val[10] ;
  int i,j ;

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

  while (fgets(Line,NAMELEN,InFile)!=NULL) {
    // Kommentarzeilen ausblenden
    if (Line[0]=='%') continue ;

    // Ganze Zeile zu Grossbuchstaben wandeln

    for (i=0;i<strlen(Line);i++) Line[i]=toupper(Line[i]); 

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
    for (i=0;i<MAX_WSLEDS*3;i++) This->Data[i]=0 ;
    
    // Lese Zeilennummer, Kommando und Parameter ein
    sscanf (Line,"%d %s %d",&(This->LineNumber),Command,&(This->Para)) ;
    if ((strcmp(Command,"DIM")==0)||(strcmp(Command,"DIM_H")==0)||
	(strcmp(Command,"SINGLE")==0)||(strcmp(Command,"SINGLE_H")==0)) {
      if ((strcmp(Command,"SINGLE")==0)||(strcmp(Command,"SINGLE_H")==0)) {
	This->Command=S_SINGLE ;
	sscanf (Line,"%d %s %d %s",&(This->LineNumber),Command,&(This->Para),Val) ;
	if (Val[0]=='$') {
	  This->LED = -1 ;
	} else {
	  sscanf (Val,"%d",&(This->LED)) ;
	} ;
	j = 4 ;
      } else {
	This->Command=S_DIM ;
	j = 3 ;
      } ;
      
      ReadTrippleVals (Line,j,This) ;

      if (This->DataLen%3!=0) {
	fprintf (stderr,"Data not in RGB format in Sequence %s, Line %d\n",Name,This->LineNumber) ;
      } ;

      if ((strcmp(Command,"DIM_H")==0)||(strcmp(Command,"SINGLE_H")==0)) {
	// HSV-Werte nach RGB wandeln
	for (i=0;i<This->DataLen;i+=3) 
	  hsv_to_rgb (This->Data[i],This->Data[i+1],This->Data[i+2],&(This->Data[i]),&(This->Data[i+1]),&(This->Data[i+2])) ;
      } ;

      // Entsprechendes Delay hinzufuegen
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
    } else if (strcmp(Command,"DELAY")==0) {
      This->Command=S_DELAY ;
    } else if (strcmp(Command,"GOTO")==0) {
      This->Command=S_GOTO ;
    } else if (strcmp(Command,"COUNT_UP")==0) {
      This->Command=S_COUNTUP ;
    } else if (strcmp(Command,"COUNT_DOWN")==0) {
      This->Command=S_COUNTDOWN ;
    } else if (strcmp(Command,"COUNT_END")==0) {
      This->Command=S_COUNTEND ;
    } ;
  } ;
}

  

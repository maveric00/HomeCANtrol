#include <sys/time.h>
#define NAMELEN 255
#define MAX_ADD_PER_NODE 10
#define MAX_WSLEDS 20

typedef enum {
  WS_UNDEF = 0,
  WS_SEND_STATUS = 1,
  WS_SEND_ALL_STATUS = 2
} tWsSend ;

typedef enum {
  S_NOP = 0,
  S_DIM = 1,
  S_GOTO = 2,
  S_DELAY = 3,
  S_SINGLE = 4,
  S_COUNTUP = 5,
  S_COUNTDOWN = 6,
  S_COUNTEND = 7,
  S_SETVAR = 8,
  S_SINGLEH = 9,
  S_DIMH = 10
} tSeqCom ;


typedef enum {
  N_UNDEF = 0,
  N_STRUCTURE = 1,
  N_ADRESS = 2,
  N_ONOFF = 3,
  N_SHADE = 4,
  N_SENSOR = 5,
  N_ACTION = 6,
  N_MACRO = 7,
  N_DELAY = 8,
  N_TIMER = 9,
  N_CALL = 10,
  N_TASK = 11,
  N_ALWAYS = 12,
  N_ACTIVE = 13,
  N_STARTUP = 14,
  N_IF = 15,
  N_VAR = 16,
  N_SET = 17,
  N_REPEAT = 18,
  N_LANGUAGE = 19,
  N_PORT = 20,
  N_BROADCAST = 21,
  N_FIRMWARE = 22,
  N_BAD = 23,
  N_PROGRAM = 24,
  N_SEQUENCE = 25,
  N_WAITFOR = 26,
  N_LED = 27,
  N_SENS2 = 28,
  N_REACT = 29,
  N_ELSE = 30,
  N_LOC = 31,
  S_SIMPLE = 100,
  S_SHORTLONG = 101,
  S_SHADE_SHORTLONG = 102,
  S_SHADE_SIMPLE = 103,
  S_MONO = 104,
  S_RETMONO = 105,
  S_ANALOG = 106,
  S_OUTPUT = 107,
  S_WSDATA = 108,
  S_WSCLOCK = 109,
  S_BWM = 110,
  S_BWM2 = 111,
  S_LIGHT = 112,
  S_PWM = 113,
  S_EXTENDED = 114,
  A_ON = 200,
  A_OFF = 201,
  A_TOGGLE = 202,
  A_SHADE_UP_FULL = 203,
  A_SHADE_DOWN_FULL = 204,
  A_SHADE_UP_SHORT = 205,
  A_SHADE_DOWN_SHORT = 206,
  A_SHADE_TO = 207,
  A_SEND_VAL = 208,
  A_HEARTBEAT = 209,
  A_CALL = 210,
  A_SEQUENCE = 211,
  A_LEDSET = 212,
  A_LEDHSET = 213,
  A_LEDDIM = 214,
  A_LEDHDIM = 215,
  A_STARTLED = 216,
  A_STOPLED = 217,
  A_PROGLED = 218
} NodeType ;

typedef enum {
  W_MACRO=1,
  W_DELAY=2,
  W_TIME=3,
  W_VALUE=4
} WaitType ;


struct TypSel {
  char *Name ;
  NodeType Type ;
} ;

struct AdInfo {
  int Linie ;
  int Knoten ;
  int Port ;
} ;

struct Aktion {
  int Type ;
  int StandAlone ;
  int Short ;
  char UnitName[NAMELEN*4] ;
  struct Node *Unit ;
  char Sequence[NAMELEN] ;
  unsigned char R ;
  unsigned char G ;
  unsigned char B ;
  unsigned char W ;
  unsigned char Delay ;
  unsigned char Step ;
} ;

struct Werte {
  char UnitName[NAMELEN*4] ;
  char Wert[NAMELEN*2] ;
} ;

struct Program {
  unsigned char Port ;
  unsigned char Data[50] ;
} ;

struct Sensor {
  NodeType SensorTyp ;
  int Lang ;
  int Ende ;
  int Reset ;
  int Intervall ;
} ;

struct Rollo {
  int Lang ;
  int Kurz ;
  int Swap ;
} ;

struct Reaction {
  struct AdInfo From ;
  struct AdInfo FromMask ;
  struct AdInfo To ;
  struct AdInfo ToMask ;
  unsigned char Data[8] ;
  unsigned char DataMask[8] ;
} ;

struct Node {
  struct Node *Parent ;
  struct Node *Next ;
  struct Node *Prev ;
  struct Node *Child ;
  
  NodeType Type ;
  char TypeDef[NAMELEN] ;
  char Name[NAMELEN] ;
  int Value ;
  union {
    struct Sensor Sensor ;
    struct Rollo Rollo ;
    struct AdInfo Adresse ;
    struct Aktion Aktion ;
    struct Node *MakroStep ;
    struct timeval Time ;
    struct Program Program ;
    char UnitName[NAMELEN*4] ;
    char PAD[NAMELEN*6] ;
    struct Werte Wert ;
    struct Reaction Reaction ;
  } Data ;
} ;

struct ListItem {
  struct ListItem *Next ;
  struct ListItem *Prev ;
  int Number ;
  int Counter ;
  char Linie ;
  unsigned char Package ;
  char State ;
  unsigned short Knoten ;
  union {
    unsigned char Command[NAMELEN*4] ;
    struct EEPROM EEprom ;
    unsigned char *Code ;
  } Data ;
} ;

struct MacroList {
  struct Node *Macro ;
  WaitType DelayType ;
  union {
    struct Node *WaitNode ;
    struct timeval WaitTime ;
  } Delay ;
} ;

struct NodeList {
  struct NodeList *Next ;
  struct Node *Node ;
} ;

struct Sequence {
  struct Sequence *Next ;
  int LineNumber ;
  tSeqCom Command ;
  int Para;
  int LED ;
  int CurrVal ;
  int DataLen ;
  struct Node *GlobalVar ;
  unsigned char Data[MAX_WSLEDS*3] ;
  unsigned char Var[MAX_WSLEDS*3] ;
} ;

struct SeqList {
  struct SeqList *Next ;
  struct Sequence *First ;
  struct Sequence *Current;
  struct Node *Action ;
  int Counter ;
  int LineNumber ;
  int UpDown ;
  int DataLen ;
  unsigned char Data[MAX_WSLEDS*3] ;
  unsigned char Vars[10] ;
  char Name[NAMELEN] ;
} ;

extern struct Node *Haus ;
extern struct Node *DefaultFloor ;
extern struct SeqList *Sequences ;
extern struct NodeList *Reactions ;
extern double West,North ;
extern struct tm SunSet ;
extern struct tm SunRise ;
extern int Verbose ;
extern int NoTime ;
extern FILE *logfd ;

// Node.c-Definitionen
struct Node *CreateNode (void);
void FreeNode (struct Node *This) ;
struct Node *NewChild (struct Node *This) ;
struct Node *FindNode (struct Node *Root,const char *Unit);
int FindGlobalNode (struct Node *Root, char *Unit);
void FullObjectName(struct Node *Node, char *Name) ;
int CollectAdress (struct Node *Root, int Linie, int Knoten, struct Node *Result[], int *ResultNumber ) ;
int CollectType (struct Node *Root, NodeType Type, struct Node *Result[], int *ResultNumber ) ;
struct Node *FindNodeAdress (struct Node *Root,int Linie, int Knoten, int Port,struct Node *Except);
int GetNodeAdress (struct Node *Node, int *Line, int *Knoten, int *Port) ;
void FreeItem (struct ListItem *This) ;
struct ListItem *CreateItem (struct ListItem* Head) ;

// ParseXML.c-Definitionen
int ReadConfig(void) ;
void ReadSequence (char *Name, char *FileName) ;
void hsv_to_rgb (unsigned char h, unsigned char s, unsigned char v,unsigned char *r, unsigned char *g, unsigned char *b);
int CalcValue (char *Expression);
void CalcSun (void) ;

// Server.c-Definitionen
void ExecuteMakro (struct Node *Makro);
void ExecuteSeq (struct Node *Action) ;
int HandleCommand (char *Command,int Socket) ;
char *LogTime(void)  ;
char *ToCommand(int Command) ;

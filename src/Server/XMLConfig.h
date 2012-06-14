#include <sys/time.h>
#define NAMELEN 255
#define MAX_ADD_PER_NODE 10


typedef enum {
	// every bootloader type has this commands
	SEND_STATUS             = 1,
	READ_CONFIG		= 2,
	WRITE_CONFIG	        = 3,
	READ_VAR		= 4,
	SET_VAR			= 5,
	START_BOOT		= 6,
	TIME			= 7,
	// LED commands
	LED_OFF			= 10,
	LED_ON			= 11,
	SET_TO			= 12,
	HSET_TO			= 13,
	L_AND_S			= 14,
	SET_TO_G1		= 15,
	SET_TO_G2		= 16,
	SET_TO_G3		= 17,
	LOAD_LOW		= 18,
	LOAD_MID1		= 19,
	LOAD_MID2		= 20,
	LOAD_HIGH		= 21,
	START_PROG		= 22,
	STOP_PROG		= 23,
	// Relais commands
	CHANNEL_ON		= 30,
	CHANNEL_OFF             = 31,
	CHANNEL_TOGGLE          = 32,
	SHADE_UP_FULL           = 33,
	SHADE_DOWN_FULL         = 34,
	SHADE_UP_SHORT          = 35,
	SHADE_DOWN_SHORT        = 36,
	// Sensor commands
	SET_PIN                 = 40,
        LOAD_LED                = 41,
	OUT_LED                 = 42,
	REQUEST			= 0x00,
	SUCCESSFULL_RESPONSE	= 0x40,
	ERROR_RESPONSE		= 0x80,
	NO_MESSAGE		= 0x3f
} tCommand;

typedef enum {
  WS_UNDEF = 0,
  WS_SEND_STATUS = 1,
  WS_SEND_ALL_STATUS = 2
} tWsSend ;


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
  S_SIMPLE = 100,
  S_SHORTLONG = 101,
  S_SHADE_SHORTLONG = 102,
  S_SHADE_SIMPLE = 103,
  S_MONO = 104,
  S_RETMONO = 105,
  A_ON = 200,
  A_OFF = 201,
  A_TOGGLE = 202,
  A_SHADE_UP_FULL = 203,
  A_SHADE_DOWN_FULL = 204,
  A_SHADE_UP_SHORT = 205,
  A_SHADE_DOWN_SHORT = 206,
  A_SHADE_TO = 207,
  A_SEND_VAL = 208,
  A_HEARTBEAT = 209
} NodeType ;

typedef enum {
  W_MACRO=1,
  W_DELAY=2,
  W_TIME=3
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
  char UnitName[NAMELEN*4] ;
  struct Node *Unit ;
} ;

struct Werte {
  char UnitName[NAMELEN*4] ;
  int Wert ;
  int Vergleich ;
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
    NodeType SensorTyp ;
    struct AdInfo Adresse ;
    struct Aktion Aktion ;
    struct Node *MakroStep ;
    struct timeval Time ;
    char UnitName[NAMELEN*4] ;
    char PAD[NAMELEN*5] ;
    struct Werte Wert ;
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

extern struct Node *Haus ;

// Node.c-Definitionen
struct Node *CreateNode (void);
void FreeNode (struct Node *This) ;
struct Node *NewChild (struct Node *This) ;
struct Node *FindNode (struct Node *Root,const char *Unit);
int CollectAdress (struct Node *Root, int Linie, int Knoten, struct Node *Result[], int *ResultNumber ) ;
struct Node *FindNodeAdress (struct Node *Root,int Linie, int Knoten, int Port,struct Node *Except);
int GetNodeAdress (struct Node *Node, int *Line, int *Knoten, int *Port) ;

// ParseXML.c-Definitionen
int ReadConfig(void) ;


// Server.c-Definitionen
void ExecuteMakro (struct Node *Makro);

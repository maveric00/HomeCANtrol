
/*
** Config.c -- configure Nodes via CAN
*/

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include "Config.h"

#define CANBUFLEN 20
#define CAN_PORT "13247"
#define CAN_PORT_NUM 13247
#define CAN_BROADCAST "192.168.69.255"

#define MAXLINE 520

#define DATA_RECORD	00	//record contain the data bytes
#define END_RECORD	01	//record mark end of file
#define SEG_ADDR_RECORD	02	//record contain the new segmented address (HEX86)
#define START_RECORD	03	//record contain the program entry point (HEX86)
#define	LIN_ADDR_RECORD	04	//record contain the new linear address (HEX386)
#define EXT_START_RECORD	05	//record contain the program entry point (HEX386)

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;

#define BUFFERSIZE 130000
#define OK 0
#define	BADPARAM	-2		       
#define	FILENOTFOUND	-3			
#define	CREATEERROR	-4		
#define	BADFILETYPE	-5		
#define	READERROR	-6		
#define	WRITEERROR	-7		
#define	NOTHINGTOSAVE	-8			
#define NOTSUPPORTED	-9			
#define	BUFFEROVERFLOW	-30
#define	OUTOFMEMORY	-31
#define BUFFERUNDERFLOW	-32
#define TRUE (0==0)
#define FALSE (0==1)

int FileSize ;
char FileBuffer[BUFFERSIZE] ;
int RecSockFD;
int SendSockFD;
struct addrinfo *servinfo, *SendInfo ;


/* Einzelne Hex-Zahlen einlesen */

inline ULONG BuildCANId (char Prio, char Repeat, char FromLine, USHORT FromAdd, char ToLine, USHORT ToAdd, char Group)
{
  return ((Group&0x1)<<1|ToAdd<<2|(ToLine&0xf)<<10|FromAdd<<14|(FromLine&0xf)<<22|(Repeat&0x1)<<26|(Prio&0x3)<<27) ;
}

inline char MatchAddress (ULONG CANId, char ToLine, USHORT ToAdd)
{
  ToLine = ToLine&0xf ;
  if (((CANId & (0xf<<10))==(ToLine<<10))&&(((CANId & (0xff<<2))==(ToAdd<<2)))) {
    return TRUE ;
  } else {
    return FALSE ;
  } ;
}

inline char MatchSrcAddress (ULONG CANId, char FromLine, USHORT FromAdd)
{
  FromLine = FromLine&0xf ;
  if (((CANId & (0xf<<22))==(FromLine<<22))&&(((CANId & (0xff<<14))==(FromAdd<<14)))) {
    return TRUE ;
  } else {
    return FALSE ;
  } ;
}

inline void GetSourceAddress (ULONG CANId, char *FromLine, USHORT *FromAdd)
{
  *FromLine = (char)((CANId>>22)&0xf) ;
  *FromAdd = (USHORT) ((CANId>>14)&0xff) ;
}

int ScanHex(char **sp, int len, USHORT *result)
{
	char cifra[20];
	int j;

	if (len > 4)
		return -2;
	for (j = 0; j < len && **sp; j++)
	{
		cifra[j] = *(*sp)++;
		if ( !isxdigit(cifra[j]) )
			return -1;
	}
	cifra[j] = '\0';
	*result = strtoul(cifra, NULL, 16);

	return 0;
}

int New_IHexRecord(int type, uint16_t address, const uint8_t *data, int dataLen, IHexRecord *ihexRecord) {
	/* Data length size check, assertion of ihexRecord pointer */
	if (dataLen < 0 || dataLen > IHEX_MAX_DATA_LEN/2 || ihexRecord == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
	
	ihexRecord->type = type;
	ihexRecord->address = address;
	memcpy(ihexRecord->data, data, dataLen);
	ihexRecord->dataLen = dataLen;
	ihexRecord->checksum = Checksum_IHexRecord(ihexRecord);
	
	return IHEX_OK;		
}

int Write_IHexRecord(const IHexRecord *ihexRecord, FILE *out) {
	int i;
	
	/* Check our record pointer and file pointer */
	if (ihexRecord == NULL || out == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
		
	/* Check that the data length is in range */
	if (ihexRecord->dataLen > IHEX_MAX_DATA_LEN/2)
		return IHEX_ERROR_INVALID_RECORD;
	
	/* Write the start code, data count, address, and type fields */
	if (fprintf(out, "%c%2.2X%2.4X%2.2X", IHEX_START_CODE, ihexRecord->dataLen, ihexRecord->address, ihexRecord->type) < 0)
		return IHEX_ERROR_FILE;
		
	/* Write the data bytes */
	for (i = 0; i < ihexRecord->dataLen; i++) {
		if (fprintf(out, "%2.2X", ihexRecord->data[i]) < 0)
			return IHEX_ERROR_FILE;
	}
	
	/* Calculate and write the checksum field */
	if (fprintf(out, "%2.2X\r\n", Checksum_IHexRecord(ihexRecord)) < 0)
		return IHEX_ERROR_FILE;
		
	return IHEX_OK;
}

uint8_t Checksum_IHexRecord(const IHexRecord *ihexRecord) {
	uint8_t checksum;
	int i;

	/* Add the data count, type, address, and data bytes together */
	checksum = ihexRecord->dataLen;
	checksum += ihexRecord->type;
	checksum += (uint8_t)ihexRecord->address;
	checksum += (uint8_t)((ihexRecord->address & 0xFF00)>>8);
	for (i = 0; i < ihexRecord->dataLen; i++)
		checksum += ihexRecord->data[i];
	
	/* Two's complement on checksum */
	checksum = ~checksum + 1;

	return checksum;
}

int LoadIHexFile (char *FileName, long relocation_offset)
{
	int rval = 0;
	int okline_counter = 0;
	u_char *endp;
	u_char *dp;
	int img_size = 0;
	char riga[MAXLINE+1];
	long laddr = 0;
	FILE *fh;
	USHORT bcount;
	USHORT addr;
	USHORT data;
	USHORT rectype;

	endp = FileBuffer + BUFFERSIZE;
	dp = FileBuffer;

	//Relocation check
	if (dp + relocation_offset > endp)
		return BADPARAM;
	else
		dp += relocation_offset;

	if ( (fh = fopen(FileName, "r")) == NULL )
		return FILENOTFOUND;

	riga[MAXLINE] = '\0';
	while ( fgets(riga, MAXLINE, fh) )
	{
		char *s;
		int k;

		if ( (s = strchr(riga, ':')) == NULL )
			continue;
		else
			s++;

		//Byte Count
		if ( ScanHex(&s, 2, &bcount) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		u_char checksum = (u_char)bcount;

		//Address
		if ( ScanHex(&s, 4, &addr) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		checksum += (u_char)(addr >> 8);
		checksum += (u_char)addr;

		//affect only low 16 bits of address
		laddr &= 0xFFFF0000;
		laddr |= addr;

		//Record Type
		if ( ScanHex(&s, 2, &rectype) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		checksum += (u_char)rectype;

		//Data Byte
		if (rectype == DATA_RECORD)
		{
			//buffer overflow
			if (dp+laddr+bcount > endp)
			{
				rval = BUFFEROVERFLOW;
				break;
			}

			char ok = TRUE;
			u_char *p;
			for (k = 0, p = dp+laddr; k < bcount && ok; k++)
			{
				if ( ScanHex(&s, 2, &data) != OK )
					ok = FALSE;

				checksum += (u_char)data;
				*p++ = (u_char)data;
			}
			if (!ok)	//Irgendwas war defekt
			{
				rval = BADFILETYPE;
				break;
			}
			img_size = laddr + bcount;
		}
		else if (rectype == SEG_ADDR_RECORD)
		{
			if (bcount != 2)
			{
				rval = BADFILETYPE;
				break;
			}
			else
			{
				//Address
				USHORT addr;
				if ( ScanHex(&s, 4, &addr) != OK )
				{
					rval = BADFILETYPE;
					break;
				}
				checksum += (u_char)(addr >> 8);
				checksum += (u_char)addr;

				laddr = (long)addr << 4;
			}
		}
		else if (rectype == LIN_ADDR_RECORD)
		{
			if (bcount != 2)
			{
				rval = BADFILETYPE;
				break;
			}
			else
			{
				//Address
				USHORT addr;
				if ( ScanHex(&s, 4, &addr) != OK )
				{
					rval = BADFILETYPE;
					break;
				}
				checksum += (u_char)(addr >> 8);
				checksum += (u_char)addr;

				laddr = (long)addr << 16;
			}
		}
		else	// Unknown record type: discard data bytes (but check for validity)
		{
			char ok = TRUE;
			while (bcount-- && ok)
			{
				if ( ScanHex(&s, 2, &data) != OK )
					ok = FALSE;
				checksum += (u_char)data;
			}
			if (!ok)
			{
				rval = BADFILETYPE;
				break;
			}
		}

		if ( ScanHex(&s, 2, &data) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		if ( (u_char)data != (u_char)(~checksum + 1) )
		{
			rval = BADFILETYPE;
			break;
		}
		else
			okline_counter++;

		if (rectype == END_RECORD)
		{
			break;
		}
	}

	fclose(fh);

	if (okline_counter == 0)
		rval = BADFILETYPE;
	else
	{
		if (img_size == 0)     
			img_size++;
	}


	if (rval == OK)
	{
		rval = img_size;
	}

	return rval;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int InitNetwork(void)
{

	struct addrinfo hints;
	int rv;
	struct sockaddr_in RecAddr;
	int val ;

	val = (0==0) ;
	// Reserve Receive-Socket
	if ((RecSockFD = socket(AF_INET, SOCK_DGRAM,0)) == -1) {
	  perror("talker: socket");
	  exit(0);
	}

	memset(&RecAddr, 0, sizeof(struct sockaddr_in));
	RecAddr.sin_family      = AF_INET;
	RecAddr.sin_addr.s_addr = INADDR_ANY;   // zero means "accept data from any IP address"
	RecAddr.sin_port        = htons(CAN_PORT_NUM);

	if (bind(RecSockFD, (struct sockaddr *) &RecAddr, sizeof(RecAddr)) == -1) {
	  close(RecSockFD);
	  perror("listener: bind");
	}

	setsockopt(RecSockFD, SOL_SOCKET, SO_BROADCAST, (char *) &val, sizeof(val)) ;

	// Set up send-Socket
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
} ;



int ReceiveCANMessage (ULONG *CANID, char *Len, char *Data)
{
  int i ;
  char *CANIDP ;
  char buf[CANBUFLEN];
  size_t addr_len;
  int numbytes ;
  struct sockaddr_storage their_addr;
  char s[INET6_ADDRSTRLEN];
  
  
  addr_len = sizeof their_addr;
  if ((numbytes = recvfrom(RecSockFD, buf, CANBUFLEN-1 , 0,
			   (struct sockaddr *)&their_addr, &addr_len)) == -1) {
    perror("recvfrom");
    exit(1);
  }


  CANIDP = (char*)CANID ;
  for (i=0;i<4;i++) CANIDP[i]=buf[i] ;
  *Len = buf[4];

  for (i=0;i<*Len;i++) Data[i] = buf[i+5] ;
  for (;i<8;i++) buf[i+5] = 0 ;
    
  return 0;
}

int SendCANMessage (ULONG CANID, char Len, char *Data)
{
  int i ;
  char Message [CANBUFLEN] ;
  char *CANIDP ;
  int numbytes;  

  /* Nachricht zusammensetzen */
  CANIDP = (char*)&CANID ;
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

void CloseNetwork (void) 
{
  freeaddrinfo(servinfo);

  close(RecSockFD);
  close(SendSockFD);
}

#define UPDATE_REQUEST 1
#define IDENTIFY 2
#define READ_VAR 4
#define SET_VAR 5

#define SUCCESS 0x40
#define ERROR 0x80
#define WRONG_NUMBER 0xC0


int ReadConfig(unsigned char Line, USHORT Add)
{
  char FileName[512] ;

  sprintf (FileName,"NodeConf\\Config_%d_%d.eep",(int)Line,(int)Add) ;
  
  FileSize=LoadIHexFile(FileName,0) ;
  if ((FileSize<=0)||(FileSize>512)) {
    /* File not loades */
    printf ("Konfigurationsdatei fuer Linie %d, Addresse %d nicht gefunden/falsche Groesse.\n",Line,Add) ;
    return -1 ;
  } ;
  return (FileSize) ;
}

int WriteConfig(unsigned char Line, USHORT Add)
{
  FILE *OutFile ;
  int i ;
  IHexRecord IR ;
  char FileName[512] ;

  sprintf (FileName,"NodeConf\\Config_%d_%d.eep",(int)Line,(int)Add) ;
  
  OutFile = fopen(FileName,"w") ;

  if (OutFile==NULL) {
    printf ("Konnte Konfiguration nicht sichern\n") ;
    return (-1) ;
  } ;
  for (i=0;i<16;i++) { /* 16 Records schreiben */
    New_IHexRecord(IHEX_TYPE_00,i*32,&(FileBuffer[i*32]),32,&IR) ;
    Write_IHexRecord(&IR,OutFile); 
  } ;
  New_IHexRecord (IHEX_TYPE_01,0,FileBuffer,0,&IR) ;
  Write_IHexRecord(&IR,OutFile); 
  fclose (OutFile) ;
  return (1) ;
}



int SendEEProm (unsigned char Line, USHORT Add)
{
  ULONG SendID ;
  ULONG RecID ;
  char SendLen ;
  char SendData[8] ;
  char RecLen ;
  char RecData[8] ;
  USHORT i,j ;
  
  SendID = BuildCANId(0,0,0,1,Line,Add,0) ;
  
  for (i = 0 ;i<FileSize;i++) {
    if (i%32==0) {
      printf (".") ;
      fflush(stdout);
    } ;
    if (i<10) continue ; /* die ersten 10 Byte sind Bootloader-Konfiguration, nicht anfassen */
    SendLen = 4 ;
    SendData[0] = SET_VAR ;
    SendData[1] = (unsigned char)(i&0xFF) ;
    SendData[2] = (unsigned char)(i>>8) ;
    SendData[3] = FileBuffer[i] ;
    SendCANMessage (SendID,SendLen,SendData) ;
    // Delay to ensure that it has been received and stored
    for (j=0;j<10;j++) {
      ReceiveCANMessage (&RecID,&RecLen,RecData) ;
      if ((MatchAddress(RecID,0,1))&&(MatchSrcAddress(RecID,Line,Add))&&(RecData[0]==SET_VAR|SUCCESS)) break ;
    } ;
    if (j>=10) { 
      printf ("Did not receive answer from %d %d\n",Line,Add) ;
      return (-1);
    } ;
  } ;
  return (0) ;
}

int ReadEEProm(unsigned char Line, USHORT Add)
{
  ULONG SendID ;
  ULONG RecID ;
  char SendLen ;
  char SendData[8] ;
  char RecLen ;
  char RecData[8] ;
  int i,j ;
  
  SendID = BuildCANId(0,0,0,1,Line,Add,0) ;
  
  for (i=0;i<512;i++) {
    /* Request senden */
    if (i%32==0) {
      printf (".") ;
      fflush(stdout);
    } ;
    SendLen = 4 ;
    SendData[0] = READ_VAR ;
    SendData[1] = (unsigned char)(i&0xFF) ;
    SendData[2] = (unsigned char)(i>>8) ;
    SendCANMessage (SendID,SendLen,SendData) ;
    /* Daten empfangen */
    for (j=0;j<10;j++) {
      ReceiveCANMessage (&RecID,&RecLen,RecData) ;
      if ((MatchAddress(RecID,0,1))&&(MatchSrcAddress(RecID,Line,Add))&&(RecData[0]==READ_VAR|SUCCESS)) break ;
    } ;
    if (j>=10) { 
      printf ("Did not receive answer from %d %d\n",Line,Add) ;
      return (-1);
    } ;
    FileBuffer[i] = RecData[3] ;
  }
  FileSize = 512 ;
  return (0) ;
}

char NodeLine ;
USHORT NodeAdd ;

int PrintMainMenu (void)
{
  int i ;
  
  for (i=-1;(i<0)||(i>8);) {
    printf ("\n\nKnotenlinie: %d; Adresse: %d, Konfiguration: %s\n\n",NodeLine,NodeAdd,FileSize==512?"geladen":"nicht geladen") ;
    printf ("1. Knoten spezifizieren \n") ;
    printf ("2. Knoten vom Bus lesen \n") ;
    printf ("3. Vorhandene Konfiguration lesen \n") ;
    printf ("4. Knoten konfigurieren \n") ;
    printf ("5. Konfiguration sichern\n") ;
    printf ("6. Knoten konfigurieren \n") ;
    printf ("7. Konfiguration anzeigen \n") ;
    printf ("8. Beenden\n\n") ;
    scanf ("%d",&i) ;
  } ;
  return (i) ;
}

void ReadNodeAdd(void)
{
  int i ;
  printf ("\nCAN-Knoten Konfiguration\n") ;
  for (i=-1;(i<0)||(i>15);) {
    printf ("Bitte Knoten-Linie eingeben (0-15): ") ;
    scanf ("%d",&i) ;
  } ;
  NodeLine = i ;
  for (i=-1;(i<0)||(i>255);) {
    printf ("Bitte Knoten-Addresse eingeben (0-255): ") ;
    scanf ("%d",&i) ;
  } ;
  NodeAdd = i ;
}

struct EEPROM *EE ;

void ConfigRelais (void) 
{
  int i ;
  int Port ;
  
  while (1) {
    printf ("\nRelais-Konfiguration\n\n") ;
    printf ("Rolladen-Laufzeiten:\n") ;

    for (i=0;i<5;i++) {
      printf ("Port %d: Lang: %d s, Kurz: %d*10ms, Vertauschen: %d\n",i+1,
	      EE->Data.Relais.RTFull[i],EE->Data.Relais.RTShort[i],EE->Data.Relais.UpDown[i]) ;
    } ;
    
    for (i=-1;(i<1)||(i>6);) {
      printf ("\nRolladen-Konfiguration von Port (1-5, 6:Ende): ") ;
      scanf("%d",&i) ;
    }
    
    if (i==6) return ;
    
    Port = i-1 ;
    
    for (i=-1;(i<0)||(i>255);) {
      printf ("Port %d: Lange Laufzeit (aktuell: %d s): ",Port+1,EE->Data.Relais.RTFull[Port]) ;
      scanf("%d",&i) ;
    }
    EE->Data.Relais.RTFull[Port] = i ;
    
    for (i=-1;(i<0)||(i>255);) {
      printf ("Port %d: Kurze Laufzeit (aktuell: %d *10 ms): ",Port+1,EE->Data.Relais.RTShort[Port]) ;
      scanf("%d",&i) ;
    }
    EE->Data.Relais.RTShort[Port] = i ;

    for (i=-1;(i<0)||(i>1);) {
      printf ("Port %d: Hoch/runter vertauschen (0: Nein, 1: Ja, aktuell: %d): ",Port+1,EE->Data.Relais.UpDown[Port]) ;
      scanf("%d",&i) ;
    }
    EE->Data.Relais.UpDown[Port] = i ;
  } ;
}

void PrintRelais(void)
{
  int i ;
  printf ("Relais-Konfiguration\n\n") ;
  printf ("Rolladen-Laufzeiten:\n") ;
  for (i=0;i<5;i++) {
    printf ("Port %d: Lang: %d s, Kurz: %d*10ms, Vertauschen: %d\n",i+1,
	    EE->Data.Relais.RTFull[i],EE->Data.Relais.RTShort[i],EE->Data.Relais.UpDown[i]) ;
  } ;
}

void InputCommand(struct EEPromSensFunc *SF)
{
  int i,j ;

  for (i=-1;(i<0)||(i>15);) {
    printf ("Bitte Ziel-Knoten-Linie eingeben (0-15, aktuell: %d): ",SF->TargetLine) ;
    scanf ("%d",&i) ;
  } ;
  SF->TargetLine = i ;
  for (i=-1;(i<0)||(i>255);) {
    printf ("Bitte Ziel-Knoten-Addresse eingeben (0-255, aktuel:%d): ",SF->TargetAdd[0]) ;
    scanf ("%d",&i) ;
  } ;
  SF->TargetAdd[0] = i ;
  SF->TargetAdd[1] = 0 ; 
  for (i=-1;(i<0)||(i>40);) {
    printf ("\nVerfuegbare Kommandos:\n") ;
    printf ("LED_OFF: 10, LED_On: 11, SET_TO: 12, HSET_TO: 13, START_PROG: 22, STOP_PROG: 23\n") ;
    printf ("CHANNEL_ON: 30, CHANNEL_OFF: 31, CHANNEL_TOGGLE: 32\n") ;
    printf ("SHADE_UP_FULL: 33, SHADE_DN_FULL: 34, SHADE_UP_SHORT: 35, SHADE_DN_SHORT: 36\n") ;
    printf ("SET_PIN: 40\n\n") ;
    printf ("Bitte Kommando eingeben (aktuell %d): ",SF->Command) ;
    scanf ("%d",&i) ;
  } ;
  SF->Command = i ;
  if ((i==30)||(i==31)||(i==32)) {
    /* Relais-Kommando, also noch den Port lesen */
    for (i=-1;(i<1)||(i>10);) {
      printf ("Bitte Ziel-Port eingeben (1-10, aktuel:%d): ",SF->Data[0]) ;
      scanf ("%d",&i) ;
    } ;
    SF->Data[0] = i ;
    SF->Data[1] = SF->Data[2] = SF->Data[3] = SF->Data[4] = SF->Data[5] = 0 ;
  } else if ((i==33)||(i==34)||(i==35)||(i==36)) {
    /* Relais-Kommando, also noch den Port lesen */
    for (i=-1;(i<1)||(i>5);) {
      printf ("Bitte Ziel-Port eingeben (1-5, aktuel:%d): ",SF->Data[0]) ;
      scanf ("%d",&i) ;
    } ;
    SF->Data[0] = i ;
    SF->Data[1] = SF->Data[2] = SF->Data[3] = SF->Data[4] = SF->Data[5] = 0 ;
  } else {
    for (j=0;j<6;j++) {
      printf ("Data[%d] (aktuell %d) = ",j,SF->Data[i]) ;
      scanf("%d",&i) ;
      SF->Data[i] = i ;
    } ;
  } ;
}

void PrintCommand(struct EEPromSensFunc *SF)
{
  int i ;
  if ((SF->TargetLine==0xff)&&(SF->TargetAdd[0]==0xFF)) return ;
  printf ("Ziel Knoten: %d, Linie %d \n",SF->TargetLine,SF->TargetAdd[0]) ;
  i = SF->Command ;
  
  if ((i==30)||(i==31)||(i==32)) {
    /* Relais-Kommando, also noch den Port lesen */
    if (i==30) printf ("Switch on Port %d\n",SF->Data[0]) ;
    if (i==31) printf ("Switch off Port %d\n",SF->Data[0]) ;
    if (i==32) printf ("Toggle Port %d\n",SF->Data[0]) ;
  } else {
    for (i=0;i<6;i++) {
      printf ("Data[%d] = %d",i,SF->Data[i]) ;
    } ;
    printf ("\n") ;
  } ;  
} 



void InputRolladen(struct EEPromSensFunc *SF)
{
  int i,j ;
  unsigned char *Data2 ;
  unsigned char Command ;

  for (i=-1;(i<1)||(i>2);) {
    printf ("Rolladen abwaerts (1) oder aufwaerts (2): ") ;
    scanf ("%d",&i) ;
  } ;
  if (i==1) {
    Command=34 ;
  } else {
    Command=33 ;
  } ;

  for (i=-1;(i<0)||(i>15);) {
    printf ("Bitte Ziel-Knoten-Linie eingeben (0-15, aktuell: %d): ",SF->TargetLine) ;
    scanf ("%d",&i) ;
  } ;
  SF->TargetLine = i ;
  SF->TargetAdd[0] = 0 ;
  SF->TargetAdd[1] = 0 ;
  SF->Command = Command ;
  Data2 = SF->Data ;
  for (j=0;j<6;j++) {
    for (i=-1;(i<0)||(i>255);) {
      printf ("Laden %d; Bitte Ziel-Knoten-Addresse eingeben (0-255, aktuell: %d): ",j,Data2[j]) ;
      scanf ("%d",&i) ;
    } ;
    Data2[j*2]=i; 
    for (i=-1;(i<1)||(i>5);) {
      printf ("Laden %d; Bitte Ziel-Port eingeben (1-5, aktuell: %d): ",j,Data2[j+1]) ;
      scanf ("%d",&i) ;
    } ;
    Data2[j*2+1]=i; 
  } ;
}

void PrintRolladen (struct EEPromSensFunc *SF)
{
  int j ;
  
  if (SF->Command==34) {
    printf ("Rollo abwaerts \n") ;
  } else {
    printf ("Rollo aufwaerts \n") ;
  } ;
  
  for (j=0;j<6;j++) {
    if ((SF->Data[j*2+1]>0)&&(SF->Data[j*2+1]<6)) 
      printf("Linie %d, Knoten %d, Port %d\n",SF->TargetLine,SF->Data[j*2],SF->Data[j*2+1]) ;
  } ;
} 

char *SensType[] = {
  "unbekannt",
  "einfacher Eingang",
  "kurz oder lang Eingang",
  "Digitaleingang mit Timer (Monoflop)",
  "Digitaleingang mit Timer (retriggerbar)",
  "Analog-Eingang",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "Digital-Ausgang",
  "PWM-Ausgang",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "unbekannt",
  "WS2801 Clock",
  "WS2801 Data" } ;

void ConfigSensor(void)
{
  int i ;
  int Port ;
  int Rolladen ;
  
  while (1) {
    printf ("\nSensor-Konfiguration\n\n") ;
    
    for (i=0;i<6;i++) {
      if (EE->Data.Sensor.Config[i].Config>40) EE->Data.Sensor.Config[i].Config = 0 ;
      printf ("Port %d konfiguriert als %s\n",i+1,SensType[EE->Data.Sensor.Config[i].Config]) ;
    } ;

    for (i=-1;(i<1)||(i>7);) {
      printf ("\nSensor-Konfiguration von Port (1-6, 7:Ende): ") ;
      scanf("%d",&i) ;
    }
    
    if (i==7) return ;
    
    Port = i-1 ;

    for (i=-1;(i<0)||(i>22);) {
      printf ("\nEingabe-Kennungen:\n") ;
      printf ("1: Einfacher Eingang, 2: Kurz-Lang-Eingang\n");
      printf ("3: Monoflop, 4: Retriggerbares Monoflop, 5: Analogeingang, 6: Rolladen\n") ;
      printf ("Ausgabe-Kennungen:\n") ;
      printf ("10: Einfacher Ausgabeport, 11: PWM-Port\n") ;
      printf ("WS2803-Port:\n") ;
      printf ("20: WS2801 Clock, 21: WS2801 Data\n\n") ;
      printf ("Port %d (aktuell: %s): ",Port+1,SensType[EE->Data.Sensor.Config[Port].Config]) ;
      scanf ("%d",&i) ;
    } ;
    
    if (i==6) {
      Rolladen = TRUE ;
      EE->Data.Sensor.Config[Port].Config = 2 ;
    } else {
      Rolladen = FALSE ;
      EE->Data.Sensor.Config[Port].Config = i ;
    } ;

    if (i<10) {
      /* Sensorfunktionen */
      if ((EE->Data.Sensor.Config[Port].Config>2)&&(EE->Data.Sensor.Config[Port].Config<6)) {
	/* Monoflop oder Analog -> Zeitwerte setzen */
	for (i=-1;(i<0)||(i>255);) {
	  printf ("Port %d Timer (aktuell: %d s): ",Port+1,SensType[EE->Data.Sensor.Config[Port].Data]) ;
	  scanf ("%d",&i) ;
	} ;      
	EE->Data.Sensor.Config[Port].Data=i ;
      } else {
	EE->Data.Sensor.Config[Port].Data=0 ;
      } ;
      if ((EE->Data.Sensor.Config[Port].Config==2)) {	  
	/* Short/long-Setzen */
	for (i=-1;(i<0)||(i>255);) {
	  printf ("Port %d Repeat_start (aktuell: %d*1/10 s): ",Port+1,EE->Data.Sensor.REPEAT_START) ;
	  scanf ("%d",&i) ;
	} ;      
	EE->Data.Sensor.REPEAT_START=i ;
	for (i=-1;(i<0)||(i>255);) {
	  printf ("Port %d Repeat_end (aktuell: %d*1/10 s): ",Port+1,EE->Data.Sensor.REPEAT_END) ;
	  scanf ("%d",&i) ;
	} ;      
	EE->Data.Sensor.REPEAT_END=i ;
      } ;
      if (Rolladen) {
	printf ("\n\nKonfiguration Rolladen mit Home-Control\n\n") ;
	InputRolladen(&EE->Data.Sensor.Pin[Port].ShortMaster) ;
	printf ("\n\nKonfiguration Rolladen ohne Home-Control\n\n") ;
	InputRolladen(&EE->Data.Sensor.Pin[Port].ShortAuto) ;
      } else {
	/* Short-Addressen abfragen */
	printf ("\n\nKonfiguration kurzer Klick mit Home-Control\n\n") ;
	InputCommand(&EE->Data.Sensor.Pin[Port].ShortMaster) ;
	printf ("\n\nKonfiguration kurzer Klick ohne Home-Control\n\n") ;
	InputCommand(&EE->Data.Sensor.Pin[Port].ShortAuto) ;
	if ((EE->Data.Sensor.Config[Port].Config!=1)&&(EE->Data.Sensor.Config[Port].Config!=5)){
	  /* auch die Long-Addressen abfragen */
	  printf ("\n\nKonfiguration langer Klick mit Home-Control\n\n") ;
	  InputCommand(&EE->Data.Sensor.Pin[Port].LongMaster) ;
	  printf ("\n\nKonfiguration langer Klick ohne Home-Control\n\n") ;
	  InputCommand(&EE->Data.Sensor.Pin[Port].LongAuto) ;
	}
      } ;
    } else if (i<20) {
      /* Ausgangfunktionen */
      for (i=-1;(i<0)||(i>1);) {
	printf ("Port %d Wert nach einschalten (aktuell: %d): ",Port+1,SensType[EE->Data.Sensor.Config[Port].Data]) ;
	scanf ("%d",&i) ;
      } ;      
      EE->Data.Sensor.Config[Port].Data=i ;
    } else {
      /* WS2801-Funktionen */
    } ;
  } ;
}
  
void PrintSensor(void)
{
  int i ;
  int Rolladen ;

  EE = (struct EEPROM*)FileBuffer ;
  
  for (i=0;i<6;i++) {
    if (EE->Data.Sensor.Config[i].Config>40) EE->Data.Sensor.Config[i].Config = 0 ;
    Rolladen = 0 ;
    if ((EE->Data.Sensor.Config[i].Config==2)&&
	((EE->Data.Sensor.Pin[i].ShortMaster.Command==33)||(EE->Data.Sensor.Pin[i].ShortMaster.Command==34))) Rolladen = 1 ;
    if (Rolladen) {
      printf ("Port %d konfiguriert als Rolladen\n",i+1) ;
    } else {
      printf ("Port %d konfiguriert als %s\n",i+1,SensType[EE->Data.Sensor.Config[i].Config]) ;
    } ;
    if ((EE->Data.Sensor.Config[i].Config>2)&&(EE->Data.Sensor.Config[i].Config<6)) {
      /* Monoflop oder Analog -> Zeitwerte setzen */
      printf ("Port %d Timer = %d s\n): ",i+1,SensType[EE->Data.Sensor.Config[i].Data]) ;
    } ;
    if ((EE->Data.Sensor.Config[i].Config==2)) {
      printf ("Port %d Repeat_start: %d*1/10 s\n",i+1,EE->Data.Sensor.REPEAT_START) ;
      printf ("Port %d Repeat_end: %d*1/10 s\n ",i+1,EE->Data.Sensor.REPEAT_END) ;
    } ;
    if (Rolladen) {
      printf ("Konfiguration mit Server:\n") ;
      PrintRolladen(&EE->Data.Sensor.Pin[i].ShortMaster) ;
      printf ("Konfiguration ohne Server:\n") ;
      PrintRolladen(&EE->Data.Sensor.Pin[i].ShortAuto) ;
    } else {
      if (EE->Data.Sensor.Config[i].Config==0) continue ;
      printf ("Konfiguration mit Server:\n") ;
      PrintCommand(&EE->Data.Sensor.Pin[i].ShortMaster) ;
      printf ("Konfiguration ohne Server:\n") ;
      PrintCommand(&EE->Data.Sensor.Pin[i].ShortAuto) ;
      if ((EE->Data.Sensor.Config[i].Config!=1)&&(EE->Data.Sensor.Config[i].Config!=5)){
	printf ("Konfiguration lang gedrueckt mit Server:\n") ;
	PrintCommand(&EE->Data.Sensor.Pin[i].LongMaster) ;
	printf ("Konfiguration lang gedrueckt ohne Server:\n") ;
	PrintCommand(&EE->Data.Sensor.Pin[i].LongAuto) ;
      } ;
    }; 
  } ;
}


void ConfigNode(void)
{
  int i ;
  int Typ; 

  EE = (struct EEPROM*)FileBuffer ;

  /* Wenn noch nicht konfiguriert, Basiskonfiguration */
  if (FileSize!=512) {
    for (i=-1;(i<0)||(i>3);) {
      printf ("\nBitte Typ eingeben\n") ;
      printf ("1. LED\n") ;
      printf ("2. Relais\n") ;
      printf ("3. Sensor\n\n") ;
      scanf ("%d",&i) ;
    } ;
    EE->BoardType = (i-1)*16 ;
    EE->BoardAdd[0] = NodeAdd ;
    EE->BoardAdd[1] = 0x00 ;
    EE->BootAdd[0] = 0x01 ;
    EE->BootAdd[1] = 0x00 ;
    EE->BootLine = 0x00 ;
    EE->BoardLine = NodeLine ;
    EE->PAD = 0xFF ;
    EE->Magic[0] = 0xBA ;
    EE->Magic[1] = 0XCA ;
    for (i=0;i<502;i++) EE->Data.PAD[i] = 0xFF ;
    FileSize = 512 ;
  }

  if (EE->BoardType==16) {
    ConfigRelais () ;
  } else if (EE->BoardType==32) {
    ConfigSensor () ;
  } ;
}


int main (void)
{
  int i ;
  ULONG TestID ;
  unsigned char Len ;
  unsigned char Data[8] ;
  InitNetwork () ;
  
  NodeLine = 16 ;
  FileSize = 0 ;
  
  while (1) {
    i = PrintMainMenu () ;
    switch (i) {
    case 1:
      ReadNodeAdd () ;
      break ;
    case 2:
      if (NodeLine>15) {
	ReadNodeAdd();
      }
      ReadEEProm (NodeLine,NodeAdd) ;
      break ;
    case 3:
      if (NodeLine>15) {
	ReadNodeAdd();
      }
      ReadConfig (NodeLine,NodeAdd) ;
      break ;
    case 4:
      ConfigNode () ;
      break ;
    case 5:
      if (FileSize!=512) {
	printf ("\nKnoten noch nicht konfiguriert\n") ;
	break ;
      } ;
      if (NodeLine>15) {
	ReadNodeAdd();
      }
      WriteConfig (NodeLine,NodeAdd) ;
      break ;
    case 6:
      if (FileSize!=512) {
	printf ("\nKnoten noch nicht konfiguriert\n") ;
	break ;
      } ;
      if (NodeLine>15) {
	ReadNodeAdd();
      }
      SendEEProm (NodeLine,NodeAdd) ;
      break ;
    case 7:
      if (FileSize!=512) {
	printf ("\nKnoten noch nicht konfiguriert\n") ;
	break ;
      } ;
      if (NodeLine>15) {
	ReadNodeAdd();
      }
      EE = (struct EEPROM*)FileBuffer ;
      if (EE->BoardType==16) {
	PrintRelais() ;
      } else {
	PrintSensor() ;
      } ;
      break ;
    case 8:
      CloseNetwork () ;
      exit(0) ;
    } ;
  } ;
}

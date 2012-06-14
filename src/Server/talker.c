/*
** talker.c -- a datagram "client" demo
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
#include <netdb.h>
#include <ctype.h>

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
#define	BADPARAM		-2			// Parametri alla funzione errati

#define	FILENOTFOUND	-3			// File non aperto in lettura
#define	CREATEERROR		-4			// File non aperto in scrittura
#define	BADFILETYPE		-5			// File di tipo errato
#define	READERROR		-6			// Errore in lettura dal file
#define	WRITEERROR		-7			// Errore in scrittura da file
#define	NOTHINGTOSAVE	-8			// Nessun contenuto da salvare
#define NOTSUPPORTED	-9			// Funzionalita` non supportata (ancora)
#define	BUFFEROVERFLOW		-30
#define	OUTOFMEMORY			-31
#define BUFFERUNDERFLOW		-32
#define TRUE (0==0)
#define FALSE (0==1)

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
  if (((CANId & (0xf<<10))==(ToLine<<10))&&(((CANId & (0xfff<<2))==(ToAdd<<2)))) {
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

  /*  printf("listener: got packet from %s\n",
	 inet_ntop(their_addr.ss_family,
		   get_in_addr((struct sockaddr *)&their_addr),
		   s, sizeof s));
		   printf("listener: packet is %d bytes long\n", numbytes);
  */

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
#define SET_ADDRESS 3
#define DATA 4
#define START_APP 5

#define SUCCESS 0x40
#define ERROR 0x80
#define WRONG_NUMBER 0xC0

typedef struct 
{
  char NodeType ;
  char *FileName ;
} BootFileList ;

BootFileList FileList[] = {
  {1,"Bad.hex"},
  {16,"Relais.hex"},
  {18,"Licht.hex"},
  {32,"Sensor.hex"},
  {0,NULL}
} ;

void UpdateServer (ULONG TestID,unsigned char Len,unsigned char *Data)
{
  static char State=0 ;
  static unsigned char Package=0 ;
  static char UpdateLine=0 ;
  static USHORT UpdateAdd=0 ;
  static char Consume = 0 ;
  char a;
  USHORT b ;
  static ULONG SendID ;
  static char SendLen ;
  static char SendData[8] ;
  static int FileSize ;
  static int ActualPosition ;
  static int PageSize ;

  if ((Len==6)&&(Data[0]==UPDATE_REQUEST)) {
    // Update-Beginn
    State = 1 ;
    Package = 0 ;
    GetSourceAddress(TestID,&UpdateLine,&UpdateAdd) ;
    SendID = BuildCANId(0,0,0,1,UpdateLine,UpdateAdd,0) ;
    SendLen = 2 ;
    SendData[0] = IDENTIFY ;
    SendData[1] = Package++ ;
    if (Consume==0) {
      SendCANMessage(SendID,SendLen,SendData) ;
      Consume = 1 ;
    } ;
    printf ("Received request from: %d %d\n", UpdateLine, UpdateAdd) ;
    return ;
  } ;

  if (State==0) {
    printf ("Mismatched Communications\n") ;
  } ;

  GetSourceAddress(TestID,&a,&b) ;
  
  if ((a!=UpdateLine)||(b!=UpdateAdd)) {
    printf ("Another board want's to have update at the same time!") ;
    return ;
  } ;

  /* Wrong Number-Antworten behandeln */
  if (Data[0]==WRONG_NUMBER) {
    // Falsche Paket-Seqzent bzw. Paket verloren gegangen-> einfachster Lösung: Noch einmal von vorne 
    State = 1 ;
    Package = Data[1] ;
    SendLen = 2 ;
    SendData[0] = IDENTIFY ;
    SendData[1] = Package++ ;
    Consume = 0 ;
    SendCANMessage (SendID,SendLen,SendData) ;
    printf ("Wrong Number\n") ;
    return ;
  } ;
  
  // Auf Fehler pruefen und ggf. Kommunikation neu starten
  if ((Data[0]&0xc0)!=SUCCESS) {
    State = 1 ;
    Package = 0 ;
    SendLen = 2 ;
    SendData[0] = IDENTIFY ;
    SendData[1] = Package++ ;
    Consume = 0 ;
    printf ("No Success\n") ;
    SendCANMessage (SendID,SendLen,SendData) ;
    return ;
  } ;
  

  if (State==1) {
    // Identifikation, dann Flash-File laden und Addresse senden.
    for (a=0;FileList[a].NodeType!=0;a++) {
      if (FileList[a].NodeType==Data[2]) break ;
    } ;
    if (FileList[a].NodeType==0) {
      /* Wrong Node Type */
      State = 0 ;
      printf ("Unbekannter Knoten-Typ %d wurde gesendet.\n",Data[2]) ;
      return ;
    } ;
    
    FileSize=LoadIHexFile(FileList[a].FileName,0) ;
    if (FileSize<=0) {
      /* File not loades */
      State = 0 ;
      printf ("Update-Datei für Knoten-Typ %d wurde nicht gefunden.\n",Data[2]) ;
      return ;
    } ;
    /* Vorbereiten des File-Puffers auf die Seitengroesse */
    PageSize = 32<<Data[3] ;
    ActualPosition = FileSize ;

    if (FileSize%PageSize!=0) FileSize = ((FileSize/PageSize)+1)*PageSize ;

    for (;ActualPosition<FileSize;ActualPosition++) FileBuffer[ActualPosition] = 0xff ;

    ActualPosition = 0 ;
    printf ("Start Update\n") ;
    State = 2 ;
    SendLen = 6 ;
    SendData[0] = SET_ADDRESS ;
    SendData[1] = Package++ ;
    SendData[2] = 0 ;
    SendData[3] = 0 ;
    SendData[4] = 0 ;
    SendData[5] = 0 ;
    SendCANMessage (SendID,SendLen,SendData) ;
    return ;
  } ;

  if (State==2) {
    // Addresse angenommen-> Daten senden
    SendLen = 6 ;
    SendData[0] = DATA ;
    SendData[1] = Package++ ;
    SendData[2] = FileBuffer[ActualPosition] ;
    SendData[3] = FileBuffer[ActualPosition+1] ;
    SendData[4] = FileBuffer[ActualPosition+2] ;
    SendData[5] = FileBuffer[ActualPosition+3] ;
    SendCANMessage (SendID,SendLen,SendData) ;
    ActualPosition+=4 ;
    if (ActualPosition%32==0) { 
      printf(".") ;
      fflush (stdout) ;
    } ;
    if (ActualPosition>=FileSize) State=3 ;
    return ;
  } ;

  if (State==3) {
    // Daten geflasht-> Applikation starten
    printf  ("\nApplikation Starten\n") ;
    SendLen = 2 ;
    SendData[0] = START_APP ;
    SendData[1] = Package++ ;
    SendCANMessage (SendID,SendLen,SendData) ;
    State=4 ;
    return ;
  } ;

  if (State=4) {
    // Applikation wurde gestartet -> Alles gut
    State=0 ;
    Package = 0 ;
    UpdateLine = 0 ;
    UpdateAdd = 0 ;
    Consume = 0 ;
    return ;
  } ;

}

int main (void)
{
  int i ;
  ULONG TestID ;
  unsigned char Len ;
  unsigned char Data[8] ;

  printf ("Init\n") ;

  InitNetwork () ;
 
  for (;;) {
    ReceiveCANMessage (&TestID,&Len,Data) ;
    
    if (MatchAddress(TestID,0,1)) {
      /* Kommunikation mit dem Software-Updater */
      UpdateServer (TestID,Len,Data) ;
    } ;
  } ;

  CloseNetwork () ;
}

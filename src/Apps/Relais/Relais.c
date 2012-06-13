/*
  Relais-Ansteuerung über CAN; Applikationsprogramm für den Bootloader.

  AT90PWM3b @ 16 MHz
 
*/
 
 
// includes
 
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "../Common/mcp2515.h"
#include "../Common/utils.h"

#define CHAN0 	C,4
#define CHAN1 	C,3
#define CHAN2 	C,2
#define CHAN3 	C,1
#define CHAN4 	C,0
#define CHAN5 	B,4
#define CHAN6 	B,3
#define CHAN7 	B,2
#define CHAN8 	B,1
#define CHAN9 	B,0

/* EEProm-Belegung vom Boot-Loader:
0   0xba
1   0xca
2   BoardAdd Low Byte
3   BoardAdd High Byte
4   BoardLine
5   BootAdd Low Byte
6   BootAdd High Byte
7   BootLine
8   BoardType (0: LED, 0x10: Relais, 0x20: Sensor)  
9   n/a

EEProm-Belegung vom Relais:
10..15  RunTime Full
20..25  RunTime Short
30..35  Config Up-Down
*/
// globale Variablen

can_t Message ;
uint8_t LastCommand ;
uint8_t Channel[10] ; // Zustand des Relais-Kanals
uint16_t Timer[5] ; // Timer im 1/100 Sekunden Takt (bis max. 600 Sekunden) für Rollo, 
                    // automatisch abwärtszählend bis 0 ;
uint8_t State[5] ;  // Zustand der Aktionsmaschine "Rollo"
uint8_t UpDown[5] ; // Befehl für Rollo
uint8_t Position[5] ; // Aktuelle Rollo-Position

// BuildCANId baut aus verschiedenen Elementen (Line & Addresse von Quelle und Ziel 
// sowie Repeat-Flag und Gruppen-Flag) den CAN Identifier auf

inline uint32_t BuildCANId (uint8_t Prio, uint8_t Repeat, uint8_t FromLine, uint16_t FromAdd, uint8_t ToLine, uint16_t ToAdd, uint8_t Group)
{
  return (((uint32_t)(Group&0x1))<<1|((uint32_t)ToAdd)<<2|((uint32_t)(ToLine&0xf))<<10|
	  ((uint32_t)FromAdd)<<14|((uint32_t)(FromLine&0xf))<<22|
	  ((uint32_t)(Repeat&0x1))<<26|((uint32_t)(Prio&0x3))<<27) ;
}

// GetSourceAddress bestimmt aus dem CAN-Identifier der eingehenden Nachricht 
// die Line & Addresse

inline void GetSourceAddress (uint32_t CANId, uint8_t *FromLine, uint16_t *FromAdd)
{
  *FromLine = (uint8_t)((CANId>>22)&0xf) ;
  *FromAdd = (uint16_t) ((CANId>>14)&0xfff) ;
}

// GetTargetAddress liefert die Addresse aus dem CAN-Identifier

inline uint8_t GetTargetAddress (uint32_t CANId)
{
  return((uint8_t)((CANId>>2)&0xff));
}

// Alle Filter des 2515 auf die eigene Board-Addresse setzen

void SetFilter(uint8_t BoardLine,uint16_t BoardAdd)
{
  can_filter_t filter ;
  filter.id = ((uint32_t)BoardAdd)<<2|((uint32_t)BoardLine)<<10 ;
  filter.mask = 0x3FFC ;
  mcp2515_set_filter(0, &filter) ;
  mcp2515_set_filter(1, &filter) ;
  mcp2515_set_filter(2, &filter) ;
  mcp2515_set_filter(3, &filter) ;
  mcp2515_set_filter(4, &filter) ;
  filter.id = ((uint32_t)0xff)<<2|((uint32_t)BoardLine)<<10 ;
  mcp2515_set_filter(5, &filter) ;
}

// Message für das zurücksenden vorbereiten (Quelle als Ziel eintragen und 
// Boardaddresse als Quelle)

void SetOutMessage (uint8_t BoardLine, uint16_t BoardAdd)
{
  uint8_t SendLine ;
  uint16_t SendAdd ;
  
  GetSourceAddress(Message.id,&SendLine,&SendAdd) ;
  Message.id = BuildCANId (0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;
  Message.data[0] = Message.data[0]|SUCCESSFULL_RESPONSE ;
  Message.length = 1 ;
}

// Kanal Chan einschalten (Translation auf Port-Pin)

void ChannelOn(uint8_t Chan)
{
  switch (Chan) {
  case 0:
    SET (CHAN0) ;
    break ;
  case 1:
    SET (CHAN1) ;
    break ;
  case 2:
    SET (CHAN2) ;
    break ;
  case 3:
    SET (CHAN3) ;
    break ;
  case 4:
    SET (CHAN4) ;
    break ;
  case 5:
    SET (CHAN5) ;
    break ;
  case 6:
    SET (CHAN6) ;
    break ;
  case 7:
    SET (CHAN7) ;
    break ;
  case 8:
    SET (CHAN8) ;
    break ;
  case 9:
    SET (CHAN9) ;
    break ;
  default:
    return ;
  } ;
}

// Kanal Chan ausschalten (Translation auf Port-Pin)

void ChannelOff(uint8_t Chan)
{
  switch (Chan) {
  case 0:
    RESET (CHAN0) ;
    break ;
  case 1:
    RESET (CHAN1) ;
    break ;
  case 2:
    RESET (CHAN2) ;
    break ;
  case 3:
    RESET (CHAN3) ;
    break ;
  case 4:
    RESET (CHAN4) ;
    break ;
  case 5:
    RESET (CHAN5) ;
    break ;
  case 6:
    RESET (CHAN6) ;
    break ;
  case 7:
    RESET (CHAN7) ;
    break ;
  case 8:
    RESET (CHAN8) ;
    break ;
  case 9:
    RESET (CHAN9) ;
    break ;
  default:
    return ;
  } ;
}


// Interrup-Service-Routine zur PWM-Rücknahme des Haltestroms; die ersten 64 Durchlaeufe
// bleibt das Relais eingeschaltet, danach wird mit 1:1 abgeschwaecht.
// Routine wird mit 2kHz bei OCR1A=1000 aufgerufen 
// (bzw. durch Vorspannen mit TCNT1=500 mit 4 kHz)

ISR(TIMER1_COMPA_vect) 
{
  static uint8_t RelaisPhase=0;
  static uint8_t CounterTimer=0 ;
  uint8_t i ;
 
  // 1:1-Taktung
  if (RelaisPhase==0) {
    TCNT1 = 500;
    RelaisPhase = 1 ;
  } else {
    TCNT1 = 500 ;
    RelaisPhase = 0 ;
  } ;

  // Timer alle 10 ms herabzählen
  if ((CounterTimer++)>=40) {
    CounterTimer = 0 ;
    for (i=0;i<5;i++) if (Timer[i]>0) Timer[i]-- ;
  }
  
  // Alle Kanäle durchgehen

  for (i=0;i<10;i++) {
    if (Channel[i]==0) {
      // Wenn Aus, dann Aus
      ChannelOff(i) ;
    } else if (Channel[i]<64) {
      // Wenn in der Anzugsphase, dann An
      ChannelOn(i) ;
      Channel[i]++ ;
    } else {
      // Wenn in der Strom-Absenkphase, dann je nach dem An oder Aus
      if (RelaisPhase==1) {
	ChannelOn(i) ;
      } else {
	ChannelOff(i) ;
      } ;
    } ;
  } ;
}

void BroadcastStatus(  uint8_t BoardLine, uint16_t BoardAdd )
{
	int i ;
	// An den Systembus schicken (Addresse: 0/1) 
	Message.id = BuildCANId (0,0,BoardLine,BoardAdd,0,1,0) ;
	Message.data[0] = SEND_STATUS|SUCCESSFULL_RESPONSE ;
    Message.data[1] = (Channel[4]>>2)+(Channel[3]>>3)+(Channel[2]>>4)+(Channel[1]>>5)+(Channel[0]>>6) ;
    Message.data[2] = (Channel[9]>>2)+(Channel[8]>>3)+(Channel[7]>>4)+(Channel[6]>>5)+(Channel[5]>>6) ;
	for (i=0;i<5;i++) Message.data[i+3]=Position[i] ;
    Message.length = 8 ;
    mcp2515_send_message(&Message) ;				
} 


// Hauptprogramm
 
int main(void) 
{
  uint8_t BoardLine ;
  uint16_t BoardAdd ;
  uint16_t Addr ;
  uint8_t r ;
  uint8_t i ;
  uint8_t Direction ;
  
  // Default-Werte:
  BoardAdd = 16 ;
  BoardLine = 1 ;

  // Noch einmal setzen (sollte nach Boot-Loader an sich schon passiert sein)
  SET_OUTPUT(CHAN0);
  SET_OUTPUT(CHAN1);
  SET_OUTPUT(CHAN2);
  SET_OUTPUT(CHAN3);
  SET_OUTPUT(CHAN4);
  SET_OUTPUT(CHAN5);
  SET_OUTPUT(CHAN6);
  SET_OUTPUT(CHAN7);
  SET_OUTPUT(CHAN8);
  SET_OUTPUT(CHAN9);

  // Lesen der EEProm-Konfiguration
  
  r = eeprom_read_byte((uint8_t*)0) ;
  if (r==0xba) {
    r = eeprom_read_byte((uint8_t*)1) ;
    if (r==0xca) {
      BoardAdd = eeprom_read_byte((uint8_t*)2) ;
      BoardAdd += ((uint16_t)eeprom_read_byte((uint8_t*)3))<<8 ;
      BoardLine = eeprom_read_byte((uint8_t*)4) ;
    } ;
  } ;
	

  // Initialisieren des CAN-Controllers
  mcp2515_init();
  
  /* Filter muss hier gesetzt werden */	
  SetFilter(BoardLine,BoardAdd) ;
  
  // Timer 1 OCRA1, als variablem Timer nutzen
  TCCR1B = 2;             // Timer läuft mit Prescaler/8
  TCNT1 = 0;              // Timer auf Null stellen
  OCR1A = 1000;           // Overflow auf 1000
  TIMSK1 |= (1 << OCIE1A);   // Interrupt freischalten
  
  sei();                  // Interrupts gloabl einschalten

  // Endlosschleife zur Abarbeitung der Kommandos

  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=mcp2515_get_message(&Message)) == NO_MESSAGE) {
      // Rollo-Zustandsmaschine
      for (i=0;i<5;i++) {
	if (UpDown[i]>0) { //Rollo soll verfahren werden
	  switch (State[i]) {
	  case 0:
	    // Motor aus
	    if (Channel[i]>0) {
	      Channel[i] = 0 ;
	      Timer[i] = 20 ;
	      State[i] = 1 ;
	    } else {
	      State[i] = 2 ;
	    } ;
	    break ;
	  case 1:
	    // Motor zum Stillstand kommen lassen
	    if (Timer[i]==0) State[i] = 2 ;
	    break ;
	  case 2:
            // Bewegungsrichtung einstellen
	    if ((UpDown[i]==1)||(UpDown[i]==3)) {
	      Channel[i+5] = 0 ;
	    } else {
	      Channel[i+5] = 1 ;
	    } ;
	    Timer [i] = 5 ;
	    State [i] = 3 ;
	    break ;
	  case 3:
	    // Relais-Umschalten abwarten //
	    if (Timer[i]==0) State[i]=4 ;
	    break ;
	  case 4:
            // Motor einschalten 
	    Channel[i] = 1 ;
	    if (UpDown[i]>2) { // Short Time Read (in 1/100 Seconds)
	      Timer[i] =((uint16_t)eeprom_read_byte((uint8_t*)(i+20))) ;
	    } else {
	      // Long Time Read (in Seconds)
	      Timer[i] =((uint16_t)eeprom_read_byte((uint8_t*)(i+10)))*100 ;
	    }
	    State[i] = 5 ;
	    break ;
	  case 5:
	    // Rollo laufen lassen
	    if (Timer[i]==0) State[i]=6 ;
	    break ;
	  case 6:
	    // Motor ausmachen
	    Channel[i] = 0 ;
	    Timer [i] = 20 ;
	    State [i] = 7 ;
	    break ;
	  case 7:
	    // Nachlauf abwarten
	    if (Timer[i]==0) State[i] = 8 ;
	    break ;
	  case 8:
	  default:
	    // Alles aus
		if (eeprom_read_byte((uint8_t*)(uint16_t)i+30)==0) {
			Position[i] = ((UpDown[i]==1)||(UpDown[i]==3))?0:100;
		} else {
			Position[i] = ((UpDown[i]==1)||(UpDown[i]==3))?100:0;
		} ;
	    Channel[i] = 0 ;
	    Channel[i+5] = 0 ;
	    UpDown[i] = 0 ;
	    State[i] = 0 ;
		BroadcastStatus(BoardLine,BoardAdd) ;
	  } ;
	} ;
      } ;
    };
    
    // Kommando extrahieren
    r = Message.data[0] ;

    // Sende-Addresse zusammenstoepseln (enthält auch die Quelladdresse aus Message,
    // ueberschreibt dann die In-Message)
    SetOutMessage(BoardLine,BoardAdd) ;

    // Befehl abarbeiten
    switch (r) {

    case SEND_STATUS:
      Message.data[1] = (Channel[4]>>2)+(Channel[3]>>3)+(Channel[2]>>4)+(Channel[1]>>5)+(Channel[0]>>6) ;
      Message.data[2] = (Channel[9]>>2)+(Channel[8]>>3)+(Channel[7]>>4)+(Channel[6]>>5)+(Channel[5]>>6) ;
	  for (i=0;i<5;i++) Message.data[i+3]=Position[i] ;
      Message.length = 8 ;
      mcp2515_send_message(&Message) ;				
      break ;

    case READ_CONFIG:
      Message.data[1] = eeprom_read_byte((uint8_t*)0) ;
      Message.data[2] = eeprom_read_byte((uint8_t*)1) ;
      Message.data[3] = eeprom_read_byte((uint8_t*)2) ;
      Message.data[4] = eeprom_read_byte((uint8_t*)3) ;
      Message.data[5] = eeprom_read_byte((uint8_t*)4) ;
      Message.length = 6 ;
      mcp2515_send_message(&Message) ;
      break ;

    case WRITE_CONFIG:
      if ((Message.data[1] == 0xba)&&(Message.data[2]==0xca)) {	
	eeprom_write_byte((uint8_t*)2,Message.data[3]) ;	
	eeprom_write_byte((uint8_t*)3,Message.data[4]) ;	
	eeprom_write_byte((uint8_t*)4,Message.data[5]) ;	
      } ;
      break ;

    case READ_VAR:
      Addr = ((uint16_t)Message.data[1])+(((uint16_t)Message.data[2])<<8) ;
      Message.data[3]=eeprom_read_byte((uint8_t*)Addr) ;
      Message.length = 4 ;
      mcp2515_send_message(&Message) ;
      break ;

    case SET_VAR:
      Addr = ((uint16_t)Message.data[1])+(((uint16_t)Message.data[2])<<8) ;
      eeprom_write_byte((uint8_t*)Addr,Message.data[3]) ;
	  Message.length=4 ;
      mcp2515_send_message(&Message) ; // Empfang bestaetigen
      break ;

    case START_BOOT:
      wdt_enable(WDTO_250MS) ;
      while(1) ;
      break ;

      // Diese Befehle sind beim Relais nicht bekannt
    case TIME:
      /* LED */
    case LED_OFF:
    case LED_ON:
    case SET_TO:
    case HSET_TO:
    case L_AND_S:
    case SET_TO_G1:
    case SET_TO_G2:
    case SET_TO_G3:
    case LOAD_LOW:
    case LOAD_MID1:
    case LOAD_MID2:
    case LOAD_HIGH:
    case START_PROG:
    case STOP_PROG:
      /* Sensor */
    case SET_PIN:
    case LOAD_LED:
    case OUT_LED:
      break ;
      // Relais-Befehle
    case CHANNEL_ON:
      if (Message.data[1]<11) Channel[Message.data[1]-1] = 1 ;
	  BroadcastStatus(BoardLine,BoardAdd) ;
      break ;
    case CHANNEL_OFF:
      if (Message.data[1]<11) Channel[Message.data[1]-1] = 0 ;
	  BroadcastStatus(BoardLine,BoardAdd) ;
      break ;
    case CHANNEL_TOGGLE:
      if (Message.data[1]<11) Channel[Message.data[1]-1] = (Channel[Message.data[1]-1]==0?1:0) ;
	  BroadcastStatus(BoardLine,BoardAdd) ;
      break ;
    case SHADE_UP_FULL:
      i = Message.data[1]-1 ;
      if (i<6) {
		if (eeprom_read_byte((uint8_t*)(uint16_t)i+30)==0) {
			Direction = 1 ;
		} else {
			Direction = 2 ;
		} ; 
		if ((State[i]>0)&&(UpDown[i]==Direction)) { // Bewegt sich gerade, also schauen ob er anhalten soll, oder in die andere Richtung fahren... 
			State[i] = 6 ;
		} else {
			State[i] = 0 ;
			UpDown[i] = Direction ;
		} ;
      }
      break ;
    case SHADE_DOWN_FULL:
      i = Message.data[1]-1 ;
      if (i<6) {
		if (eeprom_read_byte((uint8_t*)(uint16_t)i+30)==0) {
			Direction = 2 ;
		} else {
			Direction = 1 ;
		} ; 
		if ((State[i]>0)&&(UpDown[i]==Direction)) { // Bewegt sich gerade, also schauen ob er anhalten soll, oder in die andere Richtung fahren... 
			State[i] = 6 ;
		} else {
			State[i] = 0 ;
			UpDown[i] = Direction ;
		} ;
      }
      break ;
    case SHADE_UP_SHORT:
      i = Message.data[1]-1 ;
      if (i<6) {
	if (State[i]>0) { // Bewegt sich gerade, also anhalten 
	  State[i] = 6 ;
	} else {
	  State[i] = 0 ;
	  if (eeprom_read_byte((uint8_t*)(uint16_t)i+30)==0) {
	    UpDown[i] = 3 ;
	  } else {
	    UpDown[i] = 4 ;
	  } ;
	} ;
      }
      break ;
    case SHADE_DOWN_SHORT:
      i = Message.data[1]-1 ;
      if (i<6) {
	if (State[i]>0) { // Bewegt sich gerade, also anhalten 
	  State[i] = 6 ;
	} else {
	  State[i] = 0 ;
	  if (eeprom_read_byte((uint8_t*)(uint16_t)i+30)==0) {
	    UpDown[i] = 4 ;
	  } else {
	    UpDown[i] = 3 ;
	  } ;
	} ;
      } ;
      break ;
    } ;
  } ;
}

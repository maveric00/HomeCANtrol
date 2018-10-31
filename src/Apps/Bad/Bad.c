/*
  Bad-Ansteuerung über CAN; Applikationsprogramm für den Bootloader.

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

EEProm-Belegung vom LED-Board:
10..29: Programm Channel 1
30..49: Programm Channel 2,...
450..469: Programm Channel 22
470:

   470 : Pin 1 Short tic with timer running
   470 : Board Address Low 
   471 : Board Address High
   472 : Line address
   473 : Command
   474 : Data 1    or Add1  if Command = 33
   475 : Data 2    or Port1 if Command = 33
   476 : Data 3    or Add2  if Command = 33
   477 : Data 4    or Port2 if Command = 33
   478 : Data 5    or Add3  if Command = 33
   479 : Data 6    or Port3 if Command = 33


   480 : Pin 1 Long Tic with timer running
   or: 480 Add4  if Command=33
       481 Port4 if Command=33
       482 Add5  if Command=33
       483 Port5 if Command=33
       484 Add6  if Command=33
       485 Port6 if Command=33
       486 Add7  if Command=33
       487 Port7 if Command=33
       488 Add8  if Command=33
       489 Port8 if Command=33
		 
   490 : Pin 1 Short Tic without timer running
   500 : Pin 1 Long Tic without timer running

   510: DelayTimer (in Sekunden)
   511: SoftLightOn (in 0.1 Sekunden) ;

*/
// globale Variablen
#define PWM_CHANNELS 24
#define TIMER_PRESET 99


extern volatile uint8_t LEDVal[PWM_CHANNELS] ;
extern volatile uint8_t LEDPWM[56] ;
extern volatile uint8_t MasterVal ;
volatile uint16_t LEDV2[PWM_CHANNELS] ;
volatile int16_t Delta[PWM_CHANNELS] ;
volatile uint8_t Counter[PWM_CHANNELS] ;
volatile uint8_t Step[PWM_CHANNELS] ;

uint8_t Inhibit[6] ;

can_t Message ;
uint8_t  BoardLine ;
uint16_t BoardAdd ;

uint8_t  LastCommand ;
volatile uint8_t  Heartbeat ;
volatile uint16_t Timers ;
uint8_t  TimerStatus ;



// BuildCANId baut aus verschiedenen Elementen (Line & Addresse von Quelle und Ziel 
// sowie Repeat-Flag und Gruppen-Flag) den CAN Identifier auf

uint32_t BuildCANId (uint8_t Prio, uint8_t Repeat, uint8_t FromLine, uint16_t FromAdd, uint8_t ToLine, uint16_t ToAdd, uint8_t Group)
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

uint8_t GetProgram(uint8_t Channel, uint8_t PStep)
{
  uint8_t *Da ;
  
  if (Channel>22) return(0) ;
  if (PStep>=20) return (0) ;
  Da = (uint8_t*) 10 ;
  Da += Channel*20;
  Da += PStep ;
  return (eeprom_read_byte(Da)) ;
}



inline void StepLight (void)
{
  uint8_t Channel ;
  uint8_t Command ;
  
  // Alle Kanaele abarbeiten; wird 25 mal pro sekunde aufgerufen 
  for (Channel=0;Channel<PWM_CHANNELS;Channel++) {
    if (Counter[Channel]>0) {
      LEDV2[Channel] += Delta[Channel] ;
      Counter[Channel]-- ;
    } else {
      if (Step[Channel]>=20) continue ;
      Command = GetProgram (Channel,Step[Channel]) ;
      Step[Channel]++ ;
      if (Command==0) { // Programm Ende  
	Step[Channel] = 22 ;
      } else if	(Command<201) { // DimTo 
	Counter[Channel] = Command ;                                 //40 180 222 240 222 240 222 240 10 255 222 10 10 180 203
	Command = GetProgram(Channel,Step[Channel]) ;                //28B4DEF0DEF0DEF0DFDFDF0AFFDE0A0AB4CB0000
	Step[Channel]++ ;
	Delta[Channel] = (int16_t)(((((int32_t)Command)<<8)-(int32_t)LEDV2[Channel])/(int16_t)Counter[Channel]) ;
	if (Delta[Channel]>0) Delta[Channel]++ ; // Rundungsfehler ausgleichen		
	LEDV2[Channel] += Delta[Channel] ; // und ersten Schritt ausfuehren
	Counter[Channel]-- ;
      } else if (Command<221) { // JumpTo 
	Step[Channel] = Command-201 ;
      } else if (Command==221) { // SetTo 
	LEDV2[Channel] = ((uint16_t)GetProgram(Channel,Step[Channel]))<<8 ;
	Step[Channel]++ ;
      } else if (Command==222) { // Delay 
	Delta[Channel] = 0 ;
	Counter[Channel] = GetProgram(Channel,Step[Channel]) ;
	Step[Channel]++ ;
      } else if (Command==223) { // Random Delay 
	Delta[Channel] = 0 ;
	Counter[Channel] = (uint8_t)(TCNT1&0xff) ;
	Step[Channel]++ ;
      } else if (Command==224) {
	// nop
      } else { // Unknown command 
	Step[Channel] = 20 ; // Auf Ende Setzen 
      } ;
    } ;
    // Uebertragen auf die LED-Ausgabe
    LEDVal[Channel] = (uint8_t)(LEDV2[Channel]>>8) ;
  } ;
}

void StoreProgram(void)
{
  uint8_t *Da ;
  uint8_t Offset ;
  Offset = Message.data[2] ;
  if (Offset>15) return ; // Illegal Offset
  
  Da = (uint8_t*)10 ;
  Da += Offset ;
  Da += Message.data[1]*20 ;
  
  eeprom_write_byte(Da++,Message.data[3]) ;
  eeprom_write_byte(Da++,Message.data[4]) ;
  eeprom_write_byte(Da++,Message.data[5]) ;
  eeprom_write_byte(Da++,Message.data[6]) ;
  eeprom_write_byte(Da++,Message.data[7]) ;
} 

void SendPinMessage (uint8_t Long, uint8_t Timer)
{
  uint8_t *Data ;
  uint16_t SendAdd ;
  uint8_t SendLine ;
  uint8_t Command ;
  uint8_t i ;
  
  Data = (uint8_t*)470 ;
  if (Timer == 1) {
    if (Inhibit[Long]==1) return ;
    Data +=20 ;
  } ;
  
  Command = eeprom_read_byte(Data+3) ;
  SendLine = eeprom_read_byte(Data+2) ;

  if ((Command==SHADE_UP_FULL)||(Command==SHADE_DOWN_FULL)) {
    for (i=0;i<8;i++) {
      SendAdd = eeprom_read_byte(Data+4+i*2) ;
      if (SendAdd==0) break ; // Keine weiteren Empfaenger

      Message.id = BuildCANId(0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;

      if (Command==SHADE_UP_FULL) {	
	Message.data[0] = (Long==1)?SHADE_UP_FULL:SHADE_UP_SHORT ;
      } else {
	Message.data[0] = (Long==1)?SHADE_DOWN_FULL:SHADE_DOWN_SHORT ;
      }

      Message.data[1] = eeprom_read_byte(Data+4+i*2+1) ;
      Message.length = 2 ;
	  _delay_ms(5);
      mcp2515_send_message(&Message) ;
    } ;
  } else {
    if (Long == 1) Data += 10 ;
	Command = eeprom_read_byte(Data+3) ;
	SendLine = eeprom_read_byte(Data+2) ;
    
    SendAdd = eeprom_read_byte(Data) ;
    SendAdd += ((uint16_t)eeprom_read_byte(Data+1))<<8 ;
    
    if (SendAdd==0) return ;
    
    Message.id = BuildCANId (0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;
    Message.data[0] = Command ;
    Message.data[1] = eeprom_read_byte(Data+4) ;
    Message.data[2] = eeprom_read_byte(Data+5) ;
    Message.data[3] = eeprom_read_byte(Data+6) ;
    Message.data[4] = eeprom_read_byte(Data+7) ;
    Message.data[5] = eeprom_read_byte(Data+8) ;
    Message.data[6] = eeprom_read_byte(Data+9) ;
    Message.length = 7 ;
    mcp2515_send_message(&Message) ;
  } ;
}

ISR( TIMER0_OVF_vect )
{ 
  static int CC=0 ; 
  TCNT0 = (uint8_t)TIMER_PRESET;  // preload for 10ms  
  if (Timers>0) Timers-- ;  
  CC++ ;  

  if (CC>=4) {   
    TIMSK0 &= ~(1<<TOIE0) ;  // Disable Timer0 interrupt
    sei();     // Re-Enable Interrupts
    StepLight();  
    cli() ;   // Disable timer 0 interrupts
    TIMSK0 |= 1<<TOIE0;            // enable timer interrupt
    CC = 0 ;  
  }  

  if (Heartbeat<250) Heartbeat++ ;
}

// Hauptprogramm

#define LED1 B,5
#define LED2 B,6
#define INPORT C,1
#define LEDPORT C,2

extern void InitBCM(void);

int __attribute__((OS_main)) main(void) 
{
  uint16_t Addr ;
  uint8_t r ;
  uint8_t Delay ;
  
  // Default-Werte:
  BoardAdd = 16 ;
  BoardLine = 1 ;
  
  for (r=0;r<PWM_CHANNELS;r++) {
    Step[r] = 22 ;
    Counter[r] = 0 ;
    LEDVal[r] = 0 ;
    LEDV2[r] = 0 ;
  } ;
  
  
  
  SET_OUTPUT(LEDPORT);
  SET_INPUT_WITH_PULLUP(INPORT);
  

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
  
  Delay = eeprom_read_byte((uint8_t*)511) ;
  
  
  
  // Initialisieren des CAN-Controllers
  mcp2515_init();
  
  // Filter muss hier gesetzt werden 	
  SetFilter(BoardLine,BoardAdd) ;
  
  // Timer 1 OCRA1, als variablem Timer nutzen
  MasterVal = 255 ;
  
  // Timer 0 als 10 ms-Timer verwenden
  TCCR0B = (1<<CS02)|(1<<CS00);  // divide by 1024
  TCNT0 = (uint8_t)TIMER_PRESET; // preload for 100 Hz
  TIMSK0 |= 1<<TOIE0;            // enable timer interrupt
  
  InitBCM () ;
  
  SET_INPUT_WITH_PULLUP(INPORT);

  wdt_enable (WDTO_2S) ;
  
  sei();                  // Interrupts gloabl einschalten
  
  // Endlosschleife zur Abarbeitung der Kommandos
  
  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=mcp2515_get_message(&Message)) == NO_MESSAGE) {
      // Hier wird der Bewegungsmelder abgefragt

      wdt_reset();
      
      if (!IS_SET(INPORT)) {
	if (Timers==0) {
	  SendPinMessage(0,(Heartbeat>200)?1:0) ;
	  //Programm starten
	  if (Heartbeat>200) {
	    for (r=0;r<PWM_CHANNELS;r++) {
	      Step[r] = 0 ;
	      Counter[r] = 0 ;
	    } ;
	  } ;
	} ;
	SET(LEDPORT);
	Timers= ((uint16_t)eeprom_read_byte((uint8_t*)510))*1000 ;
	TimerStatus = 1 ;
      } else {
	RESET (LEDPORT);
      } ;
      if (Timers==0) {
	if (TimerStatus>0) {
	  SendPinMessage(1,(Heartbeat>200)?1:0) ;
	  TimerStatus = 0 ;
	  // Programm stoppen
	  if (Heartbeat>200) {
	    for (r=0;r<PWM_CHANNELS;r++) {
	      Step[r] = 22 ;
	      Counter[r] = 0 ;
	      LEDVal[r] = 0 ;
	      LEDV2[r] = 0 ;
	    } ;
	  } ;
	} ;
      } ;	 
    };

    wdt_reset();
    
    // Kommando extrahieren
    r = Message.data[0] ;
    
    // Sende-Addresse zusammenstoepseln (enthält auch die Quelladdresse aus Message,
    // ueberschreibt dann die In-Message)
    SetOutMessage(BoardLine,BoardAdd) ;
    
    // Befehl abarbeiten
    switch (r) {
      
    case SEND_STATUS:
      Message.data[1] = 0 ;
      Message.data[2] = 0 ;
      Message.length = 3 ;
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
      Heartbeat = 0 ;
      break ;
      /* LED */
    case CHANNEL_ON:
    case CHANNEL_OFF:
      break ;
    case CHANNEL_TOGGLE:
      break ;
    case SET_TO:
    case HSET_TO:
    case L_AND_S:
    case SET_TO_G1:
    case SET_TO_G2:
    case SET_TO_G3:
      break ;
    case LOAD_PROG:
      if (Message.data[1]>PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  Message.data[1] = r ;
	  StoreProgram () ;
	} ;
      } else {
	Message.data[1]-- ;
	StoreProgram() ;
      } ;
      break ;
    case START_PROG:
      if (Message.data[1]>PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  Step[r] = Message.data[2] ;
	  Counter[r] = 0 ;
	} ;
      } else {
	r = Message.data[1]-1 ;
	Step[r] = Message.data[2] ;
	Counter[r] = 0 ; 
      } ;
      break ;
    case STOP_PROG:
      if (Message.data[1]>PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  Step[r] = 22 ;
	  Counter[r] = 0 ;
	  LEDVal[r] = 0 ;
	  LEDV2[r] = 0 ;
	} ;
      } else {
	r = Message.data[1]-1 ;
	Step[r] = 22 ;
	Counter[r] = 0 ; 
	LEDVal[r] = 0 ; 
	LEDV2[r] = 0 ;
      } ;
      break ;
      /* Sensor */
    case SET_PIN:
    case DIM_TO:
      break ;
    case LOAD_LED:
    case OUT_LED:
      break ;
    case STOP_SENSOR:
      if ((Message.data[1]<7)&&(Message.data[1]>0)) Inhibit[Message.data[1]-1] = 1;
      break ;
    case START_SENSOR:
      if ((Message.data[1]<7)&&(Message.data[1]>0)) Inhibit[Message.data[1]-1] = 0;
      break ;
      // Relais-Befehle
    case SHADE_UP_FULL:
    case SHADE_DOWN_FULL:
    case SHADE_UP_SHORT:
    case SHADE_DOWN_SHORT:
      break ;
    } ;
  } ;
}

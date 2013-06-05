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

// globale Variablen

can_t Message ;

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


void delay_ms(uint16_t __ticks)
{
	while(__ticks)
	{
		// wait 1/10 ms
		_delay_loop_2((F_CPU) / 4e3);
		__ticks --;
	}
}

// Hauptprogramm
 
int main(void) 
{
  uint8_t BoardLine ;
  uint16_t BoardAdd ;
  uint16_t Addr ;
  uint8_t r ;
  uint8_t i,j ;
  uint8_t LastCommand ;

  // Default-Werte:
  BoardAdd = 0xFF ;
  BoardLine = 0xF ;

  // Lesen der EEProm-Konfiguration
  
  r = eeprom_read_byte((uint8_t*)0) ;
  if (r==0xba) {
    r = eeprom_read_byte((uint8_t*)1) ;
    if (r==0xca) {
      // Config has been written - restart Bootloader to enable upload of main application
      wdt_enable(WDTO_250MS) ;
      while(1) ;
    } ;
  } ;

  eeprom_write_byte((uint8_t*)0,0xba) ; // erstes byte schreiben

  // Initialisieren des CAN-Controllers
  mcp2515_init();
  
  /* Filter muss hier gesetzt werden */	
  SetFilter(BoardLine,BoardAdd) ;
  
  sei();                  // Interrupts gloabl einschalten

  // Endlosschleife zur Abarbeitung der Kommandos
  
  Message.id = BuildCANId (0,0,BoardLine,BoardAdd,0,1,0) ;
  Message.data[0] = SET_VAR|SUCCESSFULL_RESPONSE ;
  Message.data[1] = 0 ;
  Message.data[2] = 0 ;
  Message.data[3] = 0xba ;
  Message.length = 4 ;
  mcp2515_send_message(&Message) ;

  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=mcp2515_get_message(&Message)) == NO_MESSAGE) {};
    
    // Kommando extrahieren
    r = Message.data[0] ;

    // Sende-Addresse zusammenstoepseln (enthält auch die Quelladdresse aus Message,
    // ueberschreibt dann die In-Message)
    SetOutMessage(BoardLine,BoardAdd) ;

    // Befehl abarbeiten
    switch (r) {

    case SEND_STATUS:
      for (i=0;i<10;i++) ChanStat[i]=(Channel[i]>0)?1:0 ;
      Message.data[1] = (ChanStat[0])+(ChanStat[1]<<1)+(ChanStat[2]<<2)+(ChanStat[3]<<3)+(ChanStat[4]<<4) ;
      Message.data[2] = (ChanStat[5])+(ChanStat[6]<<1)+(ChanStat[7]<<2)+(ChanStat[8]<<3)+(ChanStat[9]<<4) ;
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
    case SET_TO:
    case HSET_TO:
    case L_AND_S:
    case SET_TO_G1:
    case SET_TO_G2:
    case SET_TO_G3:
    case START_PROG:
    case STOP_PROG:
      /* Sensor */
    case SET_PIN:
    case LOAD_LED:
    case OUT_LED:
      break ;
      // Relais-Befehle
    case CHANNEL_ON:
    case CHANNEL_OFF:
    case CHANNEL_TOGGLE:
    case SHADE_UP_FULL:
    case SHADE_DOWN_FULL:
    case SHADE_UP_SHORT:
    case SHADE_DOWN_SHORT:
    case SEND_LEDPORT:
      break ;
    default:
      break ;
    } ;
  } ;
}

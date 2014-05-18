/*
  Bootstrap-Programm zum setzen des EEProm-Inhalts (danach wird die Applikation geflasht)

  STM32F103RC 
*/
 
 
// includes
 
#include <stdint.h>
#include <string.h>
#include "stm32f1xx.h"
#include "main.h"
#include "CANLib.h"
#include "EEProm.h"

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
*/


// Hauptprogramm
 
int main(void) 
{
  CanTxMes Message ;
  CanRxMsg InMessage ;
  uint8_t BoardLine ;
  uint16_t BoardAdd ;
  uint16_t Addr ;
  uint8_t r ;
  uint8_t LastCommand ;

  // Default-Werte:
  BoardAdd = 0xFF ;
  BoardLine = 0xF ;

  // Lesen der EEProm-Konfiguration

  EEPromInit () ;
  
  if (EEProm!=NULL) {
    // Config has been written - restart Bootloader to enable upload of main application
    NVIC_SystemReset ();
  } ;
  
  EEPromWriteByte(0xba,0) ; // erstes byte schreiben

  CAN_Config();

  SetFilter(BoardLine,BoardAdd) ;

  Init_TxMessage (&Message) ;

  // Endlosschleife zur Abarbeitung der Kommandos
  
  Message.ExtID = BuildCANId (0,0,BoardLine,BoardAdd,0,1,0) ;
  Message.Data[0] = SET_VAR|SUCCESSFULL_RESPONSE ;
  Message.Data[1] = 0 ;
  Message.Data[2] = 0 ;
  Message.Data[3] = 0xba ;
  Message.DLC = 4 ;
  CAN_send_message(&Message) ;

  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=CAN_get_message(&InMessage)) == NO_MESSAGE) {};
    
    // Kommando extrahieren
    r = InMessage.Data[0] ;

    // Sende-Addresse zusammenstoepseln (enthält auch die Quelladdresse aus Message,
    // ueberschreibt dann die In-Message)
    SetOutMessage(InMessage,Message,BoardLine,BoardAdd) ;

    // Befehl abarbeiten
    switch (r) {

    case SEND_STATUS:
      Message.Data[1] = 0 ;
      Message.DLC = 2 ;
      CAN_send_message(&Message) ;
      break ;

    case READ_CONFIG:
      Message.Data[1] = EEProm[0] ;
      Message.Data[2] = EEProm[1] ;
      Message.Data[3] = EEProm[2] ;
      Message.Data[4] = EEProm[3] ;
      Message.Data[5] = EEProm[4] ;
      Message.DLC = 6 ;
      CAN_send_message(&Message) ;
      break ;

    case WRITE_CONFIG:
      if ((InMessage.Data[1] == 0xba)&&(InMessage.Data[2]==0xca)) {	
	EEPromWriteByte(InMessage.Data[3],2) ;	
	EEPromWriteByte(InMessage.Data[4],3) ;	
	EEPromWriteByte(InMessage.Data[5],4) ;	
      } ;
      break ;

    case READ_VAR:
      Addr = ((uint16_t)InMessage.Data[1])+(((uint16_t)InMessage.Data[2])<<8) ;
      Message.Data[3]=EEProm[Addr] ;
      Message.DLC = 4 ;
      CAN_send_message(&Message) ;
      break ;

    case SET_VAR:
      Addr = ((uint16_t)InMessage.Data[1])+(((uint16_t)InMessage.Data[2])<<8) ;
      EEPromWriteByte(InMessage.Data[3],Addr) ;	
      Message.DLC=4 ;
      CAN_send_message(&Message) ;
      break ;

    case START_BOOT:
      EEPromFlush () ;
      NVIC_SystemReset ();
      break ;

    default:
      break ;
    } ;
  } ;
}

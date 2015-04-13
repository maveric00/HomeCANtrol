/*
  Steuerprogramm für die STM32F103 Basisplatine

  STM32F103RC 
*/
 
 
// includes
 
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "CANLib.h"
#include "EEProm.h"
#include "ws2812.h"

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

int  Type[6] ;
int  Config[6];

void InitMC (void)
{
  int i ;

  for (i=0;i<6;i++) {
    // Configuration lesen

    Type[i] = EEProm[300+(i<<1)] ;
    Config[i] = EEProm[301+(i<<1)] ;

    switch (Type[i]) {
    case I_SIMPLE: // Klick-Input
    case I_SHORTLONG: // Short-Long-Input
    case I_MONO: // Monoflop
    case I_RETRIG: // Retriggerbares Monoflop
      break ;
    case I_ANALOG: // Analog-Input
      break ;
    case O_ONOFF: // Ein-Aus
    case O_PWM: // PWM
      break ;
    case O_WSCLOCK: // WS2801 Clock
    case O_WSDATA: // WS2801 Data
      WSinit () ;
      break ;
    } ;
  } ;
}


// Hauptprogramm
 
int main(void) 
{
  CanTxMsg Message ;
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
  
  if (EEProm==NULL) {
    // Config has not yet been written - restart Bootloader to enable upload of main application
    NVIC_SystemReset ();
  } ;

  BoardAdd = EEProm[2]+(EEProm[3]<<8) ;
  BoardLine = EEProm[4] ;
  BootAdd = EEProm[5]+(EEProm[6]<<8) ;
  BootLine = EEProm[7] ;
  BoardType = EEProm[8] ;
  
  // Initialize CAN

  CAN_Config();

  // Initialize WS2812 output

  InitMC () ;

  SetFilter(BoardLine,BoardAdd) ;

  Init_TxMessage (&Message) ;

  // Endlosschleife zur Abarbeitung der Kommandos
  
  Message.ExtId = BuildCANId (0,0,BoardLine,BoardAdd,0,1,0) ;
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
    SetOutMessage(&InMessage,&Message,BoardLine,BoardAdd) ;

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

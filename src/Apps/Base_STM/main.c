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

#define TIMEOUT 1000 // 10 Sekunden Timeout

int  Type[6] ;
int  Config[6];
volatile uint16_t  Heartbeat ;
volatile uint8_t Time ;
uint8_t  Running[6] ;

CanTxMsg Message ;


void SendPinMessage (uint8_t Pin, uint8_t Long,uint8_t SendData)
{
  uint8_t *Data ;
  uint16_t SendAdd ;
  uint8_t SendLine ;
  uint8_t Command ;
  uint8_t i ;
  
  Data = (uint8_t*)10 ;
  Data += Pin*40 ;
  if (Heartbeat>TIMEOUT) Data +=20 ;

  if (!Running[(int)Pin]) return ; // Dieser Pin ist ausgeschaltet worden

  Command = EEProm[Data+3] ;
  SendLine = EEProm[Data+2] ;

  if ((Command==(uint8_t)SHADE_UP_FULL)||(Command==(uint8_t)SHADE_DOWN_FULL)) {
    for (i=0;i<8;i++) {
      SendAdd = EEProm[Data+4+(i<<1)] ;
      if (!SendAdd) break ; // Keine weiteren Empfaenger

      Message.ExtId = BuildCANId(0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;

      if (Command==(uint8_t)SHADE_UP_FULL) {	
	Message.Data[0] = (Long==(uint8_t)1)?SHADE_UP_FULL:SHADE_UP_SHORT ;
      } else {
	Message.Data[0] = (Long==(uint8_t)1)?SHADE_DOWN_FULL:SHADE_DOWN_SHORT ;
      }

      Message.Data[1] = eeprom_read_byte(Data+5+(i<<1)) ;
      Message.DLC = 2 ;
      CAN_send_message(&Message) ;
    } ;
  } else {
    if (Long ==(uint8_t) 1) Data += 10 ;
	Command = EEProm[Data+3] ;
	SendLine = EEProm[Data+2] ;
    
    SendAdd = EEProm[Data] ;
    SendAdd += ((uint16_t)EEProm[Data+1])<<8 ;
    
    if (!(SendAdd||SendLine)) return ;
    
    Message.ExtId = BuildCANId (0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;
    Message.Data[0] = Command ;
    Message.Data[1] = (SendData==(uint8_t)0)?EEProm[Data+4]:SendData ;
    Message.Data[2] = EEProm[Data+5] ;
    Message.Data[3] = EEProm[Data+6] ;
    Message.Data[4] = EEProm[Data+7] ;
    Message.Data[5] = EEProm[Data+8] ;
    Message.Data[6] = EEProm[Data+9] ;
    Message.Data[7] = Heartbeat>90?Time+1:Time ;
    Message.DLC = 8 ;
    CAN_send_message(&Message) ;
  } ;
}



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
      break ;
    case O_PWM: // PWM
      PowerInit () ;
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

  SetFilter(BoardLine,BoardAdd) ;

  for (r=0;r<6;r++) Running[r] = 1 ; // Alle Eingaenge sind aktiv

  // Initialize Pin-Configuration

  InitMC () ;

  Init_TxMessage (&Message) ;


  // Endlosschleife zur Abarbeitung der Kommandos
  
  Message.ExtId = BuildCANId (0,0,BoardLine,BoardAdd,0,1,0) ;
  Message.Data[0] = IDENTIFY ;
  Message.DLC = 1 ;
  CAN_send_message(&Message) ;

  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=CAN_get_message(&InMessage)) == NO_MESSAGE) {};
    
    // Kommando extrahieren
    r = InMessage.Data[0] ;
    j = InMessage.Data[1] ;

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

    case TIME:
      Heartbeat = 0 ;
      Time = j ;
      break ;

    case CHANNEL_ON:
    case CHANNEL_OFF:
    case CHANNEL_TOGGLE:
      j-- ;
      if (j>(uint8_t)5) j=0 ;
      for (;(j<InMessage.Data[1])&&(j<6);j++) { // Wenn Port = 1..6, dann nur diesen, sonst alle
	if ((Type[j]!=(uint8_t)O_ONOFF)&&(Type[j]!=(uint8_t)O_PWM)) continue ; // Illegaler PIN
	if (r==(uint8_t)CHANNEL_ON) {
	  i = 255 ;
	} else if (r==(uint8_t)CHANNEL_OFF) {
	  i = 0 ;
	} else {
	  i =255-PWM[j] ;
	}
	START_PWM[j] = PWM[j] ;
	SOLL_PWM[j] = i ;
	if (Config[j]) {
	  TimerPWM[j] = DurationPWM[j] = Config[j] ;
	} else {
	  TimerPWM[j] = DurationPWM[j] = 1 ;
	} ;
      } ;
      break ;


    default:
      break ;
    } ;
  } ;
}

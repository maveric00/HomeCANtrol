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

#define I_SIMPLE    1
#define I_SHORTLONG 2
#define I_MONO      3
#define I_RETRIG    4
#define I_ANALOG    5
#define I_BWM       6
#define I_BWM2      7
#define I_LIGHT     8

#define O_ONOFF    10
#define O_PWM      11
#define O_WSCLOCK  20
#define O_WSDATA   21 
#define TIMEOUT 1000 // 10 Sekunden Timeout

int  Type[6] ;
int  Config[6];
volatile int system_time ;
volatile uint16_t  Heartbeat ;
volatile uint8_t Time ;
uint8_t  Running[6] ;
uint8_t BoardLine ;
uint16_t BoardAdd ;


volatile uint16_t  REPEAT_MASK ;
volatile uint16_t REPEAT_START ;
volatile uint16_t REPEAT_NEXT ;
volatile uint16_t  key_state;                               // debounced and inverted key state:
                                                           // bit = 1: key pressed
volatile uint16_t  key_press;                               // key press detect
volatile uint16_t  key_rpt;                                 // key long press and repeat

CanTxMsg Message ;


void SendPinMessage (uint8_t Pin, uint8_t Long,uint8_t SendData)
{
  int Data ;
  uint16_t SendAdd ;
  uint8_t SendLine ;
  uint8_t Command ;
  uint8_t i ;
  
  Data = 10 ;
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

      Message.Data[1] = EEProm[Data+5+(i<<1)] ;
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

int EnableKeycheck ;

void cli(void) 
{
  EnableKeycheck = 0 ;
}

void sei(void)
{
  EnableKeycheck = 1 ;
} 

void TIM3_IRQHandler(void)
{
  static uint16_t ct0, ct1;
  static uint16_t rpt;
  uint16_t i ;
  uint16_t PinStatus ;

  if (TIM3->SR&TIM_IT_Update) {
    TIM3->SR = (uint16_t)~TIM_IT_Update ;
    if (Heartbeat<=TIMEOUT+1) Heartbeat++ ;
    system_time++ ;
    if (EnableKeycheck) {
      PinStatus = ((GPIOB->IDR>>3)&0xf)|((GPIOC->IDR>>1)&0x30)|((GPIOC->IDR>>3)&0xC0) ; // collect all possible inputs
      i = key_state ^ PinStatus;      // key changed ?
      ct0 = ~( ct0 & i );                             // reset or count ct0
      ct1 = ct0 ^ (ct1 & i);                          // reset or count ct1
      i &= ct0 & ct1;                                 // count until roll over ?
      key_state ^= i;                                 // then toggle debounced state
      key_press |= key_state & i;                     // 0->1: key press detect
      
      if(!(key_state & REPEAT_MASK))            // check repeat function
	rpt = REPEAT_START;                          // start delay
      if(!( --rpt)){
	rpt = REPEAT_NEXT;                            // repeat delay
	key_rpt |= key_state & REPEAT_MASK;
      }
    } ;
  }
}

uint16_t get_key_press( uint16_t key_mask )
{
  cli();                                          // read and clear atomic !
  key_mask &= key_press;                          // read key(s)
  key_press ^= key_mask;                          // clear key(s)
  sei();
  return key_mask;
}
 
// check if a key has been pressed long enough such that the
// key repeat functionality kicks in. After a small setup delay
// the key is reported being pressed in subsequent calls
// to this function. This simulates the user repeatedly
// pressing and releasing the key.

uint16_t get_key_rpt( uint16_t key_mask )
{
  cli();                                          // read and clear atomic !
  key_mask &= key_rpt;                            // read key(s)
  key_rpt ^= key_mask;                            // clear key(s)
  sei();
  return key_mask;
}
 
uint16_t get_key_short( uint16_t key_mask )
{
  cli();                                          // read key state and key press atomic !
  return get_key_press( ~key_state & key_mask );
  sei();
}
 
uint16_t get_key_long( uint16_t key_mask )
{
  return get_key_press( get_key_rpt( key_mask ));
}


const int PortToPin[] = { 5,4,2,3,0,1,6,7 } ;  // mapping the Port to the Pin-Header

void InitMC (void)
{
  int i ;
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

  /* Enable the TIM3 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);


  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 999;
  TIM_TimeBaseStructure.TIM_Prescaler = 180;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2; // 36MHz / 1000 / 180 / 2 = 100 Hz
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
  
  sei () ;

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
  unsigned int Addr ;
  int i,j,r ;
  int LastCommand ;

  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000);
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
      Addr = ((unsigned int)InMessage.Data[1])+(((unsigned int)InMessage.Data[2])<<8) ;
      Message.Data[3]=EEProm[Addr] ;
      Message.DLC = 4 ;
      CAN_send_message(&Message) ;
      break ;

    case SET_VAR:
      Addr = ((unsigned int)InMessage.Data[1])+(((unsigned int)InMessage.Data[2])<<8) ;
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
	  //	  i =255-PWM[j] ;
	}
	//	START_PWM[j] = PWM[j] ;
	// SOLL_PWM[j] = i ;
	if (Config[j]) {
	  // TimerPWM[j] = DurationPWM[j] = Config[j] ;
	} else {
	  // TimerPWM[j] = DurationPWM[j] = 1 ;
	} ;
      } ;
      break ;


    default:
      break ;
    } ;
  } ;
}

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

/* EEProm-Belegung vom Base-STM:
   0   0xba
   1   0xca
   2   BoardAdd Low Byte
   3   BoardAdd High Byte
   4   BoardLine
   5   BootAdd Low Byte
   6   BootAdd High Byte
   7   BootLine
   8   BoardType (0x22: STM_Base_Board)  
   9   n/a

   EEProm-Belegung vom Sensor:
   10 : Pin 1 Short tic with timer running
   10 : Board Address Low 
   11 : Board Address High
   12 : Line address
   13 : Command
   14 : Data 1    or Add1  if Command = 33
   15 : Data 2    or Port1 if Command = 33
   16 : Data 3    or Add2  if Command = 33
   17 : Data 4    or Port2 if Command = 33
   18 : Data 5    or Add3  if Command = 33
   19 : Data 6    or Port3 if Command = 33


   20 : Pin 1 Long Tic with timer running
   or: 20 Add4  if Command=33
       21 Port4 if Command=33
       22 Add5  if Command=33
       23 Port5 if Command=33
       24 Add6  if Command=33
       25 Port6 if Command=33
       26 Add7  if Command=33
       27 Port7 if Command=33
       28 Add8  if Command=33
       29 Port8 if Command=33
		 
   30 : Pin 1 Short Tic without timer running
   40 : Pin 1 Long Tic without timer running

   50-89   : Pin 2
   90-129  : Pin 3
   130-169 : Pin 4
   170-209 : Pin 5
   210-249 : Pin 6
   250-289 : Pin 7
   290-329 : Pin 8
   
   350 : Configuration Pin 1
   351 : Additional Data Pin 1
   352 : Configuration Pin 2
   353 : Additional Data Pin 2
   354 : Configuration Pin 3
   355 : Additional Data Pin 3
   356 : Configuration Pin 4
   357 : Additional Data Pin 4
   358 : Configuration Pin 5
   359 : Additional Data Pin 5
   360 : Configuration Pin 6
   361 : Additional Data Pin 6
   362 : Configuration Pin 6
   363 : Additional Data Pin 6
   364 : Configuration Pin 6
   365 : Additional Data Pin 6
   366 : REPEAT_START
   367 : REPEAT_NEXT
   380 : PowerOut 1 enable
   381 : PowerOut 2 enable
   382 : WS 2812 Number of LED
   383 : WS 2812 Number of virtual LED

   Configuration: 1-9: Input-Funktionen
   1: Einfacher Input (Short Input)
   2: Short-Long-Input
   3: Digital-Input mit Timer (Monoflop)
   4: Digital-Input mit Timer (retriggerbar)
   5: Analog-Input
   6: Bewegungsmelder
   7: OC_Bewegungsmelder 

   10-19: Digital Output-Funktionen
   10: Ein-Aus
   11: PWM
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

int  Type[8] ;
int  Config[8];
volatile int system_time ;
volatile uint16_t  Heartbeat ;
volatile uint8_t Time ;
uint8_t  Running[8] ;
uint8_t BoardLine ;
uint16_t BoardAdd ;


volatile uint16_t  REPEAT_MASK ;
volatile uint16_t REPEAT_START ;
volatile uint16_t REPEAT_NEXT ;
volatile uint16_t  key_state;                               // debounced and inverted key state:
                                                           // bit = 1: key pressed
volatile uint16_t  key_press;                               // key press detect
volatile uint16_t  key_rpt;                                 // key long press and repeat

rgb_t  WSRGBStart[MAXWSNUM] ; // gets modified in main
rgb_t  WSRGBSoll[MAXWSNUM] ; // gets modified in main
uint16_t TimerLED[MAXWSNUM] ; // gets modified in main
uint16_t DurationLED[MAXWSNUM] ; // gets modified in main
uint8_t ChangedLED ;

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
  static uint8_t WSCounter ;

  if (TIM3->SR&TIM_IT_Update) {
    TIM3->SR = (uint16_t)~TIM_IT_Update ;
    if (Heartbeat<=TIMEOUT+1) Heartbeat++ ;
    system_time++ ;
    
    if (ChangedLED){
      if (ledBusy==0) {
	ChangedLED=0 ;
	WSupdate () ;
      } ;
    } ;

    WSCounter++ ;
    if (WSCounter>(uint8_t)1) {
      // Nur alle 20 ms updaten (50 Hz Update-Rate reicht); maximale Fading-Zeit liegt bei 25,5 Sekunden mit 0,1 Sekunde Aufloesung
      
      WSCounter = 0 ;
      // Berechnen des Sollwerts und Ausgeben desselben
      for (i=0;i<CurrentWSNum;i++) if (TimerLED[i]>0) break ;
      
      if (i<CurrentWSNum) {
	for (i=0;i<CurrentWSNum;i++) {
	  if (TimerLED[i]>0) {
	    TimerLED[i]-- ;
	  } ;
	  WSRGB[i].R = (uint8_t)((int16_t)WSRGBStart[i].R+
				 (int16_t)(((int32_t)WSRGBSoll[i].R-(int32_t)WSRGBStart[i].R)*
					   ((int32_t)DurationLED[i]-(int32_t)TimerLED[i])/(int32_t)DurationLED[i])) ;
	  WSRGB[i].G = (uint8_t)((int16_t)WSRGBStart[i].G+
				 (int16_t)(((int32_t)WSRGBSoll[i].G-(int32_t)WSRGBStart[i].G)*
					   ((int32_t)DurationLED[i]-(int32_t)TimerLED[i])/(int32_t)DurationLED[i])) ;
	  WSRGB[i].B = (uint8_t)((int16_t)WSRGBStart[i].B+
				 (int16_t)(((int32_t)WSRGBSoll[i].B-(int32_t)WSRGBStart[i].B)*
					   ((int32_t)DurationLED[i]-(int32_t)TimerLED[i])/(int32_t)DurationLED[i])) ;
	  if (!TimerLED[i]) {
	    DurationLED[i] = 1 ;
	    WSRGBStart[i].R = WSRGBSoll[i].R ;
	    WSRGBStart[i].G = WSRGBSoll[i].G ;
	    WSRGBStart[i].B = WSRGBSoll[i].B ;
	  } ;
	  ChangedLED=1 ;
	} ;
      } ;
    } ;
    
    
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
const uint32_t PortToGPin[] = { GPIO_Pin_3,GPIO_Pin_4,GPIO_Pin_5,GPIO_Pin_6,
				GPIO_Pin_6,GPIO_Pin_7,GPIO_Pin_10,GPIO_Pin_11 } ;

void InitMC (void)
{
  int i ;
  uint32_t InputPinsB,InputPinsC ;
  uint32_t OutputPinsB,OutputPinsC ;
  NVIC_InitTypeDef NVIC_InitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

  /* Enable the TIM3 global Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);

  InputPinsB = InputPinsC = OutputPinsB = OutputPinsC = 0 ;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 999;
  TIM_TimeBaseStructure.TIM_Prescaler = 180;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2; // 36MHz / 1000 / 180 / 2 = 100 Hz
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

  sei () ;
  
  for (i=0;i<8;i++) {
    // Configuration lesen

    Type[i] = EEProm[350+(i<<1)] ;
    Config[i] = EEProm[351+(i<<1)] ;

    switch (Type[i]) {
    case I_SIMPLE: // Klick-Input
    case I_SHORTLONG: // Short-Long-Input
    case I_MONO: // Monoflop
    case I_RETRIG: // Retriggerbares Monoflop
      // configure PIN as Input
      if (i<4) { 
	InputPinsB |= PortToGPin[i] ;
      } else {
	InputPinsC |= PortToGPin[i] ;
      } ;
      break ;
    case I_ANALOG: // Analog-Input, noch nicht implementiert
      break ;
    case O_ONOFF: // Ein-Aus
      if (i<4) { 
	OutputPinsB |= PortToGPin[i] ;
      } else {
	OutputPinsC |= PortToGPin[i] ;
      } ;
      break ;
    case O_PWM: // PWM
      break ;
    } ;
  } ;

  GPIO_InitStructure.GPIO_Pin = InputPinsB;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = InputPinsC;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = OutputPinsB;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = OutputPinsC;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  if ((EEProm[380]!=0)||(EEProm[381]!=0)) { // Power-PWM output enabled
    PowerInit () ;
  } ;
  if (EEProm[382]!=0) { // WS2812 LEDs 
    CurrentWSNum = EEProm[382] ;
    WSinit() ;
  } ;
}

void BlendLED (int Num, int R, int G, int B, int Duration)
{
  int i,j ;
  int Ch ;

  Ch = EEProm[382]/EEProm[383] ; // Ch is number of LED to be changed

  j = Ch*Num ;
  WSRGBSoll[j].R = R ;
  WSRGBSoll[j].G = G ;
  WSRGBSoll[j].B = B ;
  
  if (Num!=0) { // Modify LED in front of desired LED
    j = Ch*(Num-1) ;
    for (i=1;i<=Ch;i++) {
      WSRGBSoll[i+j].R = (int)WSRGBSoll[j].R+(((int)WSRGBSoll[j+Ch].R-(int)WSRGBSoll[j].R)*i)/Ch; 
      WSRGBSoll[i+j].G = (int)WSRGBSoll[j].G+(((int)WSRGBSoll[j+Ch].G-(int)WSRGBSoll[j].G)*i)/Ch; 
      WSRGBSoll[i+j].B = (int)WSRGBSoll[j].B+(((int)WSRGBSoll[j+Ch].B-(int)WSRGBSoll[j].B)*i)/Ch; 
      if (TimerLED[i+j]==0) {
	DurationLED[i+j]=TimerLED[i+j]=Duration; 
      } ; 
    }; 
  }; 
  if (Num!=EEProm[383]-1) { // Modify LED after desired LED
    j = Ch*Num ;
    for (i=0;i<Ch;i++) {
      WSRGBSoll[i+j].R = (int)WSRGBSoll[j].R+(((int)WSRGBSoll[j+Ch].R-(int)WSRGBSoll[j].R)*i)/Ch; 
      WSRGBSoll[i+j].G = (int)WSRGBSoll[j].G+(((int)WSRGBSoll[j+Ch].G-(int)WSRGBSoll[j].G)*i)/Ch; 
      WSRGBSoll[i+j].B = (int)WSRGBSoll[j].B+(((int)WSRGBSoll[j+Ch].B-(int)WSRGBSoll[j].B)*i)/Ch; 
      if (TimerLED[i+j]==0) {
	DurationLED[i+j]=TimerLED[i+j]=Duration; 
      } ; 
    }; 
  } ;
}

// Hauptprogramm
 
int main(void) 
{
  CanRxMsg InMessage ;
  unsigned int Addr ;
  int j,r ;
  int LastCommand ;

  NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x4000);
  // Default-Werte:
  BoardAdd = 0xFF ;
  BoardLine = 0xF ;

  // Lesen der EEProm-Konfiguration

  EEPromInit () ;
  
  if (EEProm==NULL) {
    // Config has not yet been written - restart Bootloader to enable upload of bootstrap
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
      if (j>(uint8_t)7) j=0 ;
      for (;(j<InMessage.Data[1])&&(j<8);j++) { // Wenn Port = 1..6, dann nur diesen, sonst alle
	if ((Type[j]!=(uint8_t)O_ONOFF)&&(Type[j]!=(uint8_t)O_PWM)) continue ; // Illegaler PIN
	if (r==(uint8_t)CHANNEL_ON) {
	  if (j<4) {
	    GPIOB->BSRR = PortToGPin[j] ;
	  } else {
	    GPIOC->BSRR = PortToGPin[j] ;
	  } ;
	} else if (r==(uint8_t)CHANNEL_OFF) {
	  if (j<4) {
	    GPIOB->BRR = PortToGPin[j] ;
	  } else {
	    GPIOC->BRR = PortToGPin[j] ;
	  } ;
	} else {
	  if (j<4) {
	    if ((GPIOB->ODR & PortToGPin[j])!=0) {
	      GPIOB->BRR = PortToGPin[j] ;
	    } else {
	      GPIOB->BSRR = PortToGPin[j] ;
	    } ;
	  } else {
	    if ((GPIOC->ODR & PortToGPin[j])!=0) {
	      GPIOC->BRR = PortToGPin[j] ;
	    } else {
	      GPIOC->BSRR = PortToGPin[j] ;
	    } ;
	  } ;
	} ;
      } ;
      break ;
    case SET_PIN:
    case DIM_TO:
      if (r==DIM_TO) {
	r = 3 ;
      } else {
	r = 0 ;
      } ;
      j-- ;
      if (j>(uint8_t)5) j=0 ;
      for (;(j<Message.Data[1])&&(j<6);j++) { // Wenn Port = 1..6, dann nur diesen, sonst alle
	if ((Type[j]!=O_ONOFF)&&(Type[j]!=O_PWM)) break ; // Illegaler PIN
	//	cli () ;
	//	START_PWM[j] = PWM[j] ;
	//	SOLL_PWM[j] = Message.data[2+r] ;
	//	TimerPWM[j] = DurationPWM[j] = (Message.data[3+r]<<2)+1 ;
	//	sei () ;
      } ;
      break ;
    case LOAD_LED:
      if ((j+1)>(EEProm[382]/EEProm[383])) break; // Zu hohe LED-Nummer
      BlendLED(j,Message.Data[2],Message.Data[3],Message.Data[4],(Message.Data[5]<<2)+1) ;
      break ;
    default:
      break ;
    } ;
  } ;
}

/*
  Sensor über CAN; Applikationsprogramm für den Bootloader.

  ATtiny84 @ 8 MHz
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

   300 : Configuration Pin 1
   301 : Additional Data Pin 1
   302 : Configuration Pin 2
   303 : Additional Data Pin 2
   304 : Configuration Pin 3
   305 : Additional Data Pin 3
   306 : Configuration Pin 4
   307 : Additional Data Pin 4
   308 : Configuration Pin 5
   309 : Additional Data Pin 5
   310 : Configuration Pin 6
   311 : Additional Data Pin 6
   312 : REPEAT_START
   313 : REPEAT_NEXT

   Configuration: 1-9: Input-Funktionen
   1: Einfacher Input (Short Input)
   2: Short-Long-Input
   3: Digital-Input mit Timer (Monoflop)
   4: Digital-Input mit Timer (retriggerbar)
   5: Analog-Input
   6: Bewegungsmelder
   7: OC_Bewegungsmelder 
   8: Licht  - SDA liegt auf A0, SCL liegt auf A1

   10-19: Digital Output-Funktionen
   10: Ein-Aus
   11: PWM
   20-29: Kommunikationsfunktion
   20: WS2801 Clock
   21: WS2801 Data
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

#define MAX_LEDS 20

#define LED0 B,0
#define LED1 B,1

#define TIMER_PRESET 177

#define TIMEOUT 1000 // 10 Sekunden Timeout

// globale Variablen

can_t    Message ;
uint8_t  BoardLine ;
uint16_t BoardAdd ;

uint8_t  LastCommand ;
volatile uint16_t  Heartbeat ;
volatile uint8_t Time ;
volatile uint16_t Timers[6] ;
uint8_t  TimerStatus ;
uint8_t  Type[6] ;
uint8_t  Config[6];
uint8_t  Running[6] ;

volatile uint8_t  PWMStep ;
volatile uint8_t  PWM[6] ;
volatile uint8_t  PWMTime[6] ;
volatile uint8_t  PWMOut[6] ;

volatile uint8_t* PWMPort[6] ;
volatile uint8_t  PWMPin[7] ;
volatile uint8_t SOLL_PWM[6] ;
volatile uint8_t START_PWM[6] ;
volatile uint16_t TimerPWM[6] ;
volatile uint16_t DurationPWM[6] ;

volatile uint8_t  SOLL_WS[MAX_LEDS*3] ;
volatile uint8_t  START_WS[MAX_LEDS*3] ;
volatile int16_t TimerLED[MAX_LEDS] ;
volatile int16_t DurationLED[MAX_LEDS] ;
volatile uint8_t ChangedLED ;
volatile uint8_t  NumLED ;

volatile uint8_t* PIX_Clock ;
volatile uint8_t* PIX_Data ;
volatile uint8_t  PIX_CL ;
volatile uint8_t  PIX_DA ;

volatile uint8_t  REPEAT_MASK ;
volatile uint16_t REPEAT_START ;
volatile uint16_t REPEAT_NEXT ;
volatile uint8_t  key_state;                               // debounced and inverted key state:
                                                           // bit = 1: key pressed
volatile uint8_t  key_press;                               // key press detect
volatile uint8_t  key_rpt;                                 // key long press and repeat


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
  *FromAdd = (uint16_t) ((CANId>>14)&0xff) ;
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

void SetOutMessage (uint8_t BoardLine,uint16_t BoardAdd)
{
  uint8_t SendLine ;
  uint16_t SendAdd ;
  
  GetSourceAddress(Message.id,&SendLine,&SendAdd) ;
  Message.id = BuildCANId (0,0,BoardLine,BoardAdd,SendLine,SendAdd,0) ;
  Message.data[0] = Message.data[0]|SUCCESSFULL_RESPONSE ;
  Message.length = 1 ;
}

void SendPinMessage (uint8_t Pin, uint8_t Long,uint8_t SendData)
{
  uint8_t *Data ;
  uint16_t SendAdd ;
  uint8_t SendLine ;
  uint8_t Command ;
  uint8_t i ;
  uint8_t Timer ;
  
  Timer = (Heartbeat>TIMEOUT)?1:0 ; 
  Data = (uint8_t*)10 ;
  Data += Pin*40 ;
  if (Timer == 1) Data +=20 ;
  if (Running[(int)Pin]==0) return ; // Dieser Pin ist ausgeschaltet worden
  
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
    Message.data[1] = (SendData==0)?eeprom_read_byte(Data+4):SendData ;
    Message.data[2] = eeprom_read_byte(Data+5) ;
    Message.data[3] = eeprom_read_byte(Data+6) ;
    Message.data[4] = eeprom_read_byte(Data+7) ;
    Message.data[5] = eeprom_read_byte(Data+8) ;
    Message.data[6] = eeprom_read_byte(Data+9) ;
    Message.data[7] = Heartbeat>90?Time+1:Time ;
    Message.length = 8 ;
    mcp2515_send_message(&Message) ;
  } ;
}

void ws2801_writeByte(uint8_t Send)
{
  register uint8_t BitCount = 8; // store variable BitCount in a cpu register
  do {
    PIX_Clock[0] &= ~(PIX_CL);	// set clock LOW
    // send bit to ws2801. we do MSB first
    if (Send & 0x80) {
      PIX_Data[0] |= (PIX_DA); // set output HIGH
    } else {
      PIX_Data[0] &= ~(PIX_DA); // set output LOW
    } ;
    PIX_Clock[0] |= (PIX_CL); // set clock HIGH
    // next bit
    Send <<= 1;
  } while (--BitCount);
} // ws2801_writeByte



// Haupt-Timer-Interrupt, wird alle 10 ms aufgerufen
// Setzt den Heartbeat, die Timer, das Dimmen der WS2801 und sorgt fuer das Tastenentprellen
// der an (a0:a3) und (b0:b3) angeschlossenen Tasten (wenn entsprechend konfiguriert)

ISR( TIM0_OVF_vect )                           
{
  static uint8_t ct0, ct1;
  static uint16_t rpt;
  static uint8_t WSCounter ;
  uint8_t WSByte ;
  uint8_t i,j;
 
  TCNT0 = (uint8_t)TIMER_PRESET;  // preload for 10ms

  if (Heartbeat<=TIMEOUT+1) Heartbeat++ ;
  for (i=0;i<6;i++) if (Timers[i]>0) Timers[i]-- ;

  WSCounter++ ;
  if (WSCounter>1) {
    // Nur alle 20 ms updaten (50 Hz Update-Rate reicht); maximale Fading-Zeit liegt bei 25,5 Sekunden mit 0,1 Sekunde Aufloesung
    WSCounter = 0 ;
    // Berechnen des Sollwerts und Ausgeben desselben
    for (i=0;i<NumLED;i++) if (TimerLED[i]>0) break ;
    
    if (i<NumLED) {
      for (i=0;i<NumLED*3;i++) {
	j = i/3 ;
	if ((i%3==0)&&(TimerLED[j]>0)) TimerLED[j]-- ;
	WSByte = (uint8_t)(((uint16_t)START_WS[i])+(((int16_t)((int16_t)SOLL_WS[i]-(int16_t)START_WS[i]))*(DurationLED[j]-TimerLED[j])/DurationLED[j])) ;
	ws2801_writeByte(WSByte) ;
	if (TimerLED[j]==0) {
	  DurationLED[j] = 1 ;
	  START_WS[i] = SOLL_WS[i] ;
	} ;
	ChangedLED=1 ;
      } ;
      PIX_Clock[0] &= ~(PIX_CL) ; //Clock Low zum Latchen
    } else {
      if (ChangedLED==1){
	ChangedLED=0 ;
	// Noch einmal den letzten Wert ausgeben, damit der Wert übernommen wird (das Pixel übernimmt erst mit
	// Beginn des nächsten Frames die Werte in die Ausgabe.
	for (i=0;i<NumLED*3;i++) {
	  ws2801_writeByte(START_WS[i]) ;
	} ;
	PIX_Clock[0] &= ~(PIX_CL) ; //Clock Low zum Latchen
      } ;
    } ;
  } ;
  
  i = key_state ^ (((PINB&0x7)<<3)|(PINA&0x7));      // key changed ?
  ct0 = ~( ct0 & i );                             // reset or count ct0
  ct1 = ct0 ^ (ct1 & i);                          // reset or count ct1
  i &= ct0 & ct1;                                 // count until roll over ?
  key_state ^= i;                                 // then toggle debounced state
  key_press |= key_state & i;                     // 0->1: key press detect
 
  if( (key_state & REPEAT_MASK) == 0 )            // check repeat function
    rpt = REPEAT_START;                          // start delay
  if( --rpt == 0 ){
    rpt = REPEAT_NEXT;                            // repeat delay
    key_rpt |= key_state & REPEAT_MASK;
  }
}
 
// check if a key has been pressed. Each pressed key is reported
// only once
uint8_t get_key_stat (uint8_t key_mask)
{
  return ((key_mask&(((PINB&0x7)<<3)|(PINA&0x7)))!=0);
}


uint8_t get_key_press( uint8_t key_mask )
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

uint8_t get_key_rpt( uint8_t key_mask )
{
  cli();                                          // read and clear atomic !
  key_mask &= key_rpt;                            // read key(s)
  key_rpt ^= key_mask;                            // clear key(s)
  sei();
  return key_mask;
}
 
uint8_t get_key_short( uint8_t key_mask )
{
  cli();                                          // read key state and key press atomic !
  return get_key_press( ~key_state & key_mask );
  sei();
}
 
uint8_t get_key_long( uint8_t key_mask )
{
  return get_key_press( get_key_rpt( key_mask ));
}


// Sortieren der PWM-Tabelle und bestimmen der Einschaltzeiten
void UpdatePWM (void)
{
  uint8_t i,j,k ;
  uint8_t PT[6] ;
  static uint8_t Timing=0 ;

  for (i=0;i<6;i++) { PWMTime[i] = 255 ; PWMOut[i] = 0 ; } ;
  for (i=0;i<6;i++) { // Alle durchgehen
    if (Timing>23) {
      if (TimerLED[i]>0) { 
	TimerLED[i]-- ;
	PWM[i] = (uint8_t)(((uint16_t)START_PWM[i])+(((int16_t)((int16_t)SOLL_PWM[i]-(int16_t)START_PWM[i]))*(DurationPWM[j]-TimerPWM[j])/DurationPWM[j])) ;
      } else {
	START_PWM[i]=SOLL_PWM[i] ;
      } ;
    }; 
    for (j=0;j<i;j++) if (PWM[i]<=PWMTime[j]) break ;
    for (k=5;k>j;k--) { PWMTime[k] = PWMTime[k-1] ; PWMOut[k] = PWMOut[k-1] ; } ;
    PWMTime[j] = PWM[i] ;
    if (PWM[i]==255) {  
      PWMOut[j]=6 ; // If full on never switch off
    } else {
      PWMOut[j] = i ;
    } ;
  } ;
  if (Timing>23) {
    Timing=0 ;
  } else {
    Timing++ ;
  } ;
  for (i=1;i<6;i++) PT[i] = PWMTime[i]-PWMTime[i-1] ;
  for (i=1;i<6;i++) PWMTime[i] = PT[i] ;
  PWMTime[6] = 255-PWMTime[5]-PWMTime[4]-PWMTime[3]-PWMTime[2]-PWMTime[1]-PWMTime[0] ;
}
    
// Anschalten des angegebenen Ports

inline void PortOn(uint8_t Port)
{
  if (PWMPin[Port]!=0) PWMPort[Port][0]|=PWMPin[Port] ;
}

// Ausschalten des angegebenen Ports

inline void PortOff(uint8_t Port)
{
  if (PWMPin[Port]!=0) PWMPort[Port][0]&=~(PWMPin[Port]) ;
}
  

// Timer1-Interrup-Service-Routine der PWM-Generierung

ISR(TIM1_COMPA_vect) 
{
  uint8_t i ;

  if (PWMStep==0) {
    UpdatePWM() ;
    if (PWMTime[0]<4) TCNT1 = 0 ;            // reset Timer, else UpdatePWM might be too long
    OCR1A = PWMTime[0]<<1 ;
    for (i=0;i<6;i++)
      if (PWM[i]!=0) PortOn(i) ;
    PWMStep++ ;
  } else {
    do {
      PortOff(PWMOut[PWMStep-1]) ;
      PWMStep++ ;
    } while ((PWMTime[PWMStep-1]==0)&&(PWMStep<7)) ;
    OCR1A = PWMTime[PWMStep-1]<<1 ;
    for (i=PWMStep;i<7;i++) if (PWMTime[i]!=0) break ; // If no further shut off follows, set to PWMStep7
  } ;
  if ((i==7)||(PWMStep==7)) {
    PWMStep = 0 ;
  } ;
}

// Initialisieren des MC und setzen der Port-Eigenschaften
// in Abhaengigkeit von der Konfiguration

void InitMC (void)
{
  uint8_t i ;
    
  // Timer 1 OCRA1, als variablem Timer nutzen
  TCCR1B = 3|8;             // Timer laeuft mit Prescaler/64 bis OCR1A
  TCNT1 = 0;              // Timer auf Null stellen
  OCR1A = 512;           // Overflow auf 512
  TIMSK1 |= (1 << OCIE1A);   // Interrupt freischalten

  // Timer 0 als 10 ms-Timer verwenden
  TCCR0B = (1<<CS02)|(1<<CS00);  // divide by 1024
  TCNT0 = (uint8_t)TIMER_PRESET; // preload for 10ms
  TIMSK0 |= 1<<TOIE0;            // enable timer interrupt
  
  // Port-Konfiguration durchgehen
  PWMPin[6]=0 ;

  for (i=0;i<6;i++) {
    // Konfigurations-Byte lesen
    Type[i] = eeprom_read_byte((uint8_t*)(300+(i<<1))) ;
    Config[i] = eeprom_read_byte((uint8_t*)(301+(i<<1))) ;
    PWM[i] = 0 ;

    switch (Type[i]) {
    case I_SIMPLE: // Klick-Input
    case I_SHORTLONG: // Short-Long-Input
    case I_MONO: // Monoflop
    case I_RETRIG: // Retriggerbares Monoflop
      // Einfacher Input
      if (Type[i]==I_SHORTLONG) {
	REPEAT_START = eeprom_read_byte((uint8_t*)312)*10 ; // in 1/10 Sekunden
	REPEAT_NEXT  = eeprom_read_byte((uint8_t*)313)*10 ;  // in 1/10 Sekunden
	REPEAT_MASK |= (1<<i) ;
      } ;
      if (i<3) {
	// Port A
	PORTA &= ~(1<<i) ;
	DDRA &= ~(1<<i) ;
      } else {
	// Port B
	PORTB &= ~(1<<(i-3)) ;
	DDRB &= ~(1<<(i-3)) ;
      } ;
      break ;
    case I_ANALOG: // Analog-Input
      // ADC Konverter initialisieren; es kann immer nur ein Port ADC-Port sein, hier wird jedoch
      // dies nicht abgefragt -> der letzte angegebene Port ist ADC port
      // Port kann nur in Port A liegen
      if (i>2) break ;
      PORTA &= ~(1<<i) ;
      DDRA &= ~(1<<i) ;
      ADMUX = i ; // VCC Reference voltage, PortA0-PortA2 als Eingang
      ADCSRB = 1<<4 ; // Right adjusted, Unipolar, No comparator, Free-Running
      ADCSRA = (1<<7)||(1<<6)||(1<<5)||(1<<2)||(1<<1)||(1<<0) ; // ADC Enable, ADC On, ADC FreeRun, Clock/128
      break ;
    case O_ONOFF: // Ein-Aus
    case O_PWM: // PWM
    case O_WSCLOCK: // WS2801 Clock
    case O_WSDATA: // WS2801 Data
      // Ausgabe-Port (Ein-Aus oder PWM), wird entsprechend dem Config Byte initalisiert
      if (Type[i]==O_WSCLOCK) { // WS Clock: Pointer auf den richtigen Port-Pin setzen
	if (i<3) { 
	  PIX_Clock = &PORTA ;
	  PIX_CL = 1<<i ;
	} else {
	  PIX_Clock = &PORTB ;
	  PIX_CL = 1<<(i-3) ;
	} ;
      } ;
      if (Type[i]==O_WSDATA) { // WS Data: Pointer auf den richtigen Port-Pin setzen
	if (i<3) { 
	  PIX_Data = &PORTA ;
	  PIX_DA = 1<<i ;
	} else {
	  PIX_Data = &PORTB ;
	  PIX_DA = 1<<(i-3) ;
	} ;
      } ;
      if (i<3) {
	// Port A
	PWMPort[i] = &PORTA ;
	if (Type[i]==O_PWM){
	  PWMPin[i] = 1<<i ;
	} else {
	  if (Config[i]>0) {
	    PORTA |= (1<<i) ;
	  } else {
	    PORTA &= ~(1<<i) ;
	  } ;
	  PWMPin[i] = 0 ;
	} ;
	DDRA |= (1<<i) ;
      } else {
	// Port B
	PWMPort[i] = &PORTB ;
	if (Type[i]==O_PWM){
	  PWMPin[i] = 1<<(i-3) ;
	} else {
	  if (Config[i]>0) {
	    PORTB &= ~(1<<(i-3)) ;
	  } else {
	    PORTB |= (1<<(i-3)) ;
	  } ;
	  PWMPin[i] = 0 ;
	} ;
	DDRB |= (1<<(i-3)) ;
      } ;
      break ;
    } ;
  } ;	  
}	



// Hauptprogramm
 
int main(void) 
{
  uint8_t r ;
  uint8_t i,j ;
  uint16_t Addr ;
  
  // Default-Werte:
  BoardAdd = 16 ;
  BoardLine = 1 ;
 

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

  for (r=0;r<6;r++) Running[r] = 1 ; // Alle Eingänge sind aktiv
  
  // Ports, Timer, ADC initialisieren
  InitMC() ;
 
  sei();                  // Interrupts gloabl einschalten

  // Say Hello, world...
  
  Message.id = BuildCANId (0,0,BoardLine,BoardAdd,0,1,0) ;
  Message.data[0] = IDENTIFY ;
  Message.length = 1 ;
  mcp2515_send_message(&Message) ;

  // Endlosschleife zur Abarbeitung der Kommandos

  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=mcp2515_get_message(&Message)) == NO_MESSAGE) {
      /* Ports verarbeiten */
      for (i=0,j=1;i<6;i++,j=j<<1) {
	// Aktion je nach Konfigurations-Byte ausfuehren
	switch (Type[i]) {
	case I_SIMPLE: // Einfacher Eingang
	  if (get_key_press(j)) {
	    SendPinMessage(i,0,0) ;
	  } ;
	  break ;
	case I_SHORTLONG: // Kurz oder lang gedrueckt
	  if (get_key_short(j)) {
	    SendPinMessage(i,0,0) ;
	  } else if (get_key_long(j)) {
	    SendPinMessage(i,1,0) ;
	  } ;
	  break ;
	case I_MONO: // Nicht-Nachstellbares Monoflop
	  if (Timers[i]==0) {
	    if (get_key_press(j)) {
	      SendPinMessage(i,0,0) ;
	      Timers[i] = ((uint16_t)Config[i])*100 ;
	      TimerStatus |= j ;
	    } else {
	      if ((TimerStatus&(j))>0) {
     		SendPinMessage(i,1,0) ;
	    	TimerStatus &= ~(j) ;
	      } ;
	    } ;
	  } ;
	  break ;
	case I_RETRIG: // Nachstellbares Monoflop
	  if (get_key_press(j)) {
	    if (Timers[i]==0) {
	      SendPinMessage(i,0,0) ;
	    } ;
	    Timers[i] = ((uint16_t)Config[i])*100 ;
	    TimerStatus |= j ;
	  } ;
	  if (Timers[i]==0) {
	    if ((TimerStatus&(j))>0) {
	      SendPinMessage(i,1,0) ;
	      TimerStatus &= ~(j) ;
	    } ;
	  } ;
	  break ;
  	case I_BWM: // Nachstellbares Monoflop als Bewegungsmelder
	case I_BWM2:
	  r=!get_key_stat(j) ;
	  if (Type[i]==I_BWM2) r=!r ;
	  if (r) {
	    if (Timers[i]==0) {
	      SendPinMessage(i,0,0) ;
	    } ;
	    Timers[i] = ((uint16_t)Config[i])*100 ;
	    TimerStatus |= j ;
	  } ;
	  if (Timers[i]==0) {
	    if ((TimerStatus&(j))>0) {
	      SendPinMessage(i,1,0) ;
	      TimerStatus &= ~(j) ;
	    } ;
	  } ;
	  break ;

	case I_ANALOG: // Analog-Input, wird alle ConfigByte-Sekunden auf dem Bus ausgegeben.
	  if (Timers[i]==0) {
	    r = ADCH ;
	    SendPinMessage(i,0,r) ;
	    Timers[i] = ((uint16_t)Config[i])*100 ;
	  } ;
	} ;
      } ;
    };
    
    // Kommando extrahieren
    r = Message.data[0] ;
    j = Message.data[1] ;

    // Sende-Addresse zusammenstoepseln (enthält auch die Quelladdresse aus Message,
    // ueberschreibt dann die In-Message)
    SetOutMessage(BoardLine,BoardAdd) ;

    // Befehl abarbeiten
    switch (r) {
      
    case SEND_STATUS:
      // Wartet auf einen Tastendruck und sendet dann die Port-Nummer des Tasters
      do {
	for (i=0;i<6;i++) {
	  if (get_key_press(1<<i)) break ;
	}
      } while (i==6) ;
      
      Message.data[1] = i ;
      Message.length = 2 ;
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
    case TIME:
      Heartbeat = 0 ;
      Time = j ;
      break ;
      // Diese Befehle sind beim Sensor nicht bekannt
      // LED
    case SET_TO:
    case HSET_TO:
    case L_AND_S:
    case SET_TO_G1:
    case SET_TO_G2:
    case SET_TO_G3:
    case LOAD_PROG:
    case START_PROG:
    case STOP_PROG:
      break ;
      // Relais-Befehle
    case CHANNEL_ON:
    case CHANNEL_OFF:
    case CHANNEL_TOGGLE:
      j-- ;
      if (j>5) break; // Illegaler PIN
      if ((Type[j]!=O_ONOFF)&&(Type[j]!=O_PWM)) break ; // Illegaler PIN
      if (r==CHANNEL_ON) {
	i = 255 ;
      } else if (r==CHANNEL_OFF) {
	i = 0 ;
      } else {
	i =255-PWM[j] ;
      }
      START_PWM[j] = PWM[j] ;
      SOLL_PWM[j] = i ;
      if (Config[j]>0) {
	TimerPWM[j] = DurationPWM[j] = Config[j] ;
      } else {
	TimerPWM[j] = DurationPWM[j] = 1 ;
      } ;
      break ;
    case SHADE_UP_FULL:
    case SHADE_DOWN_FULL:
    case SHADE_UP_SHORT:
    case SHADE_DOWN_SHORT:
      break ;
      // Nun die Sensor-Befehle
    case SET_PIN:
      j-- ;
      if (j>5) break; // Illegaler PIN
      if ((Type[j]!=O_ONOFF)&&(Type[j]!=O_PWM)) break ; // Illegaler PIN
      START_PWM[j] = PWM[j] ;
      SOLL_PWM[j] = Message.data[2] ;
      TimerPWM[j] = DurationPWM[j] = Message.data[3]+1 ;
      break ;
    case LOAD_LED:
      if ((j+1)>MAX_LEDS) break; // Zu hohe LED-Nummer
      NumLED = (j+1)>NumLED?(j+1):NumLED ; // Set Max used LED
      START_WS[j*3] = SOLL_WS[j*3] ;
      START_WS[j*3+1] = SOLL_WS[j*3+1] ;
      START_WS[j*3+2] = SOLL_WS[j*3+2] ;
      SOLL_WS[j*3] = Message.data[2] ;
      SOLL_WS[j*3+1] = Message.data[3] ;
      SOLL_WS[j*3+2] = Message.data[4] ;
      DurationLED[j] = TimerLED[j] = Message.data[5]*5+1 ;
      break ;
    case OUT_LED:
      break ;
    case START_SENSOR:
      Running[(int)j-1] = 1 ;
      break ;
    case STOP_SENSOR:
      Running[(int)j-1] = 0 ;
      break ;
    } ;
  } ;
}

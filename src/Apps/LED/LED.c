/*
    Eine 8-kanalige PWM mit intelligentem Lösungsansatz
 
    ATmega32 @ 8 MHz
 
*/
 
// Defines an den Controller und die Anwendung anpassen
 
#define F_PWM         90L               // PWM-Frequenz in Hz
#define PWM_PRESCALER 8                  // Vorteiler für den Timer
#define PWM_STEPS     1024                // PWM-Schritte pro Zyklus(1..256)
#define PWM_PORT1      PORTB              // Port für PWM
#define PWM_PORT2      PORTC
#define PWM_PORT3      PORTD
#define PWM_PORT4      PORTE
#define PWM_DDR1       DDRB               // Datenrichtungsregister für PWM
#define PWM_DDR2       DDRC 
#define PWM_DDR3       DDRD 
#define PWM_DDR4       DDRE 
#define PWM_CHANNELS  22                 // Anzahl der PWM-Kanäle

 
// ab hier nichts ändern, wird alles berechnet
 
#define T_PWM (F_CPU/(PWM_PRESCALER*F_PWM*PWM_STEPS)) // Systemtakte pro PWM-Takt
 
#if ((T_PWM*PWM_PRESCALER)<(111+5))
    #error T_PWM zu klein, F_CPU muss vergrösst werden oder F_PWM oder PWM_STEPS verkleinert werden
#endif
 
#if ((T_PWM*PWM_STEPS)>65535)
    #error Periodendauer der PWM zu gross! F_PWM oder PWM_PRESCALER erhöhen.   
#endif
// includes
 
#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "simple_mcp2515.h"


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
*/
// globale Variablen


uint16_t Actual[PWM_CHANNELS] ;
int16_t Delta[PWM_CHANNELS] ;
uint8_t Counter[PWM_CHANNELS] ;
uint8_t Step[PWM_CHANNELS] ;



const uint16_t LogCurve[] PROGMEM =  {
  0,  1,  1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,  7,
  7,  8,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 28, 30, 31, 32, 34, 36, 37, 38, 40, 42, 43, 44, 46, 48, 50, 52, 54, 56, 58,
  60, 62, 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 87, 90, 92, 94, 97,100,102,
  104,107,110,112,114,117,120,123,126,129,132,135,138,141,144,147,150,153,156,159,
  162,165,168,171,174,178,182,185,188,192,196,199,202,206,210,213,216,220,224,228,
  232,236,240,244,248,252,256,260,264,268,272,276,280,284,288,292,296,301,306,310,
  314,319,324,328,332,336,340,345,350,355,360,365,370,375,380,385,390,395,400,405,
  410,415,420,425,430,435,440,445,450,456,462,467,472,478,484,489,494,500,506,511,
  516,522,528,534,540,546,552,558,564,570,576,582,588,594,600,606,612,618,624,630,
  636,643,650,656,662,669,676,682,688,695,702,708,714,721,728,735,742,749,756,765,
  770,777,784,791,798,805,812,819,826,833,840,847,854,862,870,877,884,892,900,907,
  914,922,930,937,944,952,960,969,975,983,993,1002,1013,1023};

const uint16_t LED_TO_R [] PROGMEM = { 0,9,1,18,12,4,7} ;
const uint16_t LED_TO_G [] PROGMEM = { 8,10,2,19,13,5,15} ;
const uint16_t LED_TO_B [] PROGMEM = { 16,11,3,20,14,6,21} ;

	
// globale Variablen

 
uint16_t pwm_timing[PWM_CHANNELS+1];          // Zeitdifferenzen der PWM Werte
uint16_t pwm_timing_tmp[PWM_CHANNELS+1];      
uint16_t pwm_setting[PWM_CHANNELS+1];           // Einstellungen für die einzelnen PWM-Kanäle
uint8_t pwm_mask[(PWM_CHANNELS+1)*3];            // Bitmaske für PWM Bits, welche gelöscht werden sollen
uint8_t pwm_mask_tmp[(PWM_CHANNELS+1)*3];        

 
volatile uint16_t pwm_cnt_max=1;               // Zählergrenze, Initialisierung mit 1 ist wichtig!
volatile uint8_t pwm_sync;                    // Update jetzt möglich
volatile uint8_t pwm_updated ;
 
// Pointer für wechselseitigen Datenzugriff
 
uint16_t* isr_ptr_time  = pwm_timing;
uint16_t* main_ptr_time = pwm_timing_tmp;
uint8_t* isr_ptr_mask  = pwm_mask;
uint8_t* main_ptr_mask = pwm_mask_tmp;
uint8_t   pwm_cnt_max_tmp ;

// Zeiger austauschen
// das muss in einem Unterprogramm erfolgen,
// um eine Zwischenspeicherung durch den Compiler zu verhindern
 
static inline void tausche_zeiger(void) 
{
  uint16_t* tmp_ptr16;
  uint8_t* tmp_ptr32;
  
  tmp_ptr16 = isr_ptr_time;
  isr_ptr_time = main_ptr_time;
  main_ptr_time = tmp_ptr16;
  tmp_ptr32 = isr_ptr_mask;
  isr_ptr_mask = main_ptr_mask;
  main_ptr_mask = tmp_ptr32;
  pwm_cnt_max = pwm_cnt_max_tmp ;
}

void setmask(uint8_t *mask, uint8_t index, uint32_t val)
{
  uint8_t *where ;
  where = (uint8_t*)&(val) ;
  mask[(index<<1)+index] = where[0] ;
  mask[(index<<1)+index+1] = where[1] ;
  mask[(index<<1)+index+2] = where[2] ;
}

void readmask(uint8_t *mask, uint8_t index, uint32_t *val)
{
  uint8_t *where ;
  where = (uint8_t*)(val) ;
  where[0] = mask[(index<<1)+index] ;
  where[1] = mask[(index<<1)+index+1] ;
  where[2] = mask[(index<<1)+index+2] ;
  where[3] = 0 ;
} ;

// PWM Update, berechnet aus den PWM Einstellungen
// die neuen Werte für die Interruptroutine
 
void pwm_update(void) 
{
  uint8_t i, j, k;
  uint16_t tim_k ;
  uint16_t min;
  uint32_t tmp,tmp2;
  
  // PWM Maske für Start berechnen
  // gleichzeitig die Bitmasken generieren und PWM Werte kopieren
  
  pwm_sync = 0 ; // Swap vermeiden während update
  
  tmp = 0;
  tmp2= 1;
  
  for (i=0;i<PWM_CHANNELS;i++) pwm_setting[i+1] = pgm_read_word(&LogCurve[(Actual[i]>>8)]) ;
  
  
  for(i=1; i<=(PWM_CHANNELS); i++) {
    setmask(main_ptr_mask,i,~tmp2) ;
    if (pwm_setting[i]!=0) tmp |= tmp2;        // Maske zum setzen der IOs am PWM Start
    tmp2 <<= 1;
  }
  
  setmask(main_ptr_mask,0,tmp) ;
  
  // PWM settings sortieren; Einfügesortieren
  
  for(i=1; i<=PWM_CHANNELS; i++) {
    min=PWM_STEPS-1;
    k=i;
    for(j=i; j<=PWM_CHANNELS; j++) {
      if (pwm_setting[j]<min) {
	k=j;                                // Index und PWM-setting merken
	min = pwm_setting[j];
      }
    }
    if (k!=i) {
      // ermitteltes Minimum mit aktueller Sortiertstelle tauschen
      tim_k = pwm_setting[k];
      pwm_setting[k] = pwm_setting[i];
      pwm_setting[i] = tim_k;
      readmask(main_ptr_mask,k,&tmp) ;	
      readmask(main_ptr_mask,i,&tmp2) ;
      setmask(main_ptr_mask,k,tmp2) ;
      setmask(main_ptr_mask,i,tmp) ;
    }
  }
  
  // Gleiche PWM-Werte vereinigen, ebenso den PWM-Wert 0 löschen falls vorhanden
  
  k=PWM_CHANNELS;             // PWM_CHANNELS Datensätze
  i=1;                        // Startindex
  
  while(k>i) {
    while ( ((pwm_setting[i]==pwm_setting[i+1]) || (pwm_setting[i]==0))  && (k>i) ) {
      
      // aufeinanderfolgende Werte sind gleich und können vereinigt werden
      // oder PWM Wert ist Null
      if (pwm_setting[i]!=0) {
	readmask(main_ptr_mask,i,&tmp) ;
	readmask(main_ptr_mask,i+1,&tmp2) ;
	setmask(main_ptr_mask,i+1,tmp&tmp2) ;
      } ;
      
      // Datensatz entfernen,
      // Nachfolger alle eine Stufe hochschieben
      for(j=i; j<k; j++) {
	pwm_setting[j] = pwm_setting[j+1];
	readmask(main_ptr_mask,j+1,&tmp) ;
	setmask(main_ptr_mask,j,tmp) ;
      }
      k--;
    }
    i++;
  }
  
  // letzten Datensatz extra behandeln
  // Vergleich mit dem Nachfolger nicht möglich, nur löschen
  // gilt nur im Sonderfall, wenn alle Kanäle 0 sind
  if (pwm_setting[i]==0) k--;
  
  // Zeitdifferenzen berechnen
  
  if (k==0) { // Sonderfall, wenn alle Kanäle 0 sind
    main_ptr_time[0]=(uint16_t)T_PWM*PWM_STEPS/2;
    main_ptr_time[1]=(uint16_t)T_PWM*PWM_STEPS/2;
    k=1;
  }
  else {
    i=k;
    main_ptr_time[i]=(uint16_t)T_PWM*(PWM_STEPS-pwm_setting[i]);
    tim_k=pwm_setting[i];
    i--;
    for (; i>0; i--) {
      main_ptr_time[i]=(uint16_t)T_PWM*(tim_k-pwm_setting[i]);
      tim_k=pwm_setting[i];
    }
    main_ptr_time[0]=(uint16_t)T_PWM*tim_k;
  }
  
  // auf Sync warten
  pwm_cnt_max_tmp = k;
  pwm_sync=1;             // Sync wird im Interrupt gesetzt
  pwm_updated = 1 ;
}

// Timer 1 Output COMPARE A Interrupt
volatile uint8_t Timer ;

ISR(TIMER1_COMPA_vect) 
{
  static uint8_t pwm_cnt;
  uint8_t *tmp;
  uint8_t tmp3 ;
  
  OCR1A += isr_ptr_time[pwm_cnt];
  tmp = (uint8_t*)&(isr_ptr_mask[(pwm_cnt<<1)+pwm_cnt]) ;
  
  Timer++ ;
  
  tmp3    = tmp[2];
  if ((tmp3&0x20)!=0) {
    tmp3|=0x80 ;
  } else {
    tmp3 &=0x7f ;
  } ;
  if ((tmp3&0x10)!=0) {
    tmp3|=0x20 ;
  } else {
    tmp3&=0xDf ;
  } ;
  tmp3 &= 0xaf ;
  
  if (pwm_cnt == 0) {
    PWM_DDR1 = tmp[0];                         // Ports setzen zu Begin der PWM
    PWM_DDR2 = tmp[1];
    PWM_DDR3 = (PWM_DDR3&0x5e)|(tmp3&0xA1) ;  
    PWM_DDR4 = (PWM_DDR4&0xF8)|((tmp3>>1)&0x7) ;
    pwm_cnt++;
  }
  else {
    PWM_DDR1 &= tmp[0];                        // Ports löschen
    PWM_DDR2 &= tmp[1];                        // Ports löschen
    PWM_DDR3 &= tmp3|0x5e ;						
    PWM_DDR4 &= (tmp3>>1)|0xf8 ;
    if (pwm_cnt == pwm_cnt_max) {
      if (pwm_sync==1) {
	tausche_zeiger();
	pwm_sync = 0;                       // Update jetzt möglich
      }
      pwm_cnt  = 0;
    }
    else pwm_cnt++;
  }
}

uint8_t GetProgram(uint8_t Channel, uint8_t PStep)
{
  uint8_t *Da ;
  
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
  
  // Alle Kanaele abarbeiten 
  for (Channel=0;Channel<PWM_CHANNELS;Channel++) {
    if (Counter[Channel]>0) {
      Actual[Channel] += Delta[Channel] ;
      Counter[Channel]-- ;
    } else {
      if (Step[Channel]>=20) continue ;
      Command = GetProgram (Channel,Step[Channel]) ;
      Step[Channel]++ ;
      if (Command==0) { // Programm Ende 
	Step[Channel] = 20 ;
      } else if	(Command<201) { // DimTo 
	Counter[Channel] = Command ;
	Command = GetProgram(Channel,Step[Channel]) ;
	Step[Channel]++ ;
	Delta[Channel] = (int16_t)(((((int32_t)Command)<<8)-(int32_t)Actual[Channel])/(int16_t)Counter[Channel]) ;
	if (Delta[Channel]>0) Delta[Channel]++ ; // Rundungsfehler ausgleichen
	Actual[Channel] += Delta[Channel] ; // und ersten Schritt ausfuehren
	Counter[Channel]-- ;
      } else if (Command<221) { // JumpTo 
	Step[Channel] = Command-201 ;
      } else if (Command==221) { // SetTo 
	Actual[Channel] = ((uint16_t)GetProgram(Channel,Step[Channel]))<<8 ;
	Step[Channel]++ ;
      } else if (Command==222) { // Delay 
	Delta[Channel] = 0 ;
	Counter[Channel] = GetProgram(Channel,Step[Channel]) ;
	Step[Channel]++ ;
      } else { // Unknown command 
	Step[Channel] = 20 ; // Auf Ende Setzen 
      } ;
    } ;
  } ;
  if (pwm_sync==0) {
    pwm_update() ;
  } else {
    pwm_updated = 0 ;
  } ;
}



can_t InMessage ;

inline uint32_t BuildCANId (uint8_t Prio, uint8_t Repeat, uint8_t FromLine, uint16_t FromAdd, uint8_t ToLine, uint16_t ToAdd, uint8_t Group)
{
  return (((uint32_t)(Group&0x1))<<1|((uint32_t)ToAdd)<<2|((uint32_t)(ToLine&0xf))<<10|
	  ((uint32_t)FromAdd)<<14|((uint32_t)(FromLine&0xf))<<22|((uint32_t)(Repeat&0x1))<<26|
	  ((uint32_t)(Prio&0x3))<<27) ;
}


inline void GetSourceAddress (uint32_t CANId, uint8_t *FromLine, uint16_t *FromAdd)
{
  *FromLine = (uint8_t)((CANId>>22)&0xf) ;
  *FromAdd = (uint16_t) ((CANId>>14)&0xfff) ;
}

inline uint8_t GetTargetAddress (uint32_t CANId)
{
  return((uint8_t)((CANId>>2)&0xff));
}

uint8_t LastCommand ;

void SetLED (uint8_t Num, uint8_t R, uint8_t G, uint8_t B, uint8_t W)
{
  uint8_t i1,i2;
  
  i2 = 0 ;
  if (Num>99) { i2 = 1 ; Num -=100 ; } ;
  if (Num>7) return ;
  
  if (Num==7) { /* Alle LEDs setzen, rekursiv */
    for (i1=0;i1<7;i1++) SetLED(i1+100,R,G,B,W) ;
    if(pwm_sync==0) {
      pwm_update() ;
    } else {
      pwm_updated=0 ;
    } ;
    return ;
  } ;
  Actual[pgm_read_word(&(LED_TO_R[Num]))] = ((uint16_t)R)<<8 ;
  Actual[pgm_read_word(&(LED_TO_G[Num]))] = ((uint16_t)G)<<8 ;
  Actual[pgm_read_word(&(LED_TO_B[Num]))] = ((uint16_t)B)<<8 ;
  
  if (Num==0) {
    Actual[17] = ((uint16_t)W)<<8 ;
  } ;
  if (i2==0) {
    if (pwm_sync==0) {
      pwm_update() ;
    } else {
      pwm_updated = 0 ;
    } ;
  } ;
}

void hsv_to_rgb (unsigned char h, unsigned char s, unsigned char v,unsigned char *r, unsigned char *g, unsigned char *b)
{
  unsigned char i, f;
  unsigned int p, q, t;
  
  if( s == 0 ) 
    {	*r = *g = *b = v;
    }
  else
    {	i=h/43;
      f=h%43;
      p = (v * (255 - s))/256;
      q = (v * ((10710 - (s * f))/42))/256;
      t = (v * ((10710 - (s * (42 - f)))/42))/256;
      
      switch( i )
	{	case 0:
	    *r = v; *g = t; *b = p; break;
	case 1:
	  *r = q; *g = v; *b = p; break;
	case 2:
	  *r = p; *g = v; *b = t; break;
	case 3:
	  *r = p; *g = q; *b = v; break;			
	case 4:
	  *r = t; *g = p; *b = v; break;				
	case 5:
	  *r = v; *g = p; *b = q; break;
	}
    }
}

void StoreProgram(uint8_t Offset)
{
  uint8_t *Da ;
  Da = (uint8_t*)10 ;
  Da += Offset ;
  Da += InMessage.data[1]*20 ;
  eeprom_write_byte(Da++,InMessage.data[2]) ;
  eeprom_write_byte(Da++,InMessage.data[3]) ;
  if (Offset<18) {
    eeprom_write_byte(Da++,InMessage.data[4]) ;
    eeprom_write_byte(Da++,InMessage.data[5]) ;
    eeprom_write_byte(Da++,InMessage.data[6]) ;
    eeprom_write_byte(Da++,InMessage.data[7]) ;
  } ;
} 

void SetFilter(uint16_t BoardAdd, uint8_t BoardLine)
{
  can_filter_t filter ;
  filter.id = ((uint32_t)BoardAdd)<<2|((uint32_t)BoardLine)<<10 ;
  filter.mask = 0x3FE2 ;
  mcp2515_set_filter(0, &filter) ;
  mcp2515_set_filter(1, &filter) ;
  mcp2515_set_filter(2, &filter) ;
  mcp2515_set_filter(3, &filter) ;
  mcp2515_set_filter(4, &filter) ;
  mcp2515_set_filter(5, &filter) ;
}

uint8_t SetOutMessage (uint16_t BoardAdd, uint8_t BoardLine)
{
  uint8_t SendLine ;
  uint16_t SendAdd ;
  uint8_t LED ;
  
  GetSourceAddress(InMessage.id,&SendLine,&SendAdd) ;
  LED = (GetTargetAddress(InMessage.id)&0x7) ;
  InMessage.id = BuildCANId (0,0,BoardLine,BoardAdd+LED,SendLine,SendAdd,0) ;
  InMessage.data[0] = InMessage.data[0]|SUCCESSFULL_RESPONSE ;
  InMessage.length = 1 ;
  return (LED) ;
}


int main(void) 
{
  // PWM Port einstellen
  uint8_t r,g,b ;
  uint8_t BoardLine ;
  uint16_t BoardAdd ;
  uint8_t LED ;
  
  BoardAdd = 16 ;
  BoardLine = 1 ;
  
  PWM_PORT1 = 0x00;         // Port als Sink
  PWM_PORT2 = 0x00;         // Port als Sink
  PWM_PORT3 &= 0x94;
  PWM_PORT4 &= 0xF8;         // Port als Sink
  
  r = eeprom_read_byte((uint8_t*)0) ;
  if (r==0xba) {
    r = eeprom_read_byte((uint8_t*)1) ;
    if (r==0xca) {
      BoardAdd = eeprom_read_byte((uint8_t*)2) ;
      BoardAdd += ((uint16_t)eeprom_read_byte((uint8_t*)3))<<8 ;
      BoardLine = eeprom_read_byte((uint8_t*)4) ;
    } ;
  } ;
  
  for (r=0;r<22;r++) Step[r] = 20 ; // EndProgram setzen
  
  mcp2515_init();
  
  SetFilter(BoardAdd,BoardLine) ;
  
  
  /* Filter muss hier gesetzt werden */
  
  
  // Timer 1 OCRA1, als variablem Timer nutzen
  TCCR1B = 2;             // Timer läuft mit Prescaler 8
  TCNT1 = 0;
  TIMSK1 |= (1 << OCIE1A);  // Interrupt freischalten
  
  sei();                  // Interrupts gloabl einschalten
  
  while(1) {
    // Warte auf die nächste CAN-Message
    while ((LastCommand=mcp2515_get_message(&InMessage)) == NO_MESSAGE) {
      if ((pwm_sync==0)&&(pwm_updated==0)) pwm_update() ;
      if (Timer>3) {
	Timer = 0 ;
	StepLight() ;
      } ;
    };
    // Sende-Addresse zusammenstöpseln
    r = InMessage.data[0] ;
    LED = SetOutMessage(BoardLine,BoardAdd) ;
    
    //		if ((pwm_sync==0)&&(pwm_updated==0)) pwm_update() ;
    
    switch (r) {
    case SEND_STATUS:
      break ;
    case READ_CONFIG:
      InMessage.data[1] = 0xba ;
      InMessage.data[2] = 0xca ;
      InMessage.data[3] = (uint8_t)(BoardAdd&0xff) ;
      InMessage.data[4] = (uint8_t)(BoardAdd>>8) ;
      InMessage.data[5] = BoardLine ;
      InMessage.length = 6 ;
      mcp2515_send_message(&InMessage) ;
      break ;
    case WRITE_CONFIG:
      if ((InMessage.data[1] == 0xba)&&(InMessage.data[2]==0xca)) {	
	eeprom_write_byte((uint8_t*)2,InMessage.data[3]) ;	
	eeprom_write_byte((uint8_t*)3,InMessage.data[4]) ;	
	eeprom_write_byte((uint8_t*)4,InMessage.data[5]) ;	
      } ;
      break ;
    case SET_VAR:
      if (InMessage.data[1] > 1) {
	eeprom_write_byte((uint8_t*)(uint16_t)InMessage.data[1],InMessage.data[2]) ;
      } ;
      
      break ;
    case START_BOOT:
      wdt_enable(WDTO_250MS) ;
      while(1) ;
      break ;
    case TIME:
      break ;
    case LED_OFF:
      SetLED (LED,0,0,0,0) ;
      mcp2515_send_message(&InMessage) ;
      break ;
    case LED_ON:
      SetLED (LED,255,255,255,255) ;
      mcp2515_send_message(&InMessage) ;
      break ;
    case SET_TO:
      SetLED (LED,InMessage.data[1],InMessage.data[2],InMessage.data[3],InMessage.data[4]) ;
      //				mcp2515_send_message(&InMessage) ;
      break ;
    case HSET_TO:
      hsv_to_rgb(InMessage.data[1],InMessage.data[2],InMessage.data[3],&r,&g,&b) ;
      SetLED (LED,r,g,b,InMessage.data[4]) ;
      mcp2515_send_message(&InMessage) ;
      break ;
    case L_AND_S:
      StoreProgram(0) ;
      break ;
    case SET_TO_G1:
      SetLED (1,InMessage.data[1],InMessage.data[2],InMessage.data[3],0) ;
      SetLED (2,InMessage.data[4],InMessage.data[5],InMessage.data[6],0) ;
      break ;
    case SET_TO_G2:
      SetLED (3,InMessage.data[1],InMessage.data[2],InMessage.data[3],0) ;
      SetLED (4,InMessage.data[4],InMessage.data[5],InMessage.data[6],0) ;
      break ;
    case SET_TO_G3:
      SetLED (5,InMessage.data[1],InMessage.data[2],InMessage.data[3],0) ;
      SetLED (6,InMessage.data[4],InMessage.data[5],InMessage.data[6],0) ;
      break ;
    case LOAD_LOW:
      if (InMessage.data[1]>=PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  InMessage.data[1] = r ;
	  StoreProgram (0) ;
	} ;
      } else {
	StoreProgram(0) ;
      } ;
      break ;
    case LOAD_MID1:
      if (InMessage.data[1]>=PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  InMessage.data[1] = r ;
	  StoreProgram (6) ;
	} ;
      } else {
	StoreProgram(6) ;
      } ;
      break ;
    case LOAD_MID2:
      if (InMessage.data[1]>=PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  InMessage.data[1] = r ;
	  StoreProgram (12) ;
	} ;
      } else {
	StoreProgram(12) ;
      }
      break ;
    case LOAD_HIGH:
      if (InMessage.data[1]>=PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  InMessage.data[1] = r ;
	  StoreProgram (18) ;
	} ;
      } else {
	StoreProgram(18) ;
      } ;
      break ;
    case START_PROG:
      if (InMessage.data[1]>=PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  Step[r] = 0 ;
	  Counter[r] = 0 ;
	} ;
      } else {
	r = InMessage.data[1] ;
	Step[r] = 0 ;
	Counter[r] = 0 ; 
      } ;
      break ;
    case STOP_PROG:
      if (InMessage.data[1]>=PWM_CHANNELS) {
	for (r=0;r<PWM_CHANNELS;r++) {
	  Step[r] = 22 ;
	  Counter[r] = 0 ;
	} ;
      } else {
	r = InMessage.data[1] ;
	Step[r] = 22 ;
	Counter[r] = 0 ; 
      } ;
      break ;
    case CHANNEL_ON:
    case CHANNEL_OFF:
    case CHANNEL_TOGGLE:
    case SHADE_UP_FULL:
    case SHADE_DOWN_FULL:
    case SHADE_UP_SHORT:
    case SHADE_DOWN_SHORT:
    default:
      break ;
    } ;
  } ;
}

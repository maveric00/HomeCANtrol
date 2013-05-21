#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "utils.h"
#include "bootloader_common.h"
#include "mcp2515_defs.h"
#include "mcp2515.h"


// Unterscheidung Tiny / Mega
#if defined(__AVR_ATtiny84__)

uint16_t                            boot_reset RESET_SECTION = RJMP + BOOTLOADER_STARTADDRESS / 2;

static BOOTLOADER_FUNCTIONS bootloader_functions =
{
    (void (*)) NULL
}; 

#endif


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
#define LED0 B,0
#define LED1 B,1

// ----------------------------------------------------------------------------
// globale Variablen

static uint16_t flashpage = 0;
static uint8_t page_buffer_pos = 0;
static uint8_t page_buffer[SPM_PAGESIZE];
uint8_t message_number;
can_t InMessage ;
can_t OutMessage ;

// -----------------------------------------------------------------------------
// Watchdog Timer als erstes im Programm deaktivieren
// siehe http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html

void get_mcusr(void)	__attribute__((naked)) \
						__attribute__((section(".init3")));
void get_mcusr(void)
{
	MCUSR = 0;
	wdt_disable();
}

// ----------------------------------------------------------------------------
// Hauptprogram

void delay_ms(uint16_t __ticks)
{
	while(__ticks)
	{
		// wait 1/10 ms
		_delay_loop_2((F_CPU) / 4e3);
		__ticks --;
	}
}

void main (void) __attribute__((noreturn));


void main(void)
{
  enum { 
    IDLE,
    COLLECT_DATA,
    RECEIVED_PAGE
  } state = IDLE;
  
  uint8_t next_message_number = -1;
  uint8_t BootLine,BoardLine ;
  uint8_t BootAdd,BoardAdd ;
  uint8_t BoardType ;
  uint8_t Counter ;
  uint8_t Temp ;
#if defined(__AVR_ATtiny84__)
  uint16_t reset_vector_boot ;
  uint16_t reset_vector_app ;
  
  reset_vector_boot = pgm_read_word(RESET_ADDR);
  reset_vector_app = 0x0000 ;
#endif

//  PORTD |=1 ;
//  DDRD &=0xfe ;

    
  BoardAdd = 16 ;
  BoardLine = 1 ;
  BootAdd = 1 ;
  BootLine = 0 ;
  BoardType = 0 ;
  
  Temp = eeprom_read_byte((uint8_t*)0) ;
  if (Temp==0xba) {
    Temp = eeprom_read_byte((uint8_t*)1) ;
    if (Temp==0xca) {
      BoardAdd = eeprom_read_byte((uint8_t*)2) ;
      BoardAdd += ((uint16_t)eeprom_read_byte((uint8_t*)3))<<8 ;
      BoardLine = eeprom_read_byte((uint8_t*)4) ;
      BootAdd = eeprom_read_byte((uint8_t*)5) ;
      BootAdd += ((uint16_t)eeprom_read_byte((uint8_t*)6))<<8 ;
      BootLine = eeprom_read_byte((uint8_t*)7) ;
      BoardType = eeprom_read_byte((uint8_t*)8) ;
#if defined (__AVR_AT90PWM3B__)	  
      if (BoardType==0x10) { /* Relais, alle Ausgänge auf 0 setzen 
			   (0: Ausgang von ULN geht auf 1 -> 
			   Low side switch off */
			RESET(CHAN0);
			RESET(CHAN1);
			RESET(CHAN2);
			RESET(CHAN3);
			RESET(CHAN4);
			RESET(CHAN5);
			RESET(CHAN6);
			RESET(CHAN7);
			RESET(CHAN8);
			RESET(CHAN9);
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
      } ;
#endif
    } ;
  } ;
  
  
  mcp2515_register_map[2] = 	((BoardLine) << 2)|((BoardAdd>>6)&0x3) ;
  mcp2515_register_map[3] = 	((BoardAdd&0x3f)<<2) ;
  
  OutMessage.id = ((uint32_t)BootAdd)<<2|((uint32_t)BootLine)<<10|((uint32_t)BoardAdd)<<14|((uint32_t)BoardLine)<<22 ;
  OutMessage.flags.extended = 1 ;
  OutMessage.length = 0 ;
  
  mcp2515_init();
  
  delay_ms(2000+BoardAdd*10);
  
  Counter = 0 ;
  
  
  while (1)
    {
      uint8_t command;
      uint16_t page;
      
      
      // wait until we receive a new message
      while (mcp2515_get_message(&InMessage) == NO_MESSAGE)
	{
	  if (Counter<10) {
	    OutMessage.data[0] = UPDATE_REQ ;
	    OutMessage.data[1] = BoardType;
	    OutMessage.data[2] = Counter ;
	    OutMessage.data[3] = PAGESIZE_IDENTIFIER;
	    
	    // number of writeable pages
	    OutMessage.data[4] = HIGH_BYTE(RWW_PAGES);
	    OutMessage.data[5] = LOW_BYTE(RWW_PAGES);
	    OutMessage.length = 6 ;
	    mcp2515_send_message (&OutMessage) ;
	    delay_ms(200) ;
	    if (Counter++>5) {
	      // 1 sekunde gewartet, kein Update vorhanden, Applikation anspringen
	      boot_jump_to_application () ;
	    } ;
	  } ;
	} ;
      
      /* Hier muss ueberprueft werden, ob die Nachricht ueberhaupt fuer uns war... */
      Counter = 20 ;
      command = InMessage.data[0] ;
      message_number = InMessage.data[1] ;
      
      // check if the message is a request, otherwise reject it
      if ((command & ~COMMAND_MASK) != REQUEST)
	continue;

		if (command>5) {Counter = 0 ; continue ; } ;
      
      command &= COMMAND_MASK;
      
      // check message number
      next_message_number++;
      if (message_number != next_message_number)
	{
	  // wrong message number => send NACK
	  message_number = next_message_number;
	  next_message_number--;
	  OutMessage.data[0] = WRONG_NUMBER_REPSONSE ;
	  OutMessage.data[1] = next_message_number+1 ;
	  OutMessage.length = 2 ;
	  mcp2515_send_message(&OutMessage) ;
	  continue;
	}
      
      OutMessage.data[1] = next_message_number+1 ;
      
      // process command
      switch (command)
	{
	case IDENTIFY:
	  // version and command of the bootloader
	  OutMessage.data[0] = IDENTIFY|SUCCESSFULL_RESPONSE ;
	  OutMessage.data[2] = BoardType;
	  OutMessage.data[3] = PAGESIZE_IDENTIFIER;
	  
	  // number of writeable pages
	  OutMessage.data[4] = HIGH_BYTE(RWW_PAGES);
	  OutMessage.data[5] = LOW_BYTE(RWW_PAGES);
	  OutMessage.length = 6 ;
	  mcp2515_send_message (&OutMessage) ;
	  break;
	  
	  // --------------------------------------------------------------------
	  // set the current address in the page buffer
	  
	case SET_ADDRESS:
	  page = (InMessage.data[2] << 8) | InMessage.data[3];
	  
	  if (InMessage.length == 6 && 
	      InMessage.data[5] < (SPM_PAGESIZE/4) &&
	      page < RWW_PAGES)
	    {
	      flashpage = page;
	      page_buffer_pos = InMessage.data[4];
	      
	      state = COLLECT_DATA;
	      OutMessage.data[0] = SET_ADDRESS|SUCCESSFULL_RESPONSE ;
	      OutMessage.length = 2 ;
	      mcp2515_send_message(&OutMessage) ;
	    }
	  else {
	    goto error_response;
	  }
	  break;
	  
	  // --------------------------------------------------------------------
	  // collect data
	  
	case DATA:
	  if (InMessage.length != 6 || page_buffer_pos >= (SPM_PAGESIZE / 4) || state == IDLE) {
	    state = IDLE;
	    goto error_response;
	  }
	  
	  // copy data
#if defined(__AVR_ATtiny84__)
	  if ((flashpage==0)&&(page_buffer_pos==0)) { // Reset-Vektor auf Bootloader setzen
            memcpy( &(reset_vector_app), &(InMessage.data[2]), 2 );
            memcpy( page_buffer,&(reset_vector_boot),2);
            memcpy( page_buffer + 2, &(InMessage.data[4]), 2 );
            bootloader_functions.start_appl_main = (void*)(reset_vector_app-RJMP) ;
	  } else {
            memcpy( page_buffer + page_buffer_pos * 4, &(InMessage.data[2]), 4 );
	  } ;
#else
          memcpy( page_buffer + page_buffer_pos * 4, &(InMessage.data[2]), 4 );
#endif
	  page_buffer_pos++;
	  OutMessage.data[4] = 0 ;
	  
	  if (page_buffer_pos == (SPM_PAGESIZE / 4))
	    {
	      
	      if (flashpage >= RWW_PAGES) {
			goto error_response;
	      }
	      
	      boot_program_page( flashpage*SPM_PAGESIZE, (uint16_t*) page_buffer,SPM_PAGESIZE );
	      page_buffer_pos = 0;
	      flashpage += 1;
	      OutMessage.data[4] = 1 ;
	    }
	  OutMessage.data[0] =  DATA | SUCCESSFULL_RESPONSE ;
	  OutMessage.data[2] = flashpage >> 8;
	  OutMessage.data[3] = flashpage & 0xff;
	  OutMessage.length = 5 ;
	  mcp2515_send_message(&OutMessage) ;
	  
	  break;
	  
	  // --------------------------------------------------------------------
	  // start the flashed application program
	  
	case START_APP:
	  OutMessage.data[0] =  START_APP | SUCCESSFULL_RESPONSE ;
	  OutMessage.length = 2 ;
	  mcp2515_send_message(&OutMessage) ;
	  // wait for the mcp2515 to send the message
	  delay_ms(100);
#if defined(__AVR_ATtiny84__)
	  // Startaddresse der Applikation speichern, wenn geändert
          if(reset_vector_app!=0x0000) 
	          boot_program_page (BOOTLOADER_FUNC_ADDRESS, (uint16_t *) &bootloader_functions, sizeof (bootloader_functions));
#endif	  
	  // start application
	  boot_jump_to_application();
	  break;
	  
	  
	error_response:
	default:
	  OutMessage.data[0] = command|ERROR_RESPONSE ;
	  OutMessage.length = 2 ;
	  mcp2515_send_message(&OutMessage) ;
	  break;
	} ;
    } ;
}

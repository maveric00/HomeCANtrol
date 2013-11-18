#include "mcp2515.h"
#include "utils.h"

#include "spi.h"

// ----------------------------------------------------------------------------
void mcp2515_spi_init(void)
{
	// Aktivieren der Pins fuer das SPI Interface
	SET(MCP2515_CS);
	SET_OUTPUT(MCP2515_CS);
	
	// Aktivieren der Pins fuer das SPI Interface
	RESET(P_SCK);
	RESET(P_MOSI);
	RESET(P_MISO);
	SET_OUTPUT(P_SCK);
	SET_OUTPUT(P_MOSI);
	SET_INPUT(P_MISO);

#if defined (RELAISBOARD)
	// HW-SPI aktivieren
	MCUCR |= 1<<SPIPS ;
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR = (1<<SPI2X);
#endif
}

// ----------------------------------------------------------------------------
// Schreibt/liest ein Byte ueber den Hardware SPI Bus

uint8_t spi_putc(uint8_t data)
{
#if defined (RELAISBOARD)
  SPDR = data;
  
  // Wartet bis Byte gesendet wurde
  while( !( SPSR & (1<<SPIF) ) );
  
  return SPDR;
#else
  // USI can not be used as ATTiny, as DO is matched to MISO,
  // not MOSI - would need different PCB layout
  // SW SPI on ATTiny, ist auch nicht besonders viel langsamer
  // als die HW-Unterstuetze Variante
  uint8_t data_in = 0;
  
  /* Schnelle Version:
  ldi r25,#0 ; Output vorbereiten
  in r15,PORTA
  andi r15,~P_SCK
  mov r16,r15
  ori r15,P_MOSI
  out PORTA,r16

  sbrc r25,7      ;2  low
  out PORTA,r15   ;1  low
  sbi PORTA,P_SCK ;2  low->high
  sbic P_MISO     ;2  high
  sec             ;1  high
  rol r24         ;1  high
  out PORTA,r16   ;1  high->low

  sbrc r25,6      ;2  low
  out PORTA,r15   ;1  low
  sbi PORTA,P_SCK ;2  low->high
  sbic P_MISO     ;2  high
  sec             ;1  high
  rol r24         ;1  high
  out PORTA,r16   ;1  high->low
  */

  RESET(P_SCK);
  for (uint8_t i=0;i<8;i++)
    {
      data_in <<= 1;
      
      if (data & 0x80)
	SET(P_MOSI);
      else
	RESET(P_MOSI);
      SET(P_SCK);
      if (IS_SET(P_MISO))
	data_in |= 1;
      data <<= 1;
      RESET(P_SCK);
    }
  return data_in;
#endif
}

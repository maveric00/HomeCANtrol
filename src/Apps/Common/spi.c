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
#elif defined (__AVR_ATtiny84__)

  // USI can not be used as ATTiny, as DO is matched to MISO,
  // not MOSI - would need different PCB layout
  // SW SPI on ATTiny, ist auch nicht besonders viel langsamer
  // als die HW-Unterstuetze Variante
  
  asm volatile (
  "ldi r25,0x00 ; Initialize result\n\t"
  "in r18,0x1b ; read port setting\n\t"
  "andi r18,0xef; mask P_SCK\n\t"
  "mov r19,r18 ; copy \n\t"
  "ori r18,0x40 ; or P_MOSI\n\t"

  "out 0x1b,r19 ; P_SCK to low\n\t"

  "sbrc r24,7      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,6      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,5      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,4      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,3      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,2      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,1      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"

  "sbrc r24,0      ; check bit 7 2  low\n\t"
  "out 0x1b,r18   ; out P_MOSI1  low\n\t"
  "sbi 0x1b,4     ; set HIGH 2  low->high\n\t"
  "sbic 0x19,5    ; check P_MISO 2  high\n\t"
  "sec            ; if set, set carry1  high\n\t"
  "rol r25        ; rotate left with carry 1  high\n\t"
  "out 0x1b,r19   ; P_SCK to low (without MOSI) 1  high->low\n\t"
  "mov r24,r25    ; copy to output \n\t"
  ::) ;

#else
  uint8_t data_in = 0;

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

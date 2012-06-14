/* bootloader_common.h: Beinhaltet die Routinen zum Schreiben des Flash und zum Anspringen der Applikation */

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "bootloader_common.h"


// ----------------------------------------------------------------------------

#if defined(__AVR_AT90PWM3B__)
	#define	IV_REG	MCUCR
#endif

// ----------------------------------------------------------------------------
// starts the application program

#if defined(__AVR_ATtiny84__)

BOOTLOADER_FUNCTIONS bootloader_functions;

void boot_jump_to_application(void)
{
	// Startaddressen kopieren
	memcpy_P (&bootloader_functions, (PGM_P) BOOTLOADER_FUNC_ADDRESS, sizeof (bootloader_functions));
	if (bootloader_functions.start_appl_main==(void*)0xffff) bootloader_functions.start_appl_main=NULL ;
	// disable interrupts
	cli();
	(*bootloader_functions.start_appl_main) ();
}
#else
void boot_jump_to_application(void)
{
	// disable interrupts
	cli();
	
	// relocate interrupt vectors
	uint8_t reg = IV_REG & ~((1 << IVCE) | (1 << IVSEL));
	
	IV_REG = reg | (1 << IVCE);
	IV_REG = reg;
	
	// jump to address 0x0000
	#if FLASHEND > 0xffff
	asm volatile (	"push r1" "\n\t"
					"push r1" "\n\t"
					"push r1" "\n\t"
					::);
	#else
	asm volatile (	"push r1" "\n\t"
					"push r1" "\n\t"
					::);
	#endif
}
#endif

#define boot_program_page_fill(byteaddr, word)      \
{                                                   \
    sreg = SREG;                                    \
    cli ();                                         \
    boot_page_fill ((uint32_t) (byteaddr), word);   \
    SREG = sreg;                                    \
}
 
/*-----------------------------------------------------------------------------------------------------------------------
 * Flash: erase and write page
 *-----------------------------------------------------------------------------------------------------------------------
 */
#if defined (__AVR_AT90PWM3B__)						
#define boot_program_page_erase_write(pageaddr)     \
{                                                   \
    eeprom_busy_wait ();                            \
    sreg = SREG;                                    \
    cli ();                                         \
    boot_page_erase ((uint32_t) (pageaddr));        \
    boot_spm_busy_wait ();                          \
    boot_page_write ((uint32_t) (pageaddr));        \
    boot_spm_busy_wait ();                          \
    boot_rww_enable ();                             \
    SREG = sreg;                                    \
} 
#else
#define boot_program_page_erase_write(pageaddr)     \
{                                                   \
    eeprom_busy_wait ();                            \
    sreg = SREG;                                    \
    cli ();                                         \
    boot_page_erase ((uint32_t) (pageaddr));        \
    boot_spm_busy_wait ();                          \
    boot_page_write ((uint32_t) (pageaddr));        \
    boot_spm_busy_wait ();                          \
    SREG = sreg;                                    \
} 
#endif												



// ----------------------------------------------------------------------------
// write a complete page to the flash memory

void boot_program_page(uint16_t flash_addr, uint16_t *block,size_t size)
{
    uint16_t        start_addr;
    uint16_t        addr;
    uint16_t        w;
    uint8_t         idx = 0;
    uint8_t         sreg;
 
    start_addr = (flash_addr / SPM_PAGESIZE) * SPM_PAGESIZE;        // round down (granularity is SPM_PAGESIZE)
 
    for (idx = 0; idx < SPM_PAGESIZE / 2; idx++)
    {
        addr = start_addr + 2 * idx;
 
        if (addr >= flash_addr && size > 0)
        {
            w = *block++;
            size -= sizeof (uint16_t);
        }
        else
        {
            w = pgm_read_word (addr);
        }
 
        boot_program_page_fill (addr, w);
    }
    boot_program_page_erase_write(start_addr);                      // erase and write the page
}

/* bootloader_common.h: Beinhaltet die Routinen zum Schreiben des Flash und zum Anspringen der Applikation */

#ifndef	BOOTLOADER_COMMON_H
#define	BOOTLOADER_COMMON_H

#include <inttypes.h>

extern void boot_jump_to_application(void);
extern void boot_program_page(uint16_t flash_addr, uint16_t *buf,size_t size);

#endif	// BOOTLOADER_COMMON_H

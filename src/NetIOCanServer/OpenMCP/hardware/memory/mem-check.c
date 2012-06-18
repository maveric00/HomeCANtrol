/*! \file mem-check.c \brief Zeigt und Prüft die Speicherauslastung */
//***************************************************************************
//*            mem-check.c
//****************************************************************************/
///	\ingroup hardware
///	\defgroup memcheck Prüft das interne RAM (mem-check.c)
///	\code #include "mem-check.h" \endcode
//****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
//@{

#include <avr/io.h> // RAMEND
#include "mem-check.h"

// Mask to init SRAM and check against
#define MASK 0xaa

// From linker script
extern unsigned char __heap_start;

unsigned short 
get_mem_unused (void)
{
	unsigned short unused = 0;
	unsigned char *p = &__heap_start;
	
	do
	{
		if (*p++ != MASK)
			break;
			
		unused++;
	} while (p <= (unsigned char*) __heap_end);

	return unused;
}

/* !!! never call this function !!! */
void __attribute__ ((naked, section (".init8")))
__init8_mem (void)
{	
	__asm volatile (
		"ldi r30, lo8 (__heap_start)"  "\n\t"
		"ldi r31, hi8 (__heap_start)"  "\n\t"
		"ldi r24, %0"                  "\n\t"
		"ldi r25, hi8 (%1)"            "\n"
		"0:"                           "\n\t"
		"st  Z+,  r24"                 "\n\t"
		"cpi r30, lo8 (%1)"            "\n\t"
		"cpc r31, r25"                 "\n\t"
		"brlo 0b"
		:
		: "i" (MASK), "i" (__heap_end+1)
	);
}

//@}

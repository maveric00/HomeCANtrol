// ----------------------------------------------------------------------------
/*
 * Copyright (c) 2007 Fabian Greif, Roboterclub Aachen e.V.
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: mcp2515_write_id.c 6568 2008-06-16 13:56:26Z fabian $
 */
// ----------------------------------------------------------------------------

#include "mcp2515_private.h"

// ----------------------------------------------------------------------------

/*static uint8_t usi_interface_spi_temp[2];

static void spi_start(uint8_t IF, uint8_t data) {
	usi_interface_spi_temp[IF] = spi_putc(IF, data);
}

static uint8_t spi_wait(uint8_t IF) {
	return usi_interface_spi_temp[IF];
}
*/

// ----------------------------------------------------------------------------
/* Schreibt eine CAN ID in die Register des MCP2515
 *
 * Die Funktion setzt eine offene Verbindung zum MCP2515 vorraus
 * und schreibt dann die CAN ID per SPI in die folgenden vier
 * Register des MCP2515.
 *
 * ACHTUNG: die Funktion wurde "optimiert", damit nicht ständig unnötige
 * 			32-Bit Operationen verwendet werden :)
 *
 * Funktionell aequivalent zu:
 *
 *	static void mcp2515_write_id(uint32_t *id, uint8_t extended)
 *	{
 *		if (extended) {
 *			spi_putc(*id >> 21);
 *			spi_putc(((*id >> 13) & 0xe0) | (1<<IDE) | ((*id >> 16) & 0x3));
 *			spi_putc(*id >> 8);
 *			spi_putc(*id);
 *		}
 *		else {
 *			spi_putc(*id >> 3);
 *			spi_putc(*id << 5);
 *			spi_putc(0);
 *			spi_putc(0);
 *		}
 *	}
 */


void mcp2515_write_id(uint8_t IF, const uint32_t *id, uint8_t extended)
{
	uint8_t tmp;
	
	if (extended) {
		spi_start(IF,*((uint16_t *) id + 1) >> 5);
		
		// naechsten Werte berechnen
		tmp  = (*((uint8_t *) id + 2) << 3) & 0xe0;
		tmp |= (1 << IDE);
		tmp |= (*((uint8_t *) id + 2)) & 0x03;
		
		// warten bis der vorherige Werte geschrieben wurde
		spi_wait(IF);
		
		// restliche Werte schreiben
		spi_putc(IF,tmp);
		spi_putc(IF,*((uint8_t *) id + 1));
		spi_putc(IF,*((uint8_t *) id));
	}
	else {
		spi_start(IF,*((uint16_t *) id) >> 3);
		
		// naechsten Werte berechnen
		tmp = *((uint8_t *) id) << 5;
		spi_wait(IF);
		
		spi_putc(IF,tmp);
		spi_putc(IF,0);
		spi_putc(IF,0);
	}
}


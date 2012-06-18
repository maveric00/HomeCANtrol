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
 * $Id: mcp2515_read_id.c 6653 2008-09-02 13:51:25Z fabian $
 */
// ----------------------------------------------------------------------------

#include "mcp2515_private.h"

// ----------------------------------------------------------------------------
// Liest eine ID aus dem Registern des MCP2515 (siehe auch mcp2515_write_id())


uint8_t mcp2515_read_id(uint8_t IF, uint32_t *id)
{
	uint8_t first;
	uint8_t tmp;
	
	first = spi_putc(IF,0xff);
	tmp   = spi_putc(IF,0xff);
	
	if (tmp & (1 << IDE)) {
		spi_start(IF,0xff);
		
		*((uint16_t *) id + 1)  = (uint16_t) first << 5;
		*((uint8_t *)  id + 1)  = spi_wait(IF);
		spi_start(IF,0xff);
		
		*((uint8_t *)  id + 2) |= (tmp >> 3) & 0x1C;
		*((uint8_t *)  id + 2) |=  tmp & 0x03;
		
		*((uint8_t *)  id)      = spi_wait(IF);
		
		return TRUE;
	}
	else {
		spi_start(IF,0xff);
		
		*((uint8_t *)  id + 3) = 0;
		*((uint8_t *)  id + 2) = 0;
		
		*((uint16_t *) id) = (uint16_t) first << 3;
		
		spi_wait(IF);
		spi_start(IF,0xff);
		
		*((uint8_t *) id) |= tmp >> 5;
		
		spi_wait(IF);
		
		return FALSE;
	}
}


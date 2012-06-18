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
 * $Id: mcp2515_set_dyn_filter.c 6837 2008-11-16 19:05:15Z fabian $
 */
// ----------------------------------------------------------------------------

#include "mcp2515_private.h"

#include <util/delay.h>

// ----------------------------------------------------------------------------
// set a filter

bool mcp2515_set_filter(uint8_t IF, uint8_t number, const can_filter_t *filter)
{
	uint8_t mask_address = 0;
	uint8_t mode = mcp2515_read_register(IF,CANSTAT);
	
	if (number > 5)
		return false;
	
	// change to configuration mode
	mcp2515_change_operation_mode(IF, (1<<REQOP2) );
	
	// set filter mask
	if (number == 0)
	{
		mask_address = RXM0SIDH;
		
		if (filter->flags.extended == 0x3) {
			// only extended identifier
			mcp2515_write_register(IF, RXB0CTRL, (1<<RXM1));
		}
		else if (filter->flags.extended == 0x2) {
			// only standard identifier
			mcp2515_write_register(IF, RXB0CTRL, (1<<RXM0));
		}
		else {
			// receive all messages
			mcp2515_write_register(IF, RXB0CTRL, 0);
		}
	}
	else if (number == 2)
	{
		mask_address = RXM1SIDH;
		
		if (filter->flags.extended == 0x3) {
			// only extended identifier
			mcp2515_write_register(IF, RXB1CTRL, (1<<RXM1));
		}
		else if (filter->flags.extended == 0x2) {
			// only standard identifier
			mcp2515_write_register(IF, RXB1CTRL, (1<<RXM0));
		}
		else {
			mcp2515_write_register(IF, RXB1CTRL, 0);
		}
	}
	
	if (mask_address)
	{
		if (IF==0) {
			RESET(MCP2515_CS_1);
		} else {
			RESET(MCP2515_CS_2);
		} ;
		spi_putc(IF,SPI_WRITE);
		spi_putc(IF,mask_address);
		mcp2515_write_id(IF,&filter->mask, (filter->flags.extended == 0x2) ? 0 : 1);
		if (IF==0) {
			SET(MCP2515_CS_1);
		} else {
			SET(MCP2515_CS_2);
		} ;
		
		_delay_us(1);
	}
	
	// write filter
	uint8_t filter_address;
	if (number >= 3) {
		number -= 3;
		filter_address = RXF3SIDH;
	}
	else {
		filter_address = RXF0SIDH;
	}
	
	if (IF==0) {
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	spi_putc(IF,SPI_WRITE);
	spi_putc(IF,filter_address | (number * 4));
	mcp2515_write_id(IF,&filter->id, (filter->flags.extended == 0x2) ? 0 : 1);
	if (IF==0) {
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
	
	_delay_us(1);
	
	// restore previous mode
	mcp2515_change_operation_mode( IF, mode );
	
	return true;
}


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
 * $Id: mcp2515_get_dyn_filter.c 6837 2008-11-16 19:05:15Z fabian $
 */
// ----------------------------------------------------------------------------

#include "mcp2515_private.h"

// ----------------------------------------------------------------------------
// get a filter

uint8_t mcp2515_get_filter(uint8_t IF, uint8_t number, can_filter_t *filter)
{
	uint8_t mask_address;
	uint8_t filter_address;
	uint8_t temp;
	uint8_t mode = mcp2515_read_register(IF,CANSTAT);
	
	if (number > 5)
		return 0;
	
	// change to configuration mode
	mcp2515_change_operation_mode(IF, (1<<REQOP2) );
	
	if (number <= 1)
	{
		mask_address = RXM0SIDH;
		temp = mcp2515_read_register(IF,RXB0CTRL);
	}
	else
	{
		mask_address = RXM1SIDH;
		temp = mcp2515_read_register(IF,RXB1CTRL);
	}
	
	temp &= (1<<RXM1)|(1<<RXM0);
	
	if (temp == 0) {
		// filter and masks are disabled
		filter->flags.extended = 0;
		filter->flags.rtr = 0;
		filter->mask = 0;
		filter->id = 0;
		
		return 1;
	}
	
	// transform bits so that they match the format from can.h
	temp >>= 5;
	temp = ~temp;
	if (temp & 1) temp = 0x3;
	
	filter->flags.extended = temp;
	
	// read mask
	if (IF==0) {
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	spi_putc(IF,SPI_READ);
	spi_putc(IF,mask_address);
	mcp2515_read_id(IF,&filter->mask);
	if (IF==0) {
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
	
	if (number <= 2)
	{
		filter_address = RXF0SIDH + number * 4;
	}
	else
	{
		filter_address = RXF3SIDH + (number - 3) * 4;
	}
	
	// read filter
	if (IF==0) {
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	spi_putc(IF,SPI_READ);
	spi_putc(IF,filter_address);
	mcp2515_read_id(IF,&filter->id);
	if (IF==0) {
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
	
	// restore previous mode
	mcp2515_change_operation_mode(IF, mode );
	
	return 1;
}


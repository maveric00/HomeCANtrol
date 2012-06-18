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
 * $Id: mcp2515_get_message.c 6837 2008-11-16 19:05:15Z fabian $
 */
// ----------------------------------------------------------------------------

#include "mcp2515_private.h"

// ----------------------------------------------------------------------------

uint8_t mcp2515_get_message(uint8_t IF, can_t *msg)
{
	uint8_t addr;
	
	// read status
	uint8_t status = mcp2515_read_status(IF,SPI_RX_STATUS);
		
	if (_bit_is_set(status,6)) {
		// message in buffer 0
		addr = SPI_READ_RX;
	}
	else if (_bit_is_set(status,7)) {
	// message in buffer 1
		addr = SPI_READ_RX | 0x04;
	}
	else {
		// Error: no message available
		return 0;
	}
	
	if (IF==0) {
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	spi_putc(IF,addr);
	
	// CAN ID auslesen und ueberpruefen
	uint8_t tmp = mcp2515_read_id(IF,&msg->id);
	msg->flags.extended = tmp & 0x01;
	
	// read DLC
	uint8_t length = spi_putc(IF,0xff);
	msg->flags.rtr = (_bit_is_set(status, 3)) ? 1 : 0;
	
	length &= 0x0f;
	msg->length = length;
	// read data
	for (uint8_t i=0;i<length;i++) {
		msg->data[i] = spi_putc(IF,0xff);
	}
	if (IF==0) {
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
	
	// clear interrupt flag
	if (_bit_is_set(status, 6))
		mcp2515_bit_modify(IF,CANINTF, (1<<RX0IF), 0);
	else
		mcp2515_bit_modify(IF,CANINTF, (1<<RX1IF), 0);
	
	CAN_INDICATE_RX_TRAFFIC_FUNCTION;
	
	return (status & 0x07) + 1;
}


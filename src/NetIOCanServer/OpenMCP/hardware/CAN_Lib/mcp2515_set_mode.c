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
 * $Id: mcp2515_set_mode.c 6837 2008-11-16 19:05:15Z fabian $
 */
// ----------------------------------------------------------------------------

#include "mcp2515_private.h"

// ----------------------------------------------------------------------------
void mcp2515_set_mode(uint8_t IF, can_mode_t mode)
{
	uint8_t reg = 0;
	
	if (mode == LISTEN_ONLY_MODE) {
		reg = (1<<REQOP1)|(1<<REQOP0);
	}
	else if (mode == LOOPBACK_MODE) {
		reg = (1<<REQOP1);
	}
		
	// set the new mode
	mcp2515_bit_modify(IF, CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), reg);
	while ((mcp2515_read_register(IF, CANSTAT) & 0xe0) != reg) {
		// wait for the new mode to become active
	}
}


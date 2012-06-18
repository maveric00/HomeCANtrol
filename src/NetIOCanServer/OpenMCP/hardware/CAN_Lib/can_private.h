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
 * $Id: can_private.h 6910 2008-11-30 21:13:14Z fabian $
 */
// ----------------------------------------------------------------------------

#ifndef	CAN_PRIVATE_H
#define	CAN_PRIVATE_H

#include "config.h"

#ifndef	CAN_FORCE_TX_ORDER
	#define	CAN_FORCE_TX_ORDER		0
#endif
	
// settings for buffered operation (only possible for the AT90CAN...)
#if !defined(CAN_TX_BUFFER_SIZE) || CAN_TX_BUFFER_SIZE == 0
	#define	CAN_TX_BUFFER_SIZE		0
	
	// forced order is only possible with buffered transmission
	#undef	CAN_FORCE_TX_ORDER
	#define	CAN_FORCE_TX_ORDER		0
#endif

#ifndef	CAN_RX_BUFFER_SIZE
	#define	CAN_RX_BUFFER_SIZE		0
#endif


#define mcp2515_init(...)					can_init(__VA_ARGS__)
#define mcp2515_check_free_buffer(...)		can_check_free_buffer(__VA_ARGS__)
#define mcp2515_check_message(...)			can_check_message(__VA_ARGS__)
#define mcp2515_get_filter(...)				can_get_filter(__VA_ARGS__)
#define mcp2515_static_filter(...)			can_static_filter(__VA_ARGS__)
#define mcp2515_set_filter(...)				can_set_filter(__VA_ARGS__)
#define mcp2515_get_message(...)			can_get_message(__VA_ARGS__)
#define mcp2515_send_message(...)			can_send_message(__VA_ARGS__)
#define	mcp2515_read_error_register(...)	can_read_error_register(__VA_ARGS__)
#define	mcp2515_set_mode(...)				can_set_mode(__VA_ARGS__)


#ifndef	CAN_INDICATE_TX_TRAFFIC_FUNCTION
	#define	CAN_INDICATE_TX_TRAFFIC_FUNCTION
#endif

#ifndef	CAN_INDICATE_RX_TRAFFIC_FUNCTION
	#define	CAN_INDICATE_RX_TRAFFIC_FUNCTION
#endif

#ifdef	CAN_DEBUG_LEVEL
	#include <avr/pgmspace.h>
	#include <stdio.h>
	
	#define	DEBUG_INFO(format, ...)		printf_P(PSTR(format), ##__VA_ARGS__)
#else
	#define	DEBUG_INFO(format, ...)
#endif


#endif	// CAN_PRIVATE_H

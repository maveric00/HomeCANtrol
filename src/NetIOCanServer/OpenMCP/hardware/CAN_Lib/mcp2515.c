// coding: utf-8
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
 * $Id: mcp2515.c 6927 2008-12-03 22:42:59Z fabian $
 */
// ----------------------------------------------------------------------------
/* ---- Beispiel zum Einstellen des Bit Timings ----
 *	
 *	Fosc		= 16MHz
 *	BRP			= 7
 *	TQ 			= 2 * (BRP + 1) / Fosc
 *				= 1 uS
 *
 *	Sync Seg	= 					= 1 TQ
 *	Prop Seg	= (PRSEG + 1) * TQ	= 1 TQ
 *	Phase Seg1	= (PHSEG1 + 1) * TQ	= 3 TQ
 *	Phase Seg2	= (PHSEG2 + 1) * TQ = 3 TQ
 *									--------
 *									  8 TQ
 *	
 *	Bus speed	= 1 / ((Total # of TQ) * TQ)
 *				= 1 / (8 * TQ) = 125 kHz
 */
// -------------------------------------------------------------------------

#include "mcp2515_private.h"
#include "hardware/pcint/pcint.h"
#include "system/net/udp.h"


#ifndef	MCP2515_CLKOUT_PRESCALER
	#error	MCP2515_CLKOUT_PRESCALER not defined!
#elif MCP2515_CLKOUT_PRESCALER == 0
	#define	CLKOUT_PRESCALER_	0x0
#elif MCP2515_CLKOUT_PRESCALER == 1
	#define	CLKOUT_PRESCALER_	0x4
#elif MCP2515_CLKOUT_PRESCALER == 2
	#define	CLKOUT_PRESCALER_	0x5
#elif MCP2515_CLKOUT_PRESCALER == 4
	#define	CLKOUT_PRESCALER_	0x6
#elif MCP2515_CLKOUT_PRESCALER == 8
	#define	CLKOUT_PRESCALER_	0x7
#else
	#error	invaild value of MCP2515_CLKOUT_PRESCALER
#endif

prog_uint8_t can_filter[] = 
{
	// Group 0
	MCP2515_FILTER_EXTENDED(0x00001000),	// Filter 0
	MCP2515_FILTER_EXTENDED(0),				// Filter 1
	
	// Group 1
	MCP2515_FILTER_EXTENDED(0),		// Filter 2
	MCP2515_FILTER_EXTENDED(0),		// Filter 3
	MCP2515_FILTER_EXTENDED(0),		// Filter 4
	MCP2515_FILTER_EXTENDED(0),		// Filter 5
	
//	MCP2515_FILTER_EXTENDED(0xfff8),// Mask 0 (for group 0)
	MCP2515_FILTER_EXTENDED(0x0),// Mask 0 (for group 0)
	MCP2515_FILTER_EXTENDED(0x1fffffff),		// Mask 1 (for group 1)
};


// -------------------------------------------------------------------------
void mcp2515_write_register( uint8_t IF, uint8_t adress, uint8_t data )
{
	if (IF==0) {	
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	spi_putc(IF,SPI_WRITE);
	spi_putc(IF, adress);
	spi_putc(IF, data);
	
	if (IF==0) {	
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
}

// -------------------------------------------------------------------------
uint8_t mcp2515_read_register(uint8_t IF, uint8_t adress)
{
	uint8_t data;
	
	if (IF==0) {	
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	
	spi_putc(IF,SPI_READ);
	spi_putc(IF,adress);
	
	data = spi_putc(IF,0xff);	
	
	if (IF==0) {	
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
	
	return data;
}

// -------------------------------------------------------------------------
void mcp2515_bit_modify(uint8_t IF, uint8_t adress, uint8_t mask, uint8_t data)
{
	if (IF==0) {	
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	
	spi_putc(IF,SPI_BIT_MODIFY);
	spi_putc(IF,adress);
	spi_putc(IF,mask);
	spi_putc(IF,data);
	
	if (IF==0) {	
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
}

// ----------------------------------------------------------------------------
uint8_t mcp2515_read_status(uint8_t IF, uint8_t type)
{
	uint8_t data;
	
	if (IF==0) {	
		RESET(MCP2515_CS_1);
	} else {
		RESET(MCP2515_CS_2);
	} ;
	
	spi_putc(IF,type);
	data = spi_putc(IF,0xff);
	
	if (IF==0) {	
		SET(MCP2515_CS_1);
	} else {
		SET(MCP2515_CS_2);
	} ;
	
	return data;
}

// -------------------------------------------------------------------------

prog_uint8_t _mcp2515_cnf[8][3] = {
	// 10 kbps
	{	0x04,
		0xb6,
		0xe7
	},
	// 20 kbps
	{	0x04,
		0xb6,
		0xd3
	},
	// 50 kbps
	{	0x04,
		0xb6,
		0xc7
	},
	// 100 kbps
	{	0x04,
		0xb6,
		0xc3
	},
	// 125 kbps
	{	(1<<PHSEG21),					// CNF3
		(1<<BTLMODE)|(1<<PHSEG11),		// CNF2
		(1<<BRP2)|(1<<BRP1)|(1<<BRP0)	// CNF1
	},
	// 250 kbps
	{	0x03,
		0xac,
		0x81
	},
	// 500 kbps
	{	0x03,
		0xac,
		0x80
	},
	// 1 Mbps
	{	(1<<PHSEG21),
		(1<<BTLMODE)|(1<<PHSEG11),
		0
	}
};

can_t RXMessage0;
can_t RXMessage1 ;
volatile uint8_t CanRX0 = 0 ;
volatile uint8_t CanRX1 = 0 ;


extern int CAN_Socket ;
extern char *CAN_SendBuffer ;

void Can_Int (void) 
{
	if (!IS_SET(MCP2515_INT_1)) {
		// Pin is low on bus 0 //

		
		if (can_check_message(0)) {
			/* Message is here */
			can_get_message(0,&RXMessage0);
			CanRX0 = 1 ;
		}
		/* INT zurücksetzen - brutale Methode */
		mcp2515_write_register(0,CANINTF,0x00) ;

	} ;

	if (!IS_SET(MCP2515_INT_2)) {
			// Pin is low on bus 1 //

		
		if (can_check_message(1)) {
			/* Message is here */
			can_get_message(1,&RXMessage1);
			CanRX1 = 1 ;
		}
		/* INT zurücksetzen - brutale Methode */
		mcp2515_write_register(1,CANINTF,0x00) ;
	} ;

} 


// -------------------------------------------------------------------------
bool mcp2515_init(uint8_t bitrate)
{
	if (bitrate >= 8)
		return false;

	SET(MCP2515_CS_1);
	SET_OUTPUT(MCP2515_CS_1);
	SET(MCP2515_CS_2);
	SET_OUTPUT(MCP2515_CS_2);
	
	// Aktivieren der Pins fuer das SPI Interface
	RESET(P_SCK_1);
	RESET(P_MOSI_1);
	RESET(P_MISO_1);
	RESET(P_SCK_2);
	RESET(P_MOSI_2);
	RESET(P_MISO_2);
	
	SET_OUTPUT(P_SCK_1);
	SET_OUTPUT(P_MOSI_1);
	SET_INPUT(P_MISO_1);
	SET_OUTPUT(P_SCK_2);
	SET_OUTPUT(P_MOSI_2);
	SET_INPUT(P_MISO_2);

	SET(MCP_RESET) ;
	SET_OUTPUT(MCP_RESET) ;
	RESET(MCP_RESET) ;
	_delay_us(100) ;
	SET(MCP_RESET) ;
	
	// SPI Einstellung setzen
	mcp2515_spi_init();
	
	// MCP2515 per Software Reset zuruecksetzten,
	// danach ist er automatisch im Konfigurations Modus
	RESET(MCP2515_CS_1);
	spi_putc(0,SPI_RESET);
	
	_delay_ms(1);
	
	SET(MCP2515_CS_1);

	RESET(MCP2515_CS_2);
	spi_putc(1,SPI_RESET);
	
	_delay_ms(1);
	
	SET(MCP2515_CS_2);

	
	// ein bisschen warten bis der MCP2515 sich neu gestartet hat
	_delay_ms(10);
	
	// CNF1..3 Register laden (Bittiming)
	RESET(MCP2515_CS_1);
	
	spi_putc(0,SPI_WRITE);
	spi_putc(0,CNF3);
	for (uint8_t i=0; i<3 ;i++ ) {
		spi_putc(0,pgm_read_byte(&_mcp2515_cnf[bitrate][i]));
	}
	// aktivieren/deaktivieren der Interrupts
	spi_putc(0,MCP2515_INTERRUPTS);
	SET(MCP2515_CS_1);

	RESET(MCP2515_CS_2);

	spi_putc(1,SPI_WRITE);
	spi_putc(1,CNF3);
	for (uint8_t i=0; i<3 ;i++ ) {
		spi_putc(1,pgm_read_byte(&_mcp2515_cnf[bitrate][i]));
	}
	// aktivieren/deaktivieren der Interrupts
	spi_putc(1,MCP2515_INTERRUPTS);
	SET(MCP2515_CS_2);

	
	// TXnRTS Bits als Inputs schalten
	mcp2515_write_register(0,TXRTSCTRL, 0);
	mcp2515_write_register(1,TXRTSCTRL, 0);
	
	SET_INPUT(MCP2515_INT_1);
	SET(MCP2515_INT_1);
	SET_INPUT(MCP2515_INT_2);
	SET(MCP2515_INT_2);

	// Deaktivieren der Pins RXnBF Pins (High Impedance State)
	mcp2515_write_register(0,BFPCTRL, 0);
	mcp2515_write_register(1,BFPCTRL, 0);
	
	// Testen ob das auf die beschreibenen Register zugegriffen werden kann
	// (=> ist der Chip ueberhaupt ansprechbar?)
	bool error = false;
	if (mcp2515_read_register(0,CNF2) != pgm_read_byte(&_mcp2515_cnf[bitrate][1])) {
		error = true;
	}
	if (mcp2515_read_register(1,CNF2) != pgm_read_byte(&_mcp2515_cnf[bitrate][1])) {
		error = true;
	}

	// Device zurueck in den normalen Modus versetzten
	// und aktivieren/deaktivieren des Clkout-Pins
	mcp2515_write_register(0,CANCTRL, CLKOUT_PRESCALER_);
	mcp2515_write_register(1,CANCTRL, CLKOUT_PRESCALER_);
	
	if (error) {
		return false;
	}
	else
	{
		while ((mcp2515_read_register(0,CANSTAT) & 0xe0) != 0) {
			// warten bis der neue Modus uebernommen wurde
		}
		
		while ((mcp2515_read_register(1,CANSTAT) & 0xe0) != 0) {
			// warten bis der neue Modus uebernommen wurde
		}
	}
	
//	can_static_filter(0,can_filter);
//	can_static_filter(1,can_filter);
	_delay_ms(100) ;
	
	PCINT_set( PCIE2, Can_Int );
	PCINT_enablePIN(PCINT20,PCIE2) ;
	PCINT_enablePIN(PCINT21,PCIE2) ;	
	PCINT_enablePCINT(PCIE2) ;

	Can_Int() ; //Abarbeiten eines eventuell schon vorhandenen Paketes.

} 



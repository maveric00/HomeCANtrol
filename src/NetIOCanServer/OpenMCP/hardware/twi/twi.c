/***************************************************************************
 *            twi.c
 *
 *  Tue Oct  6 22:29:59 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
/*******************************************************
 Author:					Manfred Langemann
 mailto:					Manfred.Langemann ät t-online.de
 Begin of project:			04.01.2008
 Latest version generated:	04.01.2008
 Filename:					TWI_Master.c
 Description:    			TWI Master functions

 Master code adapted form Peter Fleury <pfleury@gmx.ch>
 http://jump.to/fleury

 ********************************************************/
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "config.h"

#if defined(TWI)

#include "twi.h"
#include "system/clock/clock.h"

/****************************************************************************
  TWI State codes
****************************************************************************/
// General TWI Master staus codes                      
#define TWI_START					0x08  // START has been transmitted  
#define TWI_REP_START				0x10  // Repeated START has been transmitted
#define TWI_ARB_LOST				0x38  // Arbitration lost

// TWI Master Transmitter staus codes                      
#define TWI_MTX_ADR_ACK				0x18  // SLA+W has been tramsmitted and ACK received
#define TWI_MTX_ADR_NACK			0x20  // SLA+W has been tramsmitted and NACK received 
#define TWI_MTX_DATA_ACK			0x28  // Data byte has been tramsmitted and ACK received
#define TWI_MTX_DATA_NACK			0x30  // Data byte has been tramsmitted and NACK received 

// TWI Master Receiver staus codes  
#define TWI_MRX_ADR_ACK				0x40  // SLA+R has been tramsmitted and ACK received
#define TWI_MRX_ADR_NACK			0x48  // SLA+R has been tramsmitted and NACK received
#define TWI_MRX_DATA_ACK			0x50  // Data byte has been received and ACK tramsmitted
#define TWI_MRX_DATA_NACK			0x58  // Data byte has been received and NACK tramsmitted

// TWI Slave Transmitter staus codes
#define TWI_STX_ADR_ACK				0xA8  // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST	0xB0  // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK			0xB8  // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK			0xC0  // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE	0xC8  // Last data byte in TWDR has been transmitted (TWEA = 0); ACK has been received

// TWI Slave Receiver staus codes
#define TWI_SRX_ADR_ACK				0x60  // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST	0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK				0x70  // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST	0x78  // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK		0x80  // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK		0x88  // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK		0x90  // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK		0x98  // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART		0xA0  // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE				0xF8  // No relevant state information available; TWINT = 0
#define TWI_BUS_ERROR				0x00  // Bus error due to an illegal START or STOP condition

char TWI_Init ( long TWI_Bitrate )
{
	TWBR = ((F_CPU/TWI_Bitrate)-16)/2;
	if (TWBR < 11) return FALSE;
	
	return TRUE;
}

char TWI_SendAddress ( char Address, char TWI_RW )
{
	char twst;
	int timer;

	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return FALSE;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return FALSE;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );

	twst = TWSR & 0xF8;
	if ((twst != TWI_START) && (twst != TWI_REP_START)) return FALSE;

	TWDR = (Address<<1) + TWI_RW;
	TWCR = (1<<TWINT)|(1<<TWEN);

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return FALSE;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return FALSE;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );

	twst = TWSR & 0xF8;
	if ((twst != TWI_MTX_ADR_ACK) && (twst != TWI_MRX_ADR_ACK)) return FALSE;

	return TRUE;
}

void TWI_SendStart(void)
{
	int timer;

	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); //make start

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );

	if (( TWSR != TWI_START) && ( TWSR != TWI_REP_START))
	    return;

	return;
}

void TWI_SendStop ( void )
{
	int timer;

	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );

}

char TWI_Write ( char Data )
{
	int timer;
	char twst;

	TWDR = Data;
	TWCR = (1<<TWINT)|(1<<TWEN);

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return FALSE;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return FALSE;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );

	twst = TWSR & 0xF8;
	if (twst != TWI_MTX_DATA_ACK) return 1;

	return 0;
}

char TWI_ReadAck ( void )
{
	int timer;

	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return FALSE;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return FALSE;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );

	return TWDR;
}

char TWI_ReadNack ( void )
{
	int timer;

	TWCR = (1<<TWINT)|(1<<TWEN);

	timer = CLOCK_RegisterCoundowntimer();
	if( timer == CLOCK_FAILED )
		return FALSE;
	CLOCK_SetCountdownTimer( timer , TWITIMEOUT, MSECOUND );

	while (!(TWCR & (1<<TWINT)))
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return FALSE;
		}
	}
	CLOCK_ReleaseCountdownTimer( timer );
	
	return TWDR;
}

int TWI_Scan( void )
{
	unsigned char i;
	int Devices=0;

	for ( i = 0; i < 128 ; i++ )
	{
		if ( TWI_SendAddress( i , TWI_WRITE ) == TRUE )
		{
			TWI_SendStop();
			Devices++;
		}			
	}

	return( Devices );
}#endif

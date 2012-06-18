/*!\file 1wire.c \brief Stellt Treiber für 1-Wire zu Verfügung */
/***************************************************************************
 *            1wire.c
 *
 *  Mon Apr 27 23:28:56 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 *
 * Der Code ist in Anlehnung an Peter Dannegger seinem Code geschrieben.
 * Veröffentlicht unter http://www.mikrocontroller.net/topic/14792 .
 ****************************************************************************/
///	\ingroup hardware
///	\defgroup 1wire Stellt Treiber für 1-Wire zu Verfügung (1wire.c)
///	\code #include "1wire.h" \endcode
///	\par Uebersicht
//****************************************************************************/
//@{
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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "config.h"

#if defined(ONEWIRE)

#include "hardware/timer1/timer1.h"
#include "1wire.h"

/*
const char FAMILY_10[] PROGMEM = "high precision digital thermometer DS18S20";
const char FAMILY_22[] PROGMEM = "high precision digital thermometer DS1822";
const char FAMILY_28[] PROGMEM = "programmable resolution digital thermometer DS18B20";
const char FAMILY_UNKOWN[] PROGMEM = "unkown family code";
*/
const char FAMILY_10[] PROGMEM = "DS18S20";
const char FAMILY_22[] PROGMEM = "DS1822";
const char FAMILY_28[] PROGMEM = "DS18B20";
const char FAMILY_UNKOWN[] PROGMEM = "unkown family code";

FAMILY family[ ] = {
	{ 0x10, FAMILY_10 } ,
//	{ 0x1B, FAMILY_1B } ,
	{ 0x22, FAMILY_22 } ,
	{ 0x28, FAMILY_28 } ,
//	{ 0x29, FAMILY_29 } ,
//	{ 0x2c, FAMILY_2C } ,
	{ 0, FAMILY_UNKOWN }
};

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
char ONEWIRE_reset( void )
{	char sreg_tmp;
	char Error;
	
	// DS Ausgang setzen auf Low	DQ_OUT &= ~(1<<DQ_PIN);
	DQ_DDR |= 1<<DQ_PIN;
	
	// Interrups sperren
	sreg_tmp = SREG;
	cli();
	
	// 480µs warten für Reset
	timer1_wait( 960 );

	// DS auf Eingang setzen
	DQ_DDR &= ~(1<<DQ_PIN);
	
	// warten
	timer1_wait( 132 );
	
	// immer noch Power ? Dann Fehler
	Error = DQ_IN & ( 1<<DQ_PIN );

	// Warten
	timer1_wait( 828 );
	
	SREG = sreg_tmp;

	//
	if( ( DQ_IN & ( 1<<DQ_PIN ) ) == 0 )
		Error = 1;
	
	return Error;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
char ONEWIRE_bitio( char BIT )
{
	char sreg_tmp;
	sreg_tmp = SREG;
	cli();
	
	DQ_DDR |= 1<<DQ_PIN;
	timer1_wait( 2 );

	if( BIT )
		DQ_DDR &= ~(1<<DQ_PIN);
	
	timer1_wait( 28 );
	
	if( (DQ_IN & ( 1<<DQ_PIN ) ) == 0 )
		BIT = 0;
		timer1_wait( 90 );
	DQ_DDR &= ~( 1<<DQ_PIN );

	SREG = sreg_tmp;

	return BIT;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned int ONEWIRE_writebyte( unsigned char BYTE )
{
	char i = 8, j;

	do{
		j = ONEWIRE_bitio( BYTE & 1 );
		BYTE >>= 1;
		if( j )
			BYTE |= 0x80;
	}while( --i );
  
	return BYTE;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned int ONEWIRE_readbyte( void )
{
  return ONEWIRE_writebyte( 0xFF );
}


/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void ONEWIRE_ParasitepowerOn( void )
{
    DQ_OUT |= 1<< DQ_PIN;
    DQ_DDR |= 1<< DQ_PIN;	
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
char ONEWIRE_searchrom( char diff, char * id )
{	
	char j, next_diff, b;

	int i;
	if( ONEWIRE_reset() )	
		return PRESENCE_ERR;

	ONEWIRE_writebyte( SEARCH_ROM );
	next_diff = LAST_DEVICE;

	i = 64;
	do{
		j = 8;
		do{
		b = ONEWIRE_bitio( 1 );
		if( ONEWIRE_bitio( 1 ) )
		{			if( b )
				return DATA_ERR;
  		}
		else
		{			if( !b )
			{
				if( diff > i || ((*id & 1) && diff != i) )
				{
					b = 1;
					next_diff = i;
				}
			}		}
 
				
		ONEWIRE_bitio( b );
		*id >>= 1;
		if( b )			*id |= 0x80;
  		i--;
		}while( --j );
		

		id++;
	
	}while( i );
	
	return next_diff;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void ONEWIRE_command( char command, char * id )
{
	char i;
	
	ONEWIRE_reset();
	if( id )
	{
		ONEWIRE_writebyte( MATCH_ROM );
		i = 8;
		do{
			ONEWIRE_writebyte( * id );
  			id++;
		}while( --i );
	}
	else
	{
		ONEWIRE_writebyte( SKIP_ROM );
	}
	ONEWIRE_writebyte( command );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Resetet alle 1-Wire Geräte auf dem Bus.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
const char * ONEWIRE_getfamilycode2string( char id )
{
	int i = 0;
	
	while ( family[ i ].id != 0 )
	{
		if ( family[ i ].id == id ) break;
		i++;
	}
	return( family[ i ].familystring );
}

#endif

//@}

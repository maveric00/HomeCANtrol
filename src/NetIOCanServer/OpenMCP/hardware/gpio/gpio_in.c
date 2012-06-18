/***************************************************************************
 *            gpio_in.c
 *
 *  Mon Mar  2 03:36:12 2009
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
 
#include <avr/io.h>

#include "config.h"

#if defined(GPIO)

#include "gpio_in.h"

void GPIO_in_init( void )
{
#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
	DDR_1 &= ~(1<<PB4);
	DDR_1 &= ~(1<<PB5) ;
	PORT_1 |= (1<<PB4 );
	PORT_1 |= (1<<PB5 );
	#endif
#endif

// PORTA = 0xff;

}

char GPIO_in_state( int pin )
{
#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
	if ( ( pin >= 0 ) && ( pin <= 5 ) )
		if ( ( PORT_2 & ( 1<<( pin + 2 ) ) ) != 0 ) 
			return( 1 );

	if ( pin == 6 )
		if ( ( PORT_1 & ( 1<<( PB0 ) ) ) != 0 ) 
			return( 1 );

	if ( pin == 7 )
		if ( ( PORT_1 & ( 1<<( PB3 ) ) ) != 0 ) 
			return( 1 );

	if ( ( pin >= 8 ) && ( pin <= 15 ) )
		if ( ( PORT_3 & ( 1<<( ( pin - 8 ) ) ) ) != 0 ) 
			return( 1 );
		
	#endif
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)

	if ( pin < MAX_GPIO_IN )
		if ( ( PIN_1 & ( 1<<( pin + 4 ) ) ) != 0 ) 
			return( 1 );
		else
			return( 0 );
	#endif
#endif

	return( 0 );
}
#endif

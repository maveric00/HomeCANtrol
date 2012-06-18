/*! \file gpio_out.c \brief Stellt die CLOCK Funkionalitaet bereit */
/***************************************************************************
 *            gpio_out.c
 *
 *  Mon Mar  2 03:36:12 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup system
///	\defgroup GPIO Die GPIO-Funktionen für Ausgänge (gpio_out.c)
///	\par Uebersicht
//****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <avr/io.h>

#include "config.h"

#if defined(GPIO)

#include "gpio_out.h"

void GPIO_out_init( void )
{

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
//	GPIO_OUT_DDR = 0xFF;
//	GPIO_OUT_PORT = 0x0;	
	#endif
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
//	GPIO_OUT_DDR |= 0x07;
//	GPIO_OUT_PORT &= ~0x7;
	#endif
#endif
}

void GPIO_out_set( char pin )
{
//	if ( pin < MAX_GPIO_OUT )
//		GPIO_OUT_PORT |= ( 1<<pin );
}

void GPIO_out_clear( char pin )
{
//	if ( pin < MAX_GPIO_OUT )
//		GPIO_OUT_PORT &= ~( 1<<pin );
}

char GPIO_out_state( char pin )
{
	if ( pin < MAX_GPIO_OUT )
		if ( ( GPIO_OUT_PORT & ( 1<<pin ) ) != 0 ) return( 1 );

	return( 0 );
}
#endif


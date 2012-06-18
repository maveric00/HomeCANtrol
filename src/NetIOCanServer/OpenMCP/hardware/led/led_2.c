/***************************************************************************
 *            led_0.c
 *
 *  Tue Mar 11 21:11:55 2008
 *  Copyright  2008  sharan
 *  <sharan@bastard>
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

#if defined(LED)

#include "led_core.h"
#include "led_2.h"

void LED_2_init( void )
{
#if defined(__AVR_ATmega2561__)
	LED_2_DDR |= ( 1<<LED_2_PIN );
	LED_Register ( LED_2_ON, LED_2_OFF, LED_2_TOGGLE );
	LED_2_OFF ();
#endif
}

void LED_2_ON ( void )
{
#if defined(__AVR_ATmega2561__)
	LED_2_PORT &= ~( 1<<LED_2_PIN );
#endif
}

void LED_2_OFF ( void )
{
#if defined(__AVR_ATmega2561__)
	LED_2_PORT |= ( 1<<LED_2_PIN );
#endif
}

void LED_2_TOGGLE ( void )
{
#if defined(__AVR_ATmega2561__)
	if ( ( LED_2_PORT & ( 1<<LED_2_PIN )) != 0 ) 
	{
		LED_2_PORT &= ~( 1<<LED_2_PIN );
	}
	else 
	{
		LED_2_PORT |= ( 1<<LED_2_PIN );
	}
#endif
}
#endif

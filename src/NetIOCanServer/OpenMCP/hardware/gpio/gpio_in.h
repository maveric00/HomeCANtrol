/***************************************************************************
 *            gpio_in.h
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
 
#ifndef _GPIO_IN_H
	#define GPIO_IN_H

#ifdef __AVR_ATmega2561__
	#error "GPIO wird von dieser Hardwareplatform nicht unterstützt, bitte in der config.h austragen."
#endif

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
		#define PORT_1			PINB
		#define DDR_1			DDRB
		#define PORT_2			PIND
		#define DDR_2			DDRD
		#define PORT_3			PINA
		#define DDR_3			DDRA
		#define MAX_GPIO_IN 	16
	#else
		#error "GPIO wird von dieser Hardwareplatform nicht unterstützt, bitte in der config.h austragen."
	#endif
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
		#define PORT_1			PORTB
		#define PIN_1			PINB
		#define DDR_1			DDRB

		#define MAX_GPIO_IN 	2
	#else
		#error "GPIO wird von dieser Hardwareplatform nicht unterstützt, bitte in der config.h austragen."
	#endif
#endif

void GPIO_in_init( void );
char GPIO_in_state( int pin );

#endif /* GPIO_H */


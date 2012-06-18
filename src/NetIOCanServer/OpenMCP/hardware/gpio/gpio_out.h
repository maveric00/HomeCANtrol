/*! \file gpio_out.h \brief Stellt die CLOCK Funkionalitaet bereit */
/***************************************************************************
 *            gpio_out.h
 *
 *  Mon Mar  2 03:36:12 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ***************************************************************************/
///	\ingroup system
///	\defgroup GPIO Die GPIO-Funktionen für Ausgänge (gpio_out.h)
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
#ifndef _GPIO_OUT_H
	#define GPIO_OUT_H


#ifdef __AVR_ATmega2561__
	#error "Hardwareplatform wird nicht unterstützt!"
#endif

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
		#define GPIO_OUT_PORT			PORTC
		#define GPIO_OUT_DDR			DDRC
		#define MAX_GPIO_OUT 	8
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
		#define GPIO_OUT_PORT			PORTA
		#define GPIO_OUT_DDR			DDRA

		#define MAX_GPIO_OUT 	3
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

void GPIO_out_init( void );
void GPIO_out_set( char pin );
void GPIO_out_clear( char pin );
char GPIO_out_state( char pin );

#endif /* GPIO_H */


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
 
#ifndef _LED_1_H
	#define LED_1_H

	void LED_1_init( void );
	void LED_1_ON ( void );
	void LED_1_OFF ( void );
	void LED_1_TOGGLE ( void );

#ifdef __AVR_ATmega2561__
	#define	LED_1_DDR		DDRD
	#define LED_1_PORT		PORTD
	#define	LED_1_PIN		PD5
#endif
#ifdef __AVR_ATmega644P__
	#define	LED_1_DDR		DDRC
	#define LED_1_PORT		PORTC
	#define	LED_1_PIN		PC5
#endif


#endif /* LED_0_H */

 

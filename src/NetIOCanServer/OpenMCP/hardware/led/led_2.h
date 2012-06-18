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
 
#ifndef _LED_2_H
	#define LED_2_H

	void LED_2_init( void );
	void LED_2_ON ( void );
	void LED_2_OFF ( void );
	void LED_2_TOGGLE ( void );

	#define	LED_2_DDR		DDRD
	#define LED_2_PORT		PORTD
	#define	LED_2_PIN		PD6

#endif /* LED_0_H */

 

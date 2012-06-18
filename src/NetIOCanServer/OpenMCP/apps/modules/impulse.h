/***************************************************************************
 *            impulse.h
 *
 *  Tue Nov 17 18:31:24 2009
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

#ifndef _IMPULSE_H
	#define _IMPULSE_H

	#define CounterPin	6

	void IMPULS_init( void );
	long IMPULS_getCounter( void );
	void IMPULS_setCounter( long Impulse );
	void IMPULS_saveAll( void );
	void IMPULS_setPrescaler( long Prescaler );
	long IMPULS_getPrescaler( void );
	void IMPULS_setUnit( char * Unit );
	char * IMPULS_getUnit( char * Unit );
	void IMPULS_Interrupt( void );
	void cgi_impuls( void * pStruct );

#endif

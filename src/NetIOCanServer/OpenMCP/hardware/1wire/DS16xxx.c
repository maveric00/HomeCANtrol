/***************************************************************************
 *            DS16xxx.c
 *
 *  Wed Dec 23 17:04:05 2009
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

#include "1wire.h"
#include "DS16xxx.h"
#include "system/clock/clock.h"

int DS16xxx_readtemp( char * id )
{
	int Temp;	
	
	// Wenn Temperatursensor, dann Messung starten und Temperatur ausgeben
	if( * id == 0x28 || * id == 0x22 || * id == 0x10 )
	{
		ONEWIRE_command( CONVERT_T, id );
		ONEWIRE_ParasitepowerOn( );
		CLOCK_delay( 750 );
		ONEWIRE_command( READ , id );
		Temp = ONEWIRE_readbyte( );
		Temp |= ( ONEWIRE_readbyte( ) << 8 );
		
		if( * id == 0x10 )			// 9 -> 12 bit
    		Temp <<= 3;

		Temp = ( Temp << 4 );
	}
	else
	{
		Temp = 0x8000;
	}
	return( Temp );
}


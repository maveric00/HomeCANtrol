/***************************************************************************
 *            temp.c
 *
 *  Sat Dec 26 21:53:01 2009
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
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/version.h>
#include <string.h>

#include "config.h"

#if defined(TWI)
	#include "hardware/twi/lm75.h"
#endif
#if defined(ONEWIRE)
	#include "hardware/1wire/DS16xxx.h"
#endif
#include "system/string/string.h"

int TEMP_readtempstr( char * SensorConfig )
{
	char BUS;
	char ID[8];
	char * BUS_String;
	char * ID_String;
	char * NAME_String;
	int i;
	int Temp;
	
	BUS_String = SensorConfig;
	
	// String zerlegen für ID
	while( * SensorConfig )
	{
		if ( * SensorConfig == ';' )
		{
			* SensorConfig = '\0';
			NAME_String = SensorConfig;
			SensorConfig++;
			ID_String = SensorConfig;
			break;
		}
		SensorConfig++;
	}

	// weiter suchen nach Namen
	while( * SensorConfig )
	{
		if ( * SensorConfig == ';' )
		{
			* SensorConfig = '\0';
			SensorConfig++;
			NAME_String = SensorConfig;
			break;
		}
		SensorConfig++;
	}

#if defined(TWI)
	if ( !strcmp_P( BUS_String, PSTR("TWI") ) )
	{
		strtobin( ID_String, ID, 2 );
		Temp = LM75_readtemp( ID );
	}
#endif
#if defined(ONEWIRE)
	if ( !strcmp_P( BUS_String, PSTR("ONEWIRE") ) )
	{
		strtobin( ID_String, ID, 16 );
		Temp = DS16xxx_readtemp( ID );
	}
#endif
	
	return( Temp );
}

int TEMP_readtemp( char * BUS, char * ID )
{

}
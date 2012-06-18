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
 
#include <avr/pgmspace.h>
#include <avr/version.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"

#include "system/thread/thread.h"
#include "system/clock/clock.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"
#include "cmd_temp.h"

#include "system/sensor/temp.h"

#include "system/nano_DB/nano_db.h"

void init_cmd_temp( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_temp, PSTR("temp"));
#endif
}

int cmd_temp( int argc, char ** argv )
{	
	char db[100];

	if ( !strcmp_P( argv[1], PSTR("add") ) )
		nano_DB_writeDBentry( argv[2], atol( argv[3] ), db, sizeof( db ) );
	else if ( !strcmp_P( argv[1], PSTR("make") ) )
		nano_DB_makeDB( argv[2] );
	else if ( !strcmp_P( argv[1], PSTR("get") ) )
		printf_P( PSTR("DBentry: %d \r\n") ,nano_DB_getnumbersofDB( argv[2] , sizeof( db ) ) );
	else if ( !strcmp_P( argv[1], PSTR("read") ) )
		printf_P( PSTR("DBentry: %d gelesen\r\n") , nano_DB_readDBentry( argv[2] , atol( argv[3] ), db, sizeof( db ) ) );

	/*
	int Temp;
	char VZ;
	
	if ( argc == 2 )
	{
		Temp = TEMP_readtempstr( argv[ 1 ] );
		
		if ( Temp < 0 )
			VZ = '-';
		else
			VZ = '+';
	
		printf_P( PSTR("%c%d.%01d C\r\n"), VZ, abs(Temp / 256) , abs((Temp << 8 ) / 6250 )); // 0.1øC

	}
*/
	
}

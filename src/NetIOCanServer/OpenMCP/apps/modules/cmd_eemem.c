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

#include "system/config/config.h"

#include "config.h"

#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_eemem.h"

void init_cmd_eemem( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_eemem, PSTR("eemem"));
#endif
#if defined(HTTPSERVER_EEMEM)
	cgi_RegisterCGI( cgi_eemem, PSTR("eemem.cgi"));
#endif
}

int cmd_eemem( int argc, char ** argv )
{
	int ERROR = -1;
	char string[ 32 ];

	
	if ( argc == 2 )
	{
		if ( !strcmp_P( argv[1], PSTR("protect") ) )
		{
			strcpy_P( string, PSTR("ON"));
			changeConfig_P( PSTR("WRITE_PROTECT"), string );
			setprotectConfig( PROTECT );
			ERROR = 0; 
		}
		if ( !strcmp_P( argv[1], PSTR("unprotect") ) )
		{
			setprotectConfig( UNPROTECT );
			strcpy_P( string, PSTR("OFF"));
			changeConfig_P( PSTR("WRITE_PROTECT"), string );
			printf_P( PSTR("unprotectet\r\n"));
			ERROR = 0; 
		}
		if ( !strcmp_P( argv[1], PSTR("print") ) )
		{
			PrintConfig();
			ERROR = 0; 
		}
		if ( !strcmp_P( argv[1], PSTR("clean") ) )
		{
			makeConfig();
			ERROR = 0; 
		}
	}

	if ( ERROR == -1 )
		printf_P( PSTR("eemem <protect>|<unprotect> <print> <clean>\r\n"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Anzeigen der Konfigurationsdaten im EEProm. 
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_eemem( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;

	cgi_PrintHttpheaderStart();

	printf_P( PSTR(	"<pre>"));

	PrintConfig ();

	printf_P( PSTR( "</pre>" ));

	cgi_PrintHttpheaderEnd();

}

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
 
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/version.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"

#if defined(ONEWIRE)

#include "hardware/1wire/1wire.h"
#include "hardware/1wire/DS16xxx.h"

#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_onewire.h"

void init_cmd_onewire( void )
{
#if defined(HTTPSERVER_ONEWIRE)
	cgi_RegisterCGI( cgi_onewire, PSTR("onewire.cgi"));
#endif
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Anzeigen der 1-Wire-Devices und Ausgabe der Temperatur wenn Temp-Sensor.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_onewire(  void * pStruct )
{
    struct HTTP_REQUEST * http_request;
    http_request = (struct HTTP_REQUEST *) pStruct;
 
	char id[8], diff;
	char i,VZ;
	int Temp;

/*	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<TITLE>1-Wire</TITLE>"
					"</HEAD>"
					"<BODY>" */

	cgi_PrintHttpheaderStart();

	printf_P( PSTR(	"<pre>"
					"Folgende Geraete wurden gefunden:\r\n")); 

	// 1-Wire Gereäte suchen
	for( diff = SEARCH_FIRST; diff != LAST_DEVICE; )
	{
		cli();
		diff = ONEWIRE_searchrom( diff, id );
		sei();
				
		if( diff == PRESENCE_ERR )
		{
			printf_P( PSTR("No Sensor found\r\n" ));
			break;
		}
		if( diff == DATA_ERR )
		{
			printf_P( PSTR( "Bus Error\r\n" ));
			break;
		}

		// Device-ID ausgeben + familystring
		printf_P( PSTR( "ID: " ) );
  		for( i = 0; i < 7; i++ )
		{
			printf_P( PSTR("%02X:"), id[i] );
		}
		printf_P( PSTR("%02X ("), id[ i ] );
		printf_P( ONEWIRE_getfamilycode2string ( id[0] ) );

		// Auf tempsensor testen
		if ( DS16xxx_readtemp( id ) != 0x8000 )
		{
			Temp = DS16xxx_readtemp( id );
			
			if ( Temp < 0 )
				VZ = '-';
			else
				VZ = '+';
			
			printf_P( PSTR(" %c%d.%01d C"), VZ, abs(Temp / 256) , abs((Temp << 8 ) / 6250 )); // 0.1øC			
		}
		printf_P( PSTR(")\r\n"));
	}
				 
	cgi_PrintHttpheaderEnd();

	/*printf_P( PSTR( "</pre>"
					"</BODY>"
					"</HTML>"
					"\r\n\r\n")); */
}
#endif

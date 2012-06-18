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

#include "system/softreset/softreset.h"
#include "config.h"

#if defined(TWI)

#include "hardware/twi/twi.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_twi.h"
#include "hardware/twi/lm75.h"

void init_cmd_twi( void )
{
	#if defined(HTTPSERVER_TWI)
		cgi_RegisterCGI( cgi_twi, PSTR("twi.cgi"));
	#endif
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Anzeigen der 1-Wire-Devices und Ausgabe der Temperatur wenn Temp-Sensor.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_twi(  void * pStruct )
{
    struct HTTP_REQUEST * http_request;
    http_request = (struct HTTP_REQUEST *) pStruct;
 
	char i;

	int Temp;
	char VZ;
	
	cgi_PrintHttpheaderStart();

	printf_P( PSTR(	"<pre>"
					"Scanne nach TWI Slaves:\r\nSlaves gefunden an Adresse:")); 

	for ( i = 0 ; i < 128 ; i++ )
	{
		if ( TWI_SendAddress( i , TWI_WRITE ) == TRUE )
		{
			printf_P( PSTR(" 0x%02x\r\n"), i );
		}			
		TWI_SendStop();
	}

	printf_P( PSTR(	"\r\nScanne nach LM75 Temperatursensoren:\r\n")); 

	for ( i = 0x48 ; i < 0x50 ; i++ )
	{
		if ( LM75_readtemp( &i ) != 0x8000 )
		{
			Temp = LM75_readtemp( &i );
			
			if ( Temp < 0 )
				VZ = '-';
			else
				VZ = '+';
			
			printf_P( PSTR("LM75 Sensor gefunden auf Adresse: 0x%02x, Temperatur %c%d.%01d C\r\n"), i, VZ, abs(Temp / 256) , abs((Temp << 8 ) / 6250 )); // 0.1øC			
		}			
	}

	printf_P( PSTR( "</pre>" ));

	cgi_PrintHttpheaderEnd();
}
#endif

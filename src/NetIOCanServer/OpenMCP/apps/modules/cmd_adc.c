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

#if defined(ANALOG)

#include "hardware/adc/adc.h"

#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"
#include "cmd_adc.h"

void init_cmd_adc( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_adc, PSTR("adc") );
#endif
#if defined(HTTPSERVER_ANALOG)
	cgi_RegisterCGI( cgi_aio, PSTR("aio.cgi") );
#endif
}

int cmd_adc( int argc, char ** argv )
{
	int channel;
	
	if ( !strcmp_P( argv[ 1 ], PSTR("get") ) )
	{
		channel = atoi( argv[ 2 ] );
		if ( channel < 8 && channel >= 0 )
			printf_P( PSTR("ADC (%d) = %d\r\n"), channel, ADC_GetValue( channel ) );
		else
			printf_P( PSTR("Error, %d ist kein gülter ADC-Eingang\r\n"), channel );
	}
	else
		printf_P( PSTR("adc <get> <0..7>\r\n"));

}

#if defined(HTTPSERVER_ANALOG)
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface zum anzeigen der Analogeingänge
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_aio( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char i;
	char gpio[10];
	int temp;

/*	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<TITLE>ADC</TITLE>"
					"</HEAD>"
					"<BODY>"*/
	cgi_PrintHttpheaderStart();
	printf_P( PSTR(	"<form action=\"dio.cgi\">"
					"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">" ) );
	for( i = 0 ; i < MAX_ADC ; i++ )
	{
		temp = ADC_mVolt( ADC_GetValue( i ) + 1 , 5000 );
		printf_P( PSTR(	"<tr>"
					    "<td align=\"right\">ADC %d =</td>"
					   	"<td align=\"right\"> %d.%03dV</td>"
 						"</tr>\r"), i, temp / 1000, temp % 1000 );
	}
	printf_P( PSTR(	"</table>"
					"</form>"));
	cgi_PrintHttpheaderEnd();
/*					"</BODY>"
					"</HTML>\r\n"
					"\r\n")); */
}
#endif

#endif

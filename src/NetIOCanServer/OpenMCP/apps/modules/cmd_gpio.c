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

#if defined(GPIO)

#include "hardware/gpio/gpio_in.h"
#include "hardware/gpio/gpio_out.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_gpio.h"

void init_cmd_gpio( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_gpio, PSTR("gpio_out"));
#endif
#if defined(HTTPSERVER_GPIO)
	cgi_RegisterCGI( cgi_dio_in, PSTR("dio_in.cgi"));
	cgi_RegisterCGI( cgi_dio_out, PSTR("dio_out.cgi"));
#endif
}

int cmd_gpio( int argc, char ** argv )
{
	int channel;
	
	channel = atoi( argv[ 2 ] );
	if ( !(channel < MAX_GPIO_OUT && channel >= 0) )	
	{
		printf_P( PSTR("Error, %d ist kein gülter Ausgang.\r\n"), channel );
		return( 0 );
	}
	
	if ( !strcmp_P( argv[ 1 ], PSTR("get") ) )
	{
	}
	else if ( !strcmp_P( argv[ 1 ], PSTR("set") ) )
	{
		GPIO_out_set( channel );
	}
	else if ( !strcmp_P( argv[ 1 ], PSTR("toggle") ) )
	{
		if ( GPIO_out_state( channel ) == 1 )
			GPIO_out_clear( channel );
		else
			GPIO_out_set( channel );
	}
	else if ( !strcmp_P( argv[ 1 ], PSTR("clear") ) )
	{
		GPIO_out_clear( channel );
	}
	else
		printf_P( PSTR("gpio_out <get|set|toggle|clear> <0..7>\r\n"));	

	printf_P( PSTR("GPIO (%d) = %d\r\n"), channel, GPIO_out_state( channel ) );

}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Anzeigen der Digitalen Eingänge.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_dio_in( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char i;
	char gpio[10];
		
/*	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<TITLE>GPIO</TITLE>"
					"</HEAD>"
					"<BODY>"*/
	cgi_PrintHttpheaderStart();

	printf_P( PSTR(	"<form action=\"dio_out.cgi\">"
					"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">" ) );

	for( i = 0 ; i < MAX_GPIO_IN ; i++ )
	{
		printf_P( PSTR(	"<tr>"
					   	"<td align=\"right\">GPIO %d =</td>" ), i);

		if ( GPIO_in_state( i ) != 0 )
		{
			printf_P( PSTR( "<td>On</td>" ), i); 
		}
		else
		{
			printf_P( PSTR( "<td>Off</td>" ), i); 
		}
		printf_P( PSTR(	"</td>"
 						"</tr>") );
	}
	
	printf_P( PSTR(	"</table>"
					"</form>"
					"</BODY>"
					"</HTML>\r\n"
					"\r\n"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Steuern der Digitalen Ausgänge
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_dio_out( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char i;
	char gpio[10];
	
	if ( http_request->argc != 0 )
	{
		for( i = 0 ; i < MAX_GPIO_OUT ; i++ )
		{
			sprintf_P( gpio, PSTR("GPIO%d"), i );
			if ( PharseCheckName( http_request, gpio ) ) GPIO_out_set( i );
			else GPIO_out_clear( i );
		}
	}

	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<TITLE>GPIO</TITLE>"
					"</HEAD>"
					"<BODY>"
					"<form action=\"dio_out.cgi\">"
					"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">" ) );

	for( i = 0 ; i < MAX_GPIO_OUT ; i++ )
	{
		printf_P( PSTR(	"<tr>"
					   	"<td align=\"right\">GPIO %d</td>" ), i);

		if ( GPIO_out_state( i ) != 0 )
		{
			
			printf_P( PSTR( "<td><input type=\"checkbox\" name=\"GPIO%d\" checked" ), i); 
		}
		else
		{
			printf_P( PSTR( "<td><input type=\"checkbox\" name=\"GPIO%d\"" ), i); 
		}
		printf_P( PSTR(	"></td>"
 						"</tr>") );
	}

	printf_P( PSTR(	"<tr>"
					"<td></td><td><input type=\"submit\" name=\"SUB\"></td>"
					"</tr>"
				   	"</table>"
					"</form>" ));

	cgi_PrintHttpheaderEnd();

/*					"</BODY>"
					"</HTML>\r\n"
					"\r\n")); */
}
#endif

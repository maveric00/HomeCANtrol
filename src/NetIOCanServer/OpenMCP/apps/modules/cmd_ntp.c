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

#if defined(NTP)

#include "hardware/network/enc28j60.h"
#include "hardware/uart/uart.h"
#include "system/clock/clock.h"
#include "system/net/ip.h"
#include "system/net/dns.h"
#include "system/net/ntp.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"

#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_ntp.h"

void init_cmd_ntp( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_ntp, PSTR("ntp"));
#endif
#if defined(HTTPSERVER_NTP)
	cgi_RegisterCGI( cgi_ntp, PSTR("ntp.cgi"));
#endif
}

int cmd_ntp( int argc, char ** argv )
{ 
	long ip;
	char ipstr[ 20 ];

	struct TIME time;
	
	if ( argc == 2 )
	{
		ip = strtoip( argv[1] );

		if ( ip == 0 )
		{
			ip = DNS_ResolveName( argv[ 1 ] );
			if ( ip == DNS_NO_ANSWER )
			{
				printf_P( PSTR("Fehler\r\n"));
				return( 0 );
			}
		}
		
		printf_P( PSTR("Hole Zeit von %s\r\n"), argv[ 1 ] );
		
		if( checkConfigName_P( PSTR("UTCZONE") ) != -1 )
			readConfig_P ( PSTR("UTCZONE"), ipstr );
		else
			ipstr[0] = '\0';

		if ( NTP_GetTime( ip , 0 , atol( ipstr ) ) != NTP_ERROR )
		{
			CLOCK_GetTime( &time );
			printf_P( PSTR("Neue Zeit: %02d:%02d:%02d\r\n") , time.hh , time.mm , time.ss );
		}
		else
			printf_P( PSTR("Fehler\r\n"));			
	}
	else
		printf_P( PSTR("ntp <ntpserver>\r\n"));

	return( 0 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Einstellen und Anzeigen für den NTP-Service.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_ntp( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char NTPSERVER[32];
	char UTCZONE[4];
/*
	printf_P( PSTR(		"<HTML>\r\n"
						"<HEAD>\r\n"
						"<TITLE>SNTP</TITLE>\r\n"
						"</HEAD>\r\n"
						" <BODY>\r\n"));
*/
	cgi_PrintHttpheaderStart();

	// Wenn keine Variablen übergeben wurden, dann Config ausgeben,
	// wenn Variablen übergeben wurden Config ändern
	if ( http_request->argc == 0 )
	{
		readConfig_P( PSTR("NTP"), NTPSERVER );
		printf_P( PSTR(	"<form action=\"ntp.cgi\">\r"
					   	"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">"
  						"<tr>"
					   	"<td align=\"right\">SNTP aktivieren</td>"
					    "<td><input type=\"checkbox\" name=\"NTP\" value=\"on\" " )); 
		if ( !strcmp_P( NTPSERVER, PSTR("on") ) )
						printf_P( PSTR("checked"));
		printf_P( PSTR(	"></td>"
  						"</tr>") );
		
		if( checkConfigName_P( PSTR("NTP") ) != -1 )
			readConfig_P ( PSTR("NTPSERVER"), NTPSERVER );
  		else
			strcpy_P ( NTPSERVER, PSTR("time.fu-berlin.de"));
			
		if( checkConfigName_P( PSTR("UTCZONE") ) != -1 )
			readConfig_P ( PSTR("UTCZONE"), UTCZONE );
  		else
			strcpy_P ( UTCZONE, PSTR("0"));

		printf_P( PSTR(	"<tr>"
					   	"<td align=\"right\">SNTP-Server</td>"
  						"<td><input name=\"NTPSERVER\" type=\"text\" size=\"25\" value=\"%s\" maxlength=\"25\"></td>"
  						"</tr>"
						"<tr>"
					   	"<td align=\"right\">Zeitzone</td>"
						"<td><input name=\"UTCZONE\" type=\"text\" size=\"3\" value=\"%s\" maxlength=\"3\"></td>"
  						"</tr>"
 						"<tr>"
   						"<td></td><td><input type=\"submit\" value=\" Einstellung &Uuml;bernehmen \"></td>"
  						"</tr>"
					   	"</table>"
						"</form>" ) , NTPSERVER, UTCZONE );
	}
	else
	{
		if ( PharseCheckName_P( http_request, PSTR("NTP") ) )
			changeConfig_P( PSTR("NTP"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("NTP") ) ] );
		else
			changeConfig_P( PSTR("NTP"), "off" );	
		if ( PharseCheckName_P( http_request, PSTR("NTPSERVER") ) )
			changeConfig_P( PSTR("NTPSERVER"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("NTPSERVER") ) ] );
		if ( PharseCheckName_P( http_request, PSTR("UTCZONE") ) )
			changeConfig_P( PSTR("UTCZONE"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("UTCZONE") ) ] );
		
		printf_P( PSTR("Einstellungen uebernommen!\r\n"));

	}

	cgi_PrintHttpheaderEnd();

/*
	 printf_P( PSTR( "</BODY>"
					"</HTML>"
					"\r\n\r\n"));
*/
}
#endif

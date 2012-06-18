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

#if defined(LEDTAFEL)

#include "system/led-tafel/tafel.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_tafel.h"

void init_cmd_tafel( void )
{
	cgi_RegisterCGI( cgi_tafel, PSTR("tafel.cgi"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die streamingengine.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_tafel( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	unsigned int Zeile = 1,Spalte = 1,Seite = 1, Tafel = 2;
	unsigned char clrstring[] = { 27,'L',0 };
	
	
	if ( PharseCheckName_P( http_request, PSTR("tafel") ) )
		Tafel = atol( http_request->argvalue[ PharseGetValue_P( http_request, PSTR("tafel") ) ] );

	if ( PharseCheckName_P( http_request, PSTR("seite") ) )
		Seite = atol( http_request->argvalue[ PharseGetValue_P( http_request, PSTR("seite") ) ] );

	if ( PharseCheckName_P( http_request, PSTR("spalte") ) )
		Spalte = atol( http_request->argvalue[ PharseGetValue_P( http_request, PSTR("spalte") ) ] );
		
	if ( PharseCheckName_P( http_request, PSTR("zeile") ) )
		Zeile = atol( http_request->argvalue[ PharseGetValue_P( http_request, PSTR("zeile") ) ] );

	printf_P( PSTR(		"<HTML>\r\n"
						"<HEAD>\r\n"
						"<TITLE>Reset</TITLE>\r\n"
						"</HEAD>\r\n"
						"<BODY>\r\n"));

	if ( http_request->argc == 0 )
	{
		printf_P( PSTR(	"<form action=\"tafel.cgi\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "Zeile<input name=\"zeile\" size=\"2\" value=\"1\">"
					    "Spalte<input name=\"spalte\" size=\"2\" value=\"1\">"
					    "Text<input name=\"text\" size=\"56\">"
						"<p><input type=\"submit\" value=\"Abschicken\"></p></form>"
					   	"<form action=\"tafel.cgi?clr\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p><input type=\"submit\" name=\"clr\" value=\"clr\"></p></form>"
						"</BODY>\r\n"
						"</HTML>\r\n"
						"\r\n"));
	}
	else if( PharseCheckName_P( http_request, PSTR("clr") ) )
	{
		tafel_print ( Tafel , Seite , Zeile , Spalte , clrstring );
		printf_P( PSTR(	"LED-Anzeige löschen</BODY>\r\n"
							"</HTML>\r\n"
							"\r\n"));		
	}
	else if( PharseCheckName_P( http_request, PSTR("text") ) )
	{			
		printf_P( PSTR("\"%s\" "), http_request->argvalue[ PharseGetValue_P( http_request, PSTR("text") ) ] );
		printf_P( PSTR(	"gesendet\r\n" ));

		printf_P( PSTR("<pre>Tafel-ACK -> "));
		
		tafel_print ( Tafel , Seite , Zeile , Spalte , http_request->argvalue[ PharseGetValue_P( http_request, PSTR("text") ) ] );
		
		printf_P( PSTR(	"</pre></BODY>\r\n"
						"</HTML>\r\n"
						"\r\n"));
	}
	STDOUT_Flush();	
}
#endif

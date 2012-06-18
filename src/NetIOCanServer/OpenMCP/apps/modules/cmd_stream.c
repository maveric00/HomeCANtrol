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

#include "apps/mp3-streamingclient/mp3-clientserver.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_stream.h"

void init_cmd_stream( void )
{
#if defined(HTTPSERVER_STREAM)
	cgi_RegisterCGI( cgi_stream, PSTR("stream.cgi"));
#endif
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Streaminterface für zum Steuern der Streamingengine
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_stream( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	

	printf_P( PSTR(		"<HTML>\r\n"
						"<HEAD>\r\n"));
	if( http_request->argc != 0 && PharseCheckName_P ( http_request, PSTR("info") ) )
	{
		printf_P( PSTR(	"<meta http-equiv=\"refresh\" content=\"60; \">\r\n"));
	}
	
	printf_P( PSTR(	"<TITLE>Streaming</TITLE>\r\n"
					"</HEAD>\r\n"
					"<BODY>\r\n"));

	if ( http_request->argc == 0  )
	{
		printf_P( PSTR( "<form action=\"stream.cgi\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p>Stream-URL:<input name=\"streamurl\" size=\"50\"></p>"
					    "<p><input type=\"submit\" value=\"stream starten\"></p></form>"
					   	"<form action=\"stream.cgi?stop\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p><input type=\"submit\" name=\"stop\" value=\"stop\"></p></form>"
					   	"<form action=\"stream.cgi?replay\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p><input type=\"submit\" name=\"replay\" value=\"replay\"></p></form>" ));
	}
	else if( PharseCheckName_P ( http_request, PSTR("streamurl") ) )
	{	
		printf_P( PSTR("<pre><p>"));
		PlayURL( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("streamurl") ) ], http_request->HTTP_SOCKET );
		printf_P( PSTR("</pre></p>"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("config") ) )
	{	
		printf_P( PSTR("kommt noch!"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("replay") ) )
	{	
		printf_P( PSTR("<pre><p>"));
		RePlayURL( http_request->HTTP_SOCKET );
		printf_P( PSTR("</pre></p>"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("stop") ) )
	{	
		printf_P( PSTR("<pre><p>"));
		StopPlay();
		printf_P( PSTR("</pre></p>"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("info") ) )
	{
		printf_P( PSTR("<pre><p>"));
		mp3clientPrintInfo ( http_request->HTTP_SOCKET , HTML );
		printf_P( PSTR("</pre></p>"));
	}
	printf_P( PSTR(		"</BODY>\r\n"
						"</HTML>\r\n"
						"\r\n"));
	
}

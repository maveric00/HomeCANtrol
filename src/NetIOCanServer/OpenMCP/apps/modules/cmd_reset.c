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
#include "system/softreset/softreset.h"
#include "system/stdout/stdout.h"
#include "system/net/tcp.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_reset.h"

void init_cmd_reset( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_reset, PSTR("reset"));
#endif
#if defined(HTTPSERVER_RESET)
	cgi_RegisterCGI( cgi_reset, PSTR("reset.cgi"));
#endif
}

int cmd_reset( int argc, char ** argv )
{
	softreset();
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Resetinterface.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_reset( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	/*
	printf_P( PSTR(		"<HTML>"
						"<HEAD>"
						"<TITLE>Reset</TITLE>"
						"</HEAD>"
						"<BODY>"));
*/
	cgi_PrintHttpheaderStart();

	if ( http_request->argc == 0 )
	{
		printf_P( PSTR(	"<form action=\"reset.cgi\" method=\"get\" accept-charset=\"ISO-8859-1\">"
						"<p><input type=\"submit\" name=\"reset\" value=\"reset\"></p></form>"
						"</BODY>"
						"</HTML>"
						"\r\n\r\n"));
	}
	else if( PharseCheckName_P( http_request, PSTR("reset") ) )
	{	
		if ( !strcmp_P( http_request->argvalue[ PharseGetValue_P( http_request, PSTR("reset") ) ] , PSTR("reset") ) )
		{
			printf_P( PSTR(	"Reset</BODY>"
							"</HTML>"
							"\r\n\r\n"));
			STDOUT_Flush();
			CloseTCPSocket( http_request->HTTP_SOCKET );
			softreset();
			while(1);
		}
	}
}

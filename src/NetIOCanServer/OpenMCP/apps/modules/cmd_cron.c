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
#include "system/stdout/stdout.h"
#include "system/net/tcp.h"

#include "config.h"
#include "apps/cron/cron.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_cron.h"

void init_cmd_cron( void )
{
#if defined(HTTPSERVER_CRON)
	cgi_RegisterCGI( cgi_cron, PSTR("cron.cgi"));
#endif
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Anzeigen der Konfigurationsdaten im EEProm. 
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_cron( void * pStruct )
{
	int i;
	char string[32];
	int HH, MM;
	
	
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
		
/*	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<TITLE>Cron</TITLE>"
					"</HEAD>\r"
					"<BODY>"));*/

	cgi_PrintHttpheaderStart();

	if ( http_request->argc == 0 )
	{
		printf_P( PSTR(	"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">" ));
		for( i = 0 ; i < MAX_CRON ; i++ )
		{
			if ( CRON_getentry( string, i) == 1 )
			{
				printf_P( PSTR(	"<tr>"
					   			"<td align=\"left\">%s</td>"
					   			"<td align=\"left\">"
								"<a href=\"cron.cgi?del=%d\" style=\"text-decoration:none\" title=\"Del\"><input type=\"button\" value=\"del\" class=\"actionBtn\"></a>"
								"</td>"
  								"</tr>"), string, i );
			}
		}

		printf_P( PSTR(	"<tr>"
			   			"<td align=\"left\">"
						"<a href=\"cron.cgi?addnew\" style=\"text-decoration:none\" title=\"Add\"><input type=\"button\" value=\"add new cron\" class=\"actionBtn\"></a>"
						"</td>"
						"</tr>"
						"</table>\r") );

	}
	else
	{
		if ( PharseCheckName_P( http_request, PSTR("addnew") ) )
		{
			printf_P( PSTR(	"<form action=\"cron.cgi\">"
					   		"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">"
							"<tr>"
						   	"<td align=\"right\">Stunde</td><td><input name=\"HH\" type=\"text\" size=\"3\" value=\"0\"maxlength=\"3\"></td>"
  							"</tr>"
							"<tr>"
						   	"<td align=\"right\">Minute</td>"
							"<td><input name=\"MM\" type=\"text\" size=\"3\" value=\"0\" maxlength=\"3\"></td>"
  							"</tr>"
							"<tr>"
						   	"<td align=\"right\">Command</td>"
							"<td><input name=\"CMD\" type=\"text\" size=\"30\" maxlength=\"30\"></td>"
  							"</tr>"
 							"<tr>"
   							"<td></td><td><input type=\"submit\" value=\" Eintrag sichern \"></td>"
  							"</tr>"
						   	"</table>"
							"</form>\r" ) );
		}
		else if ( PharseCheckName_P( http_request, PSTR("CMD") ) )
		{
			HH = atoi( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("HH") ) ] );
			MM = atoi( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("MM") ) ] );
			sprintf_P( string, PSTR("%d %d \"%s\""), HH, MM, http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("CMD") ) ] );
			CRON_addentry( string );
			CRON_reloadcrontable();
			printf_P( PSTR("Einstellungen uebernommen!\r\n"));

		}
		else if ( PharseCheckName_P( http_request, PSTR("del") ) )
		{
			CRON_delentry( atoi( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("del") ) ] ) );
			CRON_reloadcrontable();
		}
	}

	cgi_PrintHttpheaderEnd();

/*	printf_P( PSTR( " </BODY>"
					"</HTML>\r\n"
					"\r\n")); */

}

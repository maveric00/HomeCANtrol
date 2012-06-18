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

#if defined(TWITTER)

#include "system/softreset/softreset.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"
#include "system/net/tcp.h"
#include "system/net/twitter.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_twitter.h"

void init_cmd_twitter( void )
{
	#if defined(HTTPSERVER_TWITTER)
		cgi_RegisterCGI( cgi_twitter, PSTR("twitter.cgi"));
	#endif
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface zum versenden eines tweety auf twitter.com.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_twitter( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;

	printf_P( PSTR(		"<HTML>\r\n"
						"<HEAD>\r\n"
						"<TITLE>Twitter</TITLE>\r\n"
						"</HEAD>\r\n"
						" <BODY>\r\n"));

	// Wenn keine Variablen übergeben wurden, dann Config ausgeben,
	// wenn Variablen übergeben wurden Config ändern
	if ( http_request->argc == 0 )
	{
		printf_P( PSTR(	"<form action=\"twitter.cgi\">\r"
	               		"<script type=\"text/javascript\">\r\n"
		               	"function textCounter(field, countfield, maxlimit)\r\n"
						"{\r\n"
						"	if (field.value.length > maxlimit)\r\n"
						"		field.value = field.value.substring(0, maxlimit);\r\n"
						"	else\r\n"
						"		countfield.value = maxlimit - field.value.length;\r\n"
						"}\r\n"
		               	"</script>\r\n"
		               	" <table>\r"
		               	"  <tr>"
		               	"   <td valign=\"top\">Tweety-Text:</td>"
						"   <td><textarea cols=20 rows=6 style=\"WIDTH: 250px\" name=\"TWITTERMSG\" wrap=physical maxLength=\"140\" onChange=\"textCounter(this.form.TWITTERMSG,this.form.remLen,140);\" onFocus=\"textCounter(this.form.TWITTERMSG,this.form.remLen,140);\" onKeyDown=\"textCounter(this.form.TWITTERMSG,this.form.remLen,140);\" onKeyUp=\"textCounter(this.form.TWITTERMSG,this.form.remLen,140);\"></textarea>"
						"   </td>" 
						"  </tr>" 
						"  <tr>" 
						"   <td>Sie haben:</td>" 
						"   <td valign=absmiddle><input readonly type=text name=\"remLen\" size=3 maxlength=3 value=140> Zeichen &uuml;brig</td>" 
						"  </tr>" 
 						"  <tr>\r"
   						"   <td colspan=\"2\" align=\"right\"><input type=\"submit\" value=\"Tweety senden\"><br>"
  						"  </tr>\r" 
					   	" </table>\r"
		                "<a href=\"http://www.twitter.com/openmcp_foo\" target=\"_blank\">OpenMCP_foo on Twitter.</a><br>"
		               	"<a href=\"twitter.cgi?config\">Richtigen Account eingestellen.	</a>"
						"</form>" ) );
	}
	else
	{
		if ( PharseCheckName_P( http_request, PSTR("config") ) )
		{
			printf_P( PSTR( "<form action=\"twitter.cgi\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    	"<p>Accountdaten ( user:pw ) :<input name=\"userpw\" size=\"50\"></p>"
					    	"<p><input type=\"submit\" value=\"Account speichern\"></p></form>" ));
		}
		else if ( PharseCheckName_P( http_request, PSTR("userpw") ) )
		{
			changeConfig_P( PSTR("&TWITTERPW"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("userpw") ) ] );
			printf_P( PSTR("Einstellungen uebernommen!\r\n"));
		}
		else if ( PharseCheckName_P( http_request, PSTR("TWITTERMSG") ) )
		{
			char userpw[32];
			if ( readConfig_P( PSTR("&TWITTERPW"), userpw ) == -1 )
				printf_P( PSTR("Keine Accountdaten hinterlegt!"));
			else
			{
				TWITTER_sendtweet( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("TWITTERMSG") ) ] , userpw );
				printf_P( PSTR("Tweet wurde versucht zu versenden!\r\n"));
			}
		}
	}
	
	printf_P( PSTR( " </BODY>\r\n"
					"</HTML>\r\n"
					"\r\n"));

}
#endif

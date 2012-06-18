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

#include "system/net/arp.h"
#include "system/net/ip.h"
#include "system/net/dns.h"
#include "system/net/dyndns.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"

#include "config.h"
#include "apps/cron/cron.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_dyndns.h"

const char configname_account[] PROGMEM = "&DYNDNS_USERPW";
const char configname_domain[] PROGMEM = "DYNDNS_DOMAIN";

void init_cmd_dyndns( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_dyndns, PSTR("dyndns"));
#endif
#if defined(HTTPSERVER_DYNDNS)
	cgi_RegisterCGI( cgi_dyndns, PSTR("dyndns.cgi"));
#endif
}

int cmd_dyndns( int argc, char ** argv )
{
	int ERROR = -1;
	char * account = NULL;
	char * domain = NULL;
	int i;
	char accountdata[ 32 ];
	char domaindata[ 32 ];

	
	if ( argc == 2 )
	{
		if ( !strcmp_P( argv[1], PSTR("update") ) )
		{
			readConfig_P( configname_account, accountdata );
			readConfig_P( configname_domain, domaindata );
			if ( DYNDNS_updateIP( accountdata, domaindata ) == DYNDNS_OK )	
				printf_P( PSTR("Update fuer %s erfolgreich.\r\n"), domaindata );
			else
				printf_P( PSTR("Update fuer %s fehlgeschlagen\r\n"), domaindata );
			ERROR = 0; 
		}
	}
	
	if ( argc == 3 )
	{
		if ( !strcmp_P( argv[1], PSTR("setaccount") ) )
		{
			account = &argv[2][0];
			for( i = 0 ; i < strlen( argv[2] ); i++ )
			{				
				if ( argv[2][i] == '@' )
				{
					domain = &argv[2][i+1];
					argv[2][i] = '\0';
					changeConfig_P( configname_account, account );
					changeConfig_P( configname_domain, domain );
					printf_P( PSTR("Accountdaten gespeichert.\r\n") );
					ERROR = 0 ;
					break;
				}
			}
		}
	}

	if ( ERROR == -1 )
		printf_P( PSTR("dyndns <update> / <setaccount user:pw@dyndns-domain>\r\n"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Einstellen und Anzeigen für den NTP-Service.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_dyndns( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char DYNDNSON[10];
	char DYNDNSDOMAIN[32];
	int i;

	printf_P( PSTR(		"<HTML>\r\n"
						"<HEAD>\r\n"
						"<TITLE>dynDNS</TITLE>\r\n"
						"</HEAD>\r\n"
						" <BODY>\r\n"));

	
	if ( http_request->argc == 0 )
	{
		if( checkConfigName_P( PSTR("DYNDNS_DOMAIN") ) != -1 )
			readConfig_P ( PSTR("DYNDNS_DOMAIN"), DYNDNSDOMAIN );
		else
			DYNDNSDOMAIN[0] = '\0';

		readConfig_P( PSTR("DYNDNS_ON"), DYNDNSON );
		if ( !strcmp_P( DYNDNSON, PSTR("on") ) )
			sprintf_P( DYNDNSON, PSTR("checked"));

		printf_P( PSTR(	"<form action=\"dyndns.cgi\">\r"
					   	" <table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">"
  						"  <tr>\r"
					   	"   <td align=\"right\">DynDNS aktivieren</td>"
					    "   <td><input type=\"checkbox\" name=\"DYNDNS\" value=\"on\" %s >"
						"  </td>\r"
						"  <tr>\r"
					   	"   <td align=\"right\">dynDNS Account ( user:pw )</td>"
  						"   <td><input name=\"ACCOUNT\" type=\"text\" size=\"31\" value=\"\" maxlength=\"31\"></td>"
  						"  </tr>\r"
						"  <tr>\r"
					   	"   <td align=\"right\">DynDNS Domain</td>"
						"   <td><input name=\"DOMAIN\" type=\"text\" size=\"31\" value=\"%s\" maxlength=\"31\"></td>"
  						"  </tr>\r"
 						"  <tr>\r"
   						"   <td></td><td><input type=\"submit\" value=\" Einstellung &Uuml;bernehmen \"></td>"
  						"  </tr>\r"
					   	" </table>\r"
						"</form>" ) , DYNDNSON, DYNDNSDOMAIN );
	}
	else
	{
		if ( PharseCheckName_P( http_request, PSTR("ACCOUNT") ) )
		{
			if ( strlen( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("ACCOUNT") ) ] ) != 0 )
				changeConfig_P( PSTR("&DYNDNS_USERPW"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("ACCOUNT") ) ] );
		}
		if ( PharseCheckName_P( http_request, PSTR("DOMAIN") ) )
		{
			changeConfig_P( PSTR("DYNDNS_DOMAIN"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("DOMAIN") ) ] );
		}
		
		if ( PharseCheckName_P( http_request, PSTR("DYNDNS") ) )
		{
			changeConfig_P( PSTR("DYNDNS_ON"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("DYNDNS") ) ] );
			strcpy_P( DYNDNSDOMAIN, PSTR("-1 -15 \"dyndns update\""));
			i = CRON_findentry( DYNDNSDOMAIN );
			if ( i == -1 )
				CRON_addentry( DYNDNSDOMAIN );
		}
		else
		{
			changeConfig_P( PSTR("DYNDNS_ON"), "off" );	
			strcpy_P( DYNDNSDOMAIN, PSTR("-1 -15 \"dyndns update\""));
			i = CRON_findentry( DYNDNSDOMAIN );
			if ( i != -1 )
				CRON_delentry( i );
		}
		CRON_reloadcrontable( );
		printf_P( PSTR("Einstellungen uebernommen!\r\n"));
	}
	
	printf_P( PSTR( " </BODY>\r\n"
					"</HTML>\r\n"
					"\r\n"));

}


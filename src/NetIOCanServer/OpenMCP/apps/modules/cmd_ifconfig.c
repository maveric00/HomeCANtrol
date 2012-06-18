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
#include "system/net/ethernet.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"

#include "config.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_ifconfig.h"

void init_cmd_ifconfig( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_ifconfig, PSTR("ifconfig"));
#endif
#if defined(HTTPSERVER_NETCONFIG)
	cgi_RegisterCGI( cgi_network, PSTR("network.cgi"));
#endif
}

int cmd_ifconfig( int argc, char ** argv )
{
	unsigned char IP[16];
		
	printf_P( PSTR(	"IP:        %s\r\n") , iptostr ( myIP, IP ) );		
	printf_P( PSTR(	"Netmask:   %s\r\n") , 		iptostr ( Netmask, IP ) );
	printf_P( PSTR(	"Gateway:   %s\r\n") , iptostr ( Gateway, IP ) );
	#ifdef DNS
		printf_P( PSTR(	"DNS:       %s\r\n") , iptostr ( DNSserver, IP ) );
	#endif
	printf_P( PSTR( "\r\nHW-Adr: %02x:%02x:%02x:%02x:%02x:%02x\r\n\r\n"),mymac[0],mymac[1],mymac[2],mymac[3],mymac[4],mymac[5]);	
}

#ifdef HTTPSERVER_NETCONFIG
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Einstellen und Anzeigen der Netzwerkeinstellungen.
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_network( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	unsigned char IP[32];

/*	printf_P( PSTR(		"<HTML>\r\n"
						"<HEAD>\r\n"
						"<TITLE>network</TITLE>\r\n"
						"</HEAD>\r\n"
						" <BODY>\r\n"));
*/
	cgi_PrintHttpheaderStart();

	if ( http_request->argc == 0 )
	{
					   
		printf_P( PSTR( "<pre>\r\n") );
#ifdef TELNETSERVER
		TELNET_runcmdextern_P( PSTR("ifconfig") );
		TELNET_runcmdextern_P( PSTR("stats") );
#endif		
		printf_P( PSTR( "</pre>\r\n") );
	}
	else if( PharseCheckName_P( http_request, PSTR("config") ) )
	{	
		
		printf_P( PSTR(	"<form action=\"network.cgi\">\r"
					   	" <table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">" ));
#ifdef DHCP
		readConfig_P( PSTR("DHCP"), IP );
  		printf_P( PSTR( "  <tr>\r"
					   	"   <td align=\"right\">DHCP aktivieren</td>"
					    "   <td><input type=\"checkbox\" name=\"DHCP\" value=\"on\" " )); 
		if ( !strcmp_P( IP, PSTR("on") ) )
						printf_P( PSTR("checked"));
		printf_P( PSTR(	"></td>\r"
  						"  </tr>\r") );
#endif
		if ( readConfig_P ( PSTR("IP"), IP ) != 1 )
			iptostr ( myIP, IP );
  		printf_P( PSTR(	"  <tr>\r"
					   	"   <td align=\"right\">IP-Adresse</td>"
  						"   <td><input name=\"IP\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"15\"></td>"
  						"  </tr>\r") , IP );
		if ( readConfig_P ( PSTR("MASK"), IP ) != 1 )
			iptostr ( Netmask, IP );		
  		printf_P( PSTR(	"  <tr>\r"
					   	"   <td align=\"right\">Netzwerkmaske</td>"
  						"   <td><input name=\"MASK\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"15\"></td>"
  						"  </tr>\r") , IP );
		if ( readConfig_P ( PSTR("GATE"), IP ) != 1 )
			iptostr ( Gateway, IP );		
  		printf_P( PSTR(	"  <tr>\r"
					   	"   <td align=\"right\">Gateway</td>"
						"   <td><input name=\"GATE\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"15\"></td>"
  						"  </tr>\r") , IP );
#ifdef DNS
		if ( readConfig_P ( PSTR("DNS"), IP ) != 1 )
			iptostr ( DNSserver, IP );		
		printf_P( PSTR(	"  <tr>\r"
					   	"   <td align=\"right\">DNS-Server</td>"
						"   <td><input name=\"DNS\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"15\"></td>"
  						"  </tr>\r" ), IP );
#endif
		readConfig_P( PSTR("ETH_FLDX"), IP );
  		printf_P( PSTR( "  <tr>\r"
					   	"   <td align=\"right\">Fullduplex aktivieren</td>"
					    "   <td><input type=\"checkbox\" name=\"ETH_FLDX\" value=\"on\" " )); 
		if ( !strcmp_P( IP, PSTR("on") ) )
						printf_P( PSTR("checked"));
		printf_P( PSTR(	"></td>\r"
  						"  </tr>\r") );

		if ( readConfig_P ( PSTR("MAC"), IP ) != 1 )
			mactostr( mymac, IP );
  		printf_P( PSTR(	"  <tr>\r"
					   	"   <td align=\"right\">Hardware-MAC</td>"
						"   <td><input name=\"MAC\" type=\"text\" size=\"17\" value=\"%s\" maxlength=\"17\"></td>"
  						"  </tr>\r"
		  				"  <tr>\r"
   						"   <td></td><td><input type=\"submit\" value=\" Einstellung &Uuml;bernehmen \"></td>"
  						"  </tr>\r"
					   	" </table>\r"
						"</form>") , IP );
	}
	else
	{
#ifdef DHCP
		if ( PharseCheckName_P( http_request, PSTR("DHCP") ) )
			changeConfig_P( PSTR("DHCP"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("DHCP") ) ] );
		else
			changeConfig_P( PSTR("DHCP"), "off" );
#endif
		if ( PharseCheckName_P( http_request, PSTR("IP") ) )
			changeConfig_P( PSTR("IP"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("IP") ) ] );
		if ( PharseCheckName_P( http_request, PSTR("MASK") ) )
			changeConfig_P( PSTR("MASK"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("MASK") ) ] );
		if ( PharseCheckName_P( http_request, PSTR("GATE") ) )
			changeConfig_P( PSTR("GATE"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("GATE") ) ] );
		#ifdef DNS
		if ( PharseCheckName_P( http_request, PSTR("DNS") ) )
			changeConfig_P( PSTR("DNS"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("DNS") ) ] );
		#endif
		if ( PharseCheckName_P( http_request, PSTR("MAC") ) )
			changeConfig_P( PSTR("MAC"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("MAC") ) ] );
		if ( PharseCheckName_P( http_request, PSTR("ETH_FLDX") ) )
			changeConfig_P( PSTR("ETH_FLDX"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("ETH_FLDX") ) ] );
		else
			changeConfig_P( PSTR("ETH_FLDX"), "off" );
		printf_P( PSTR("Einstellungen uebernommen!\r\n"));
	}
	
	cgi_PrintHttpheaderEnd();
	
/*	printf_P( PSTR( " </BODY>\r\n"
					"</HTML>\r\n"
					"\r\n"), IP);
*/
}
#endif

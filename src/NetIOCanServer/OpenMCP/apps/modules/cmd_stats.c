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

#include "hardware/network/enc28j60.h"
#include "system/net/ethernet.h"
#include "system/net/tcp.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"
#include "system/clock/clock.h"

#include "config.h"
#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "cmd_stats.h"

void init_cmd_stats( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_stats, PSTR("stats"));
#endif
#if defined(HTTPSERVER_STATS)
	cgi_RegisterCGI( cgi_stats, PSTR("stats.cgi"));
#endif
}

int cmd_stats( int argc, char ** argv )
{
	unsigned int REVID,_PHSTAT1,_PHSTAT2;

	// buffer mit ausgabe vorbereiten & ausgeben
	printf_P( PSTR("Ethernet: %ld Bytes in %ld Packeten\r\n") , ByteCounter, PacketCounter );

	#ifdef TCP_with_unsortseq
		printf_P( PSTR("TCP-TX Errors: %d TCP-RX Errors: %d (unsorted %d, oldseq %d)\r\n\r\n"), TXErrorCounter , RXErrorCounter, RXErrorUnsort, RXErrorOldSeq );
	#else
		printf_P( PSTR("TCP-TX Errors: %d TCP-RX Errors: %d (oldseq %d)\r\n\r\n"), TXErrorCounter , RXErrorCounter, RXErrorOldSeq );
	#endif
						
	LockEthernet();
		
	REVID = enc28j60Read( EREVID );
	_PHSTAT1 = enc28j60PhyRead( PHSTAT1 );
	_PHSTAT2 = enc28j60PhyRead( PHSTAT2 );
					
	FreeEthernet();
					
	printf_P( PSTR("ENC28J60:\r\n"));
	printf_P( PSTR("Reg.    | Wert\r\n"));
	printf_P( PSTR("--------+--------\r\n"));
	printf_P( PSTR("Rev.ID  |   0x%02X\r\n") , REVID );
	printf_P( PSTR("PHSTAT1 | 0x%04X\r\n") , _PHSTAT1 );
	printf_P( PSTR("PHSTAT2 | 0x%04X\r\n") , _PHSTAT2 );

}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface um Stats anzuzeigen (Uhrzeit, Tranfervolumen etc.).
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_stats( void * pStruct )
{	
	
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	static int VisitCounter=0;

	long time;
	
	struct TIME Time;
	// Zeit holen
	CLOCK_GetTime ( &Time );
	
	VisitCounter++;
			

	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<meta http-equiv=\"refresh\" content=\"600; \">"
					"<TITLE>Streaming</TITLE>"
					"</HEAD>"
					"<BODY bgcolor=\"#228B22\" text=\"#FFFFFF\">"
					"Zeit: %02d:%02d:%02d.%02d ( %u/%u/%04u )"
					", Uptime: "), Time.hh, Time.mm, Time.ss, Time.ms , Time.DD, Time.MM, Time.YY);
	
	time = Time.uptime / ( 86400l );
	if ( time != 0 )
		if ( time == 1 ) printf_P(PSTR("%ld tag "), time );
		else printf_P(PSTR("%ld tage "), time );

	time = ( Time.uptime % ( 86400l ) ) / 3600;
	if ( time != 0 ) printf_P(PSTR("%ld std "), time );

	time = ( Time.uptime % 3600 ) / 60;
	if ( time != 0 ) printf_P(PSTR("%ld min "), time );

	time = Time.uptime % 60;
	if ( time != 0 ) printf_P(PSTR("%ld sek"), time );
	
	printf_P( PSTR(	", "
					"Ethernet:") );
	
	if ( ByteCounter < 1024 )
		printf_P( PSTR(" %ld Bytes "), ByteCounter);
	else if ( ByteCounter < ( 1024l*1024l ) )
		printf_P( PSTR(" %ld kBytes "), ByteCounter / 1024l );
	else if ( ByteCounter < (1024l*1024l*1024l) )
		printf_P( PSTR(" %ld.%03ld MBytes "), ByteCounter / ( 1024l * 1024l ), ( ByteCounter % ( 1024l * 1024l ) ) / 1024l  );
	else
		printf_P( PSTR(" %ld.%03ld GBytes "), ByteCounter / ( 1024l * 1024l *1024l ), ( ByteCounter % ( 1024l * 1024l * 1024l ) ) / ( 1024l * 1024l ) );
		
	printf_P( PSTR( "in %ld Packeten ; "
					"Du bist der %d. Besucher auf Socket %d."
					"</BODY>"
					"</HTML>"
					"\r\n\r\n"), PacketCounter, VisitCounter, http_request->HTTP_SOCKET );

}

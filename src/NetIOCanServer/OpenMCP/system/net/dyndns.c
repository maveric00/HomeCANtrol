/*! \file dyndns.c \brief Stellt Funktionen für dyndns.org bereit. */
/***************************************************************************
 *            dyndns.c
 *
 *  Sat Sep 19 14:57:17 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
//****************************************************************************/
///	\ingroup system
///	\defgroup DYNDNS Stellt Funktionen für dyndns.org bereit. (dyndns.c)
///	\code #include "dyndns.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen für das dyndns.org bereit.
//****************************************************************************/
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
//@{

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#include "dyndns.h"
#include "system/base64/base64.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"
#include "system/net/dns.h"
#include "system/net/ip.h"
#include "system/net/tcp.h"
#include "system/clock/clock.h"

#include "hardware/timer1/timer1.h"
 
const char DYNDNSFILE[] PROGMEM = "/nic/update?hostname=%s";
const char DYNDNSURL[] PROGMEM = "members.dyndns.org";
const char DYNDNSCHECKIPURL[] PROGMEM = "checkip.dyndns.org";
const int  DYNDNSPORT = 80;

const char DYNDNSHEADER_1[] PROGMEM =  "GET ";
const char DYNDNSHEADER_2[] PROGMEM =  " HTTP/1.0\r\nUser-Agent: Wget/1.11.4\r\nAccept: */*\r\nHost: ";
const char DYNDNSHEADER_3[] PROGMEM =  "\r\nAuthorization: Basic %s\r\n\r\n";

//****************************************************************************/
/*!\brief Updatet auf die aktuelle Public IP.
 * \param userpw		Zeiger auf den String der Username und Password enthÃ¤lt.
 * \param domain		Zeiger auf den String der den Domainname enthÃ¤lt.
 * \return				Erfolgreich oder nicht.
 * \retval 0			Erfolgreich gesendet.
 * \retval 1			Fehler beim senden.
 */
//****************************************************************************/
int DYNDNS_updateIP( char * userpw, char * domain )
{
	struct STDOUT oldstream;

	char userpw_b64[64];
	int ret = 0, SOCKET, Contentlenght, ResponseCode ;
	long IP, publicIP;

	// hole meine public IP
	publicIP = DYNDNS_getPublicIP();
	if ( publicIP == -1 ) return( DYNDNS_FAILED );
	
	// hole die IP der Domain
	IP = DNS_ResolveName( domain );
	if ( IP == -1 ) return( DYNDNS_FAILED );

	// ist public IP und IP gleich, dann ist kein Update nÃ¶tig
	if ( IP == publicIP ) return( DYNDNS_OK );
	
	// DYNDNSURL auflÃ¶sen
	IP = DNS_ResolveName_P( DYNDNSURL );
	if ( IP == -1 )
		return( DYNDNS_FAILED );
	
	// zu DYNDNS verbinden
	SOCKET = Connect2IP( IP, DYNDNSPORT );
	if ( SOCKET == SOCKET_ERROR )
		return( DYNDNS_FAILED );
	
	// STDOUT umbiegen auf die neue Verbingung und alt STDOUT sichern
	STDOUT_save( &oldstream );
	STDOUT_set( _TCP, SOCKET );

	// Username und Passwort encoden
	base64_encode( userpw_b64, sizeof( userpw_b64 ), userpw, strlen( userpw ) ); 
	
	// Request senden
	printf_P( DYNDNSHEADER_1 );
    printf_P( DYNDNSFILE, domain );
    printf_P( DYNDNSHEADER_2 );
	printf_P( DYNDNSURL );
    printf_P( DYNDNSHEADER_3, userpw_b64 );
	
	// Gesicherte STDOUT wieder herstellen
	STDOUT_restore( &oldstream );

	ResponseCode = DYNDNS_pharseHTTPheader( SOCKET, &Contentlenght );
		
	CloseTCPSocket( SOCKET );

	if ( ResponseCode != 200 ) return ( DYNDNS_FAILED );
	
	// und zurÃ¼ck
	return( DYNDNS_OK );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Liest den HTTP-header.
 * \param	socket			Der TCP-Socket von wo die HTTP-header lesen werden soll und speichert Infos zum Stream.
 * \param	Contentlenght	Zeiger auf eine Variable vom Typ int die die lÃ¤nge der noch folgenden Daten Speichert.
 * \return	REQUEST 		Der Returncode auf dem HTTP-header.
 */
/*------------------------------------------------------------------------------------------------------------*/
int DYNDNS_pharseHTTPheader ( unsigned int socket, int * Contentlenght )
{

	char pharsebuffer[32], Data;
	int HTTPtimer, pharsebufferpos=0;
	int REQUEST = 0;

	* Contentlenght = -1;
	
	HTTPtimer = CLOCK_RegisterCoundowntimer();
	
	if ( HTTPtimer == CLOCK_FAILED ) return( -1 );
	
	// Timeout fÃ¼r pufferauffÃ¼llen setzen
	CLOCK_SetCountdownTimer ( HTTPtimer, 1000, MSECOUND );

	while( 1 )
	{

		// sind Daten angekommen, oder Timeout schon erreicht ?
		while ( GetBytesInSocketData( socket ) == 0 && CLOCK_GetCountdownTimer( HTTPtimer ) != 0)
		{
			// Wenn Timeout erreicht, dann beenden
			if ( CLOCK_GetCountdownTimer( HTTPtimer ) == 0 )
			{
				CLOCK_ReleaseCountdownTimer( HTTPtimer );
				return( -1 );
			}
		}

		// Neue Daten lesen
		Data = GetByteFromSocketData ( socket );
		
		if ( CLOCK_GetCountdownTimer( HTTPtimer ) == 0 )
		{
			CLOCK_ReleaseCountdownTimer( HTTPtimer );
			return( -1 );
		}

		// Daten bis auf 0x0a durch lassen
		if ( Data != 0x0a )
		{
			// Passen die Daten noch in den Puffer ?
			if ( pharsebufferpos < sizeof( pharsebuffer ) - 1 )
			{
				if ( Data != 0x0d )
				{
					pharsebuffer[ pharsebufferpos++ ] = Data;
					pharsebuffer[ pharsebufferpos ] = '\0';
				}
			}

			// Zeilenende erreicht ? Wenn ja kieck mal was drinne ist
			if ( Data == 0x0d )
			{
				if ( !memcmp_P( &pharsebuffer[0] , PSTR("HTTP/1.") , 7 ) )
				{
					REQUEST = atoi( &pharsebuffer[9] );
				}
				else if ( !memcmp_P( &pharsebuffer[0] , PSTR("Content-Length: ") , 16 ) )
				{
					* Contentlenght = atoi( &pharsebuffer[16] );
				}								

				// War das eine Leerzeile? Wenn ja ist der HTTP-Header zu ende
				if ( pharsebuffer[0] == '\0' )
				{
					Data = GetByteFromSocketData ( socket );
					break;
				}
				else
				{
					pharsebufferpos = 0;
					pharsebuffer[0] = '\0';
				}
					
			}
		}
	}
	
	CLOCK_ReleaseCountdownTimer( HTTPtimer );

	return( REQUEST );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Public IP.
 * \return	IP Die Ã¶ffentlich IP-Adresse.
 */
/*------------------------------------------------------------------------------------------------------------*/
long DYNDNS_getPublicIP( void )
{
	struct STDOUT oldstream;

	char Data, IPstr[16];
	long IP;
	int Contentlenght, ResponseCode, SOCKET ;
		
	// DYNDNSURL auflÃ¶sen
	IP = DNS_ResolveName_P( DYNDNSCHECKIPURL );
	if ( IP == -1 )
		return( DYNDNS_FAILED );
		
	// zu DYNDNS verbinden
	SOCKET = Connect2IP( IP, DYNDNSPORT );
	if ( SOCKET == -1 )
		return( DYNDNS_FAILED );
	
	// STDOUT umbiegen auf die neue Verbingung und alt STDOUT sichern
	STDOUT_save( &oldstream );
	STDOUT_set( _TCP, SOCKET );
	
	// Request senden
	printf_P( DYNDNSHEADER_1 );
    printf_P( PSTR("/") );
    printf_P( DYNDNSHEADER_2 );
	printf_P( DYNDNSURL );
	printf_P( PSTR("\r\n\r\n"));
	
	// Gesicherte STDOUT wieder herstellen
	STDOUT_restore( &oldstream );

	// Antwort auswerten
	ResponseCode = DYNDNS_pharseHTTPheader( SOCKET, &Contentlenght );

	// Wenn Antwort nicht okay, Exit
	if ( ResponseCode != 200 )
	{
		CloseTCPSocket( SOCKET );
		return( DYNDNS_FAILED );
	}
	
	// Wenn antwort Okay, Die Zeichen die der Server senden mÃ¶chte auslesen und IP herrausfiltern
	while( Contentlenght != 0 )
	{
		Data = GetByteFromSocketData ( SOCKET );
		
		// Wenn ':' dann kommt die IP
		if( Data == ':' )
		{
			// Dummyread, da erstes Zeichen nach dem ':' ein ' ' ist und nicht zur IP gehÃ¶rt
			GetByteFromSocketData ( SOCKET );
			int i;

			// IP speichern
			for ( i = 0 ; i < 15 ; i++ )
			{
				Data = GetByteFromSocketData ( SOCKET );
				Contentlenght--;
				if ( Data == '<' ) break;
				IPstr[i] = Data;
				IPstr[i+1] = '\0';
			}
		}
		Contentlenght--;
	}
	
	// Verbindung beenden
	CloseTCPSocket( SOCKET );

	// String mit der IP umwandeln
	IP = strtoip( IPstr );

	return( IP );
}
	
//@}


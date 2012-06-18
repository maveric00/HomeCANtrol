/*! \file dns.c \brief Aufloesung von Hostnmen nach IP */
//***************************************************************************
//*            dns.c
//*
//*  Mon Aug 21 21:34:20 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup network
///	\defgroup DNS DNS-Funktionen (dns.c)
///	\code #include "dns.h" \endcode
///	\par Uebersicht
///		Loest die Hostnamen in eine IP-Adresse auf um weiter zu verarbeiten.
/// Als IP fuer den DNS-Server dienst DNSserver aus ip.c .
//****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
//@{
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"
#include "udp.h"
#include "dns.h"

#include "system/clock/clock.h"
#include "system/math/math.h"
#include "hardware/uart/uart.h"

/*! \brief Holt von einen Hostname die IP-Adressen
 * \warning Es ist drauf zu achten das genug Speicher vorgesehen ist fuer die Anworten und das auch Packete mit entsprechender groesse
 * vom Ethernetmodul empfagen werden koennen und nicht verworfen werden. Siehe MAX_FRAMELEN in enc28j60.h .
 * \param	HOSTNAME	Zeiger auf den Hostnamestring der mit 0 teminiert ist.
 * \retval	IP		Die IP des Hostname, wenn IP = DNS_NO_ANSWER ist war die Anfrage nicht erfolgreich. Es sollte der DNSserver Eintrag ueberprueft werden
 * oder die richtigkeit des Hostname.
 */
unsigned long DNS_ResolveName( char * HOSTNAME )
{		
		int i,UDP_socket;
		int timer;
				
		// udp-puffer anlegen
		unsigned char * udpbuffer;
		udpbuffer = (unsigned char*) __builtin_alloca (( size_t ) DNS_BUFFER_LENGHT );
			
		// DNS-struct in udp-puffer anlegen
		struct DNS_header * DNS_question;
		DNS_question = ( struct DNS_header *) udpbuffer;
			
		// DNS anfrage bauen
		DNS_question->TransactionID = 0x1acd;
		DNS_question->Flags = ChangeEndian16bit( 0x0100 );
		DNS_question->Questions = ChangeEndian16bit( 1 );
		DNS_question->Answer_RRs = 0;
		DNS_question->Authority_RRs = 0;
		DNS_question->Additional_RRs = 0;
		
		// Hostename für DNS umwandeln, in i steht die länge des neuen strings
		i = DNS_convertHostName( HOSTNAME, DNS_question->Queries );
		
		DNS_question->Queries[i + 1] = '\0';
		DNS_question->Queries[i + 2] = 1;
		DNS_question->Queries[i + 3] = '\0';
		DNS_question->Queries[i + 4] = 1;

		i = i + 5;
		
		// Antwortstruct anlegen
		struct DNS_answer * DNS_ans;
		DNS_ans = ( void * ) &DNS_question->Queries[i];
		
		// UDP-Paccket senden
		UDP_socket = UDP_RegisterSocket( DNSserver , DNS_SERVER_PORT, DNS_BUFFER_LENGHT , udpbuffer);
		if ( UDP_socket == UDP_SOCKET_ERROR )
				return( DNS_NO_ANSWER );
		UDP_SendPacket( UDP_socket, DNS_HEADER_LENGHT + i , udpbuffer);

		// empfang des der DNS-Atwort abwarten
		timer = CLOCK_RegisterCoundowntimer();
		if ( timer == CLOCK_FAILED ) return ( DNS_NO_ANSWER );

		CLOCK_SetCountdownTimer ( timer, DNS_REQUEST_TIMEOUT, MSECOUND );

		while ( 1 )
		{
			if ( UDP_GetSocketState( UDP_socket ) == UDP_SOCKET_BUSY )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				UDP_CloseSocket( UDP_socket );
				if ( ( ChangeEndian16bit( DNS_question->Flags ) & 0x000f ) != 0 ) return ( DNS_NO_ANSWER );
				break;
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				CLOCK_ReleaseCountdownTimer( timer );
				UDP_CloseSocket( UDP_socket );
				return( DNS_NO_ANSWER );
			}
		}

		// Antwortpacket auseinander nehmen
		while ( 1 )
		{
			// Wenn noch nicht der Hosteintrag dann nächsten DNS-Answer Datensatz
			if ( ChangeEndian16bit( DNS_ans->Type ) != A_HOSTNAME )
			{
				i = i + ChangeEndian16bit( DNS_ans->Datalenght ) + DNS_ANSWER_HEADER_LENGHT;
				DNS_ans = ( void * ) &DNS_question->Queries[i];
			}
			else break;
		}
		return( DNS_ans->Adress );		
}

unsigned long DNS_ResolveName_P( const char * HOSTNAME_P )
{
	char dnsbuffer[64];

	strcpy_P( dnsbuffer, HOSTNAME_P );
		
	return( DNS_ResolveName( dnsbuffer ) );
}

/*! \brief Bereitet den Hostnamestring fuer die Weiterverrbeitung vor.
 * \warning Es wird keine Ueberbruefung der Puffergroesen vorgenommen, also Achtung!
 * \param	HOSTNAME	Zeiger auf den Hostnamestring der mit 0 terminiert ist.
 * \param	Destbuffer	Zeiger auf den Zielspeicher in welchen der neue String abgelegt werden soll.
 * \retval	strlaenge	Die laenge des neuen Strings.
 */
unsigned int DNS_convertHostName( char * HOSTNAME , char * Destbuffer )
{
	int i,j=0,k;
	int strlaenge = strlen( HOSTNAME );
	k = strlaenge;
	
	for ( i = strlaenge - 1 ; i >= 0 ; i-- )
	{
		if ( HOSTNAME[i] == '.' )
		{
			Destbuffer[k] = j;
			k--;
			j = 0;
		}
		else
		{
			Destbuffer[k] = HOSTNAME[i];
			k--;
			j++;
		}
	}
	Destbuffer[k] = j;

	Destbuffer[ strlaenge + 1 ] = '\0';

	return( strlaenge + 1 );
}
//@}

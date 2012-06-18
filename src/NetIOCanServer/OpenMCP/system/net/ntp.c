/*! \file "ntp.c" \brief Die ntp-Funktionlitaet */
/***************************************************************************
 *            ntp.c
 *
 *  Mon Aug 28 11:36:49 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
///	\ingroup network
///	\defgroup NTP NTP-Funktionen (ntp.c)
///	\code #include "ip.h" \endcode
///	\code #include "dns.h" \endcode
///	\code #include "tcp.h" \endcode
///	\code #include "ntp.h" \endcode
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
#include <avr/interrupt.h>

#include "system/clock/clock.h"
#include "hardware/uart/uart.h"

#include "config.h"

#include "ip.h"
#include "udp.h"
#include "ntp.h"
#include "dns.h"

// #define NTP_DEBUG

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Zeit von einen NTP-Server.
 * \param 	IP				Die IP-Adresse von einen NTP-Server.
 * \param 	dnsbuffer		Der DNS-Name von einen NTP-Server.
 * \param 	timedif			Der Zeitunterschied, wichtig bei Sommer/Winterzeit.
 * \return	int
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int NTP_GetTime( unsigned long IP, unsigned char * dnsbuffer, long timedif )
	{
		unsigned char buffer[4];
		unsigned int i=0,SOCKET, timer, retval= NTP_ERROR;
		unsigned long Zeit,Std,Min,Sek ;
		char String[32];
		
		union DATE ZeitInSek;
		
		struct TIME time;
	
		if ( IP == 0 )
		{
#ifdef UDP
	#ifdef DNS
			if ( dnsbuffer != 0 )
			{
				// Host nach IP auflösen
				IP = DNS_ResolveName( dnsbuffer );
				// könnte er aufgelöst werden ?
				if ( IP == DNS_NO_ANSWER ) return( NTP_ERROR );
			}
			else
	#endif
#endif
				return( NTP_ERROR );
		}
		
		// UDP-socket aufmachen für Bootp
		SOCKET = UDP_RegisterSocket( IP , 37 , 4 , buffer);
		// Wenn Fehler aufgetretten, return
		if ( SOCKET == UDP_SOCKET_ERROR ) 
		{
			return ( NTP_ERROR );
		}

#if defined(NTP_DEBUG)
			printf_P( PSTR("UDP-Socket aufgemacht zur %s.\r\n"), iptostr( IP, String ) );
#endif
		// leeres UDP-Packet an Time-server senden
		UDP_SendPacket( SOCKET, 0 , buffer);

#if defined(NTP_DEBUG)
		printf_P( PSTR("UDP-Packet gesendet.\r\n"));
#endif
		// Timeout-counter reservieren und starten
		timer = CLOCK_RegisterCoundowntimer();
		if ( timer == CLOCK_FAILED ) return ( NTP_ERROR );

		CLOCK_SetCountdownTimer( timer , 500, MSECOUND );

#if defined(NTP_DEBUG)
		printf_P( PSTR("Warte auf Antwort."));
#endif
		// Auf Antwort des Timer-Servers warten
		while( 1 )
		{
			// Wenn Time-Server geantwortet hat inerhalb des Timeouts, hier weiter
			if ( UDP_GetSocketState( SOCKET ) == UDP_SOCKET_BUSY && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				// Sind 4 Bytes empfangen worden, wenn ja okay, sonst fehler
				if ( UDP_GetByteInBuffer( SOCKET ) >= 4 )
				{				
					// Daten kopieren und Zeit ausrechnen
					for ( i = 0 ; i < 4 ; i++ ) 
						ZeitInSek.DateByte[ i ] = buffer[ 3 - i ];

					CLOCK_decode_time( ZeitInSek.Date + ( timedif * 3600 ) );
					retval = NTP_OK;
#if defined(NTP_DEBUG)
					printf_P( PSTR("Antwort erhalten.\r\n"));
#endif
					// fertisch
					break;
				}
				else
				{
#if defined(NTP_DEBUG)
					printf_P( PSTR("Falsches Format der Antwort.\r\n"));
#endif
					retval = NTP_ERROR;
					break;
				}
			}
			// Timeout erreicht ? Wenn ja Fehler.
			if ( CLOCK_GetCountdownTimer( timer ) == 0 )
			{
#if defined(NTP_DEBUG)
				printf_P( PSTR("Timeout beun warten auf Antwort.\r\n"));
#endif
				retval = NTP_ERROR;
				break;
			}
		}
		// timer freigeben und UDP-Socket schliessen
		CLOCK_ReleaseCountdownTimer( timer );
		UDP_CloseSocket( SOCKET );
#if defined(NTP_DEBUG)
		printf_P( PSTR("UDP-Socket geschlossen.\r\n"));
#endif
		return( retval );
	}
//@}

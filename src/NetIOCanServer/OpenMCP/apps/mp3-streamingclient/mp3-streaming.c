/*! \file mp3-streaming.c \brief MP3-Streamingengine zum streamen von MP3 */
//***************************************************************************
//*            mp3-streaming.c
//*
//*  Sat May  10 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup mp3stream Tsumani MP3-Streamingengine (mp3-streaming.c)
///	\code #include "mp3-streaming.h" \endcode
///	\par Uebersicht
///
/// 	Die Tsunami MP3-streamingengine zum streamen von mp3 über das netzwerk.
/// 	Die Engine kümmert sich um das Puffern des MP3-Stream der von einem Shoutcast-
/// 	Streamingserver gesendet wird.
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
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "config.h"

#if defined(HTTPSERVER_STREAM)

#include "mp3-streaming.h"
#include "mp3-clientserver.h"

#if defined(VS10XX)
	#include "hardware/vs10xx/vs10xx.h"
#else
	#error "Bitte unterstützung für den VS10xx in der config.h aktivieren."
#endif

#if defined(LED)
	#include "hardware/led/led_core.h"
#else
	#error "Bitte LED in der config.h aktivieren."
#endif
#include "hardware/timer0/timer0.h"

#include "system/buffer/fifo.h"
#include "system/clock/clock.h"
#include "system/math/math.h"
#include "system/net/tcp.h"
#include "system/net/dns.h"
#include "system/net/ethernet.h"
#include "system/stdout/stdout.h"

volatile int MP3_SOCKET = SOCKET_ERROR;
volatile unsigned char MP3_FIFO = 0;
volatile unsigned long totalbytes = 0;
volatile unsigned int vol = 255;
volatile int mp3timer = CLOCK_FAILED;

volatile unsigned long metaintcounter = 0;
volatile unsigned char metainfo = 1;
volatile unsigned int metainfolen = 0;
volatile unsigned int metaint = 0;

volatile unsigned char verboselevel = 1;
volatile unsigned char slowstart = 1;
unsigned char mp3_buffer[ mp3_buffer_size ];

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die streamingengine.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3client_init( void )
{
	MP3_SOCKET = SOCKET_ERROR;
	mp3timer = CLOCK_RegisterCoundowntimer();
	if ( mp3timer == CLOCK_FAILED ) return;
	
	MP3_FIFO = Get_FIFO ( mp3_buffer, mp3_buffer_size );
	
	timer0_init( buffercleaninterval );
	timer0_RegisterCallbackFunction( mp3client_stream );

	printf_P( PSTR("Tsumani v0.1.3 MP3-Streamingengine gestartet \r\n$Id: mp3-streaming.c 219 2009-12-13 13:00:57Z sharan $.\r\n"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schick die MP3-Daten zum VS10xx
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3client_stream( void )
{
	static unsigned int temp;
	char sreg_tmp;

		
	if ( MP3_SOCKET == SOCKET_ERROR ) return;
	
	timer0_stop();

	// Wenn slowstart auf 0 steht Decoder nicht befüttern, sonst warten bis 1/2 voll
	if ( slowstart == 0 )
		VS10xx_flush_from_FIFO( MP3_FIFO );
	else
		if ( Get_Bytes_in_FIFO ( MP3_FIFO ) > ( mp3_buffer_size / 8 ) * 4 ) slowstart = 0;
	
	// Verbindung wirklich noch offen
	if( CheckSocketState( MP3_SOCKET ) == SOCKET_NOT_USE )
	{
		CloseTCPSocket( MP3_SOCKET );
		MP3_SOCKET = SOCKET_ERROR;
	}
	else
	{
		LockEthernet();

		sreg_tmp = SREG;
		sei();
		
		// Datenpuffer auffrischen mit neuen daten von TCP-Verbindung
		while ( GetBytesInSocketData( MP3_SOCKET ) >= MP3TCPtranslen && Get_FIFOrestsize ( MP3_FIFO ) > MP3TCPtranslen && metainfolen == 0 )
		{
			// Wenn die transferlänge zum nächsten Metaint kleiner als translen zwischen tcp und mp3 buffer ist, nur diese
			// kopieren damit danach der metaint eingelesen werden kann
			LED_on(1);

			if ( metaint - ( metaintcounter%metaint ) < MP3TCPtranslen && metaint != 0)
			{
				temp = GetSocketDataToFIFO ( MP3_SOCKET, MP3_FIFO, metaint - ( metaintcounter%metaint ) );
			}
			else
			{	
				temp = GetSocketDataToFIFO ( MP3_SOCKET, MP3_FIFO, MP3TCPtranslen );
			}

			totalbytes = totalbytes + temp;
			metaintcounter = metaintcounter + temp;

			LED_off(1);

			if ( metaintcounter%metaint == 0 && metaint != 0 )
			{
				// checkt und wartet ob mehr als 1 Byte im Puffer ist
				while ( GetBytesInSocketData( MP3_SOCKET ) <= 0 );

				// lese Byte und errechne die metalänge
				metainfolen = GetByteFromSocketData ( MP3_SOCKET ) * 16 ;
				
				break;
			}
	
		}
		
		SREG = sreg_tmp;

		FreeEthernet();
	}
	
	timer0_free();

}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die MP3-streamingengine, sie sollte in einer Mainloop in regelmässigen Abständen aufgerufen werden,
 * wobei die Intervalle nicht zu knapp gewählt werden sollten, damit sie noch richtig arbeitet.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3client_thread( void )
{
	unsigned char i;
	static unsigned char y;	
	static unsigned int temp;
	static unsigned long lastbytes=0;
	static unsigned long bandwidth;
	struct STDOUT oldSTDOUT;
	
	timer0_stop();
	
	STDOUT_save( &oldSTDOUT );
	STDOUT_set( RS232, 0 );
	
	if ( MP3_SOCKET == SOCKET_ERROR )
	{ 	
		// keine Verbindung offen? Trafficcounter zurücksetzen
		y=0;
		metaintcounter=0;
		slowstart = 1;
	}
	else
	{	
		// Verbindung wirklich noch offen
		if( CheckSocketState( MP3_SOCKET ) == SOCKET_NOT_USE )
		{
			CloseTCPSocket( MP3_SOCKET );
			MP3_SOCKET = SOCKET_ERROR;
		}
		else
		{		
			// Wenn Puffer auf unter 1/8 gelaufen ist -> slowstart setzen 
			if ( Get_Bytes_in_FIFO ( MP3_FIFO ) < mp3_buffer_size / 8 ) slowstart = 1;			
					
			// Wenn der metaintcounter auf einen metaint steht, den metaint lesen und ausgeben
			if ( metainfolen != 0 )
			{						
				mp3client_readmetainfo( MP3_SOCKET, metainfolen );
				metainfolen = 0;
			}

			// Counter schon bei 0, wenn ja stats updaten
			if ( CLOCK_GetCountdownTimer ( mp3timer ) == 0 )
			{			
				bandwidth = ( ( bandwidth * 3l ) + ( totalbytes - lastbytes )) / 4l ;
				lastbytes = totalbytes;

				// stats sichern
				mp3clientupdate( bandwidth , GetBytesInSocketData( MP3_SOCKET ) , Get_Bytes_in_FIFO ( MP3_FIFO ), VS10xx_get_decodetime( ), verboselevel );	

				// wenn verboselevel gesetzt ausgeben
				if ( verboselevel != 0)
					printf_P( PSTR("\r%5d(TCP %4d)Bytes on FIFO, bandwidth:%3ldkbit/s(%2ldkb/s)"), Get_Bytes_in_FIFO ( MP3_FIFO ), GetBytesInSocketData( MP3_SOCKET ), (bandwidth * 8 ) / 1000 , bandwidth / 1024 );	
				
				// Counter neu starten 1 Sekunde
				CLOCK_SetCountdownTimer( mp3timer, 100, MSECOUND );
			} 
		}
	}
	
	STDOUT_restore( &oldSTDOUT );

	timer0_free();
	return;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Diese Funktion dient zum Starten des MP3-Stream, damit diese dem stream dann im mp3-client_thread abarbeiten
 * kann.
 * \param 	streamingPROTO	Pointer auf einen String für das Protokoll, Standard ist http.
 * \param	streamingURL	Pointer auf den DNS-namen oder die IP als String.
 * \param	streamingIP		Die IP-Adresse des Streamingserver(shoutcastserver).
 * \param	streamingPORT	Der TCP-Port auf dem der Streamingserver angesprochen werden soll.
 * \param	streamingFILE	Pointer der auf einen String zeigt der das File auf den Streamingserver zeigt.
 * \param	socket			TCP-handle um Parameter der Aushandlung auf den TCP-Socket auszugeben ( z.B. Handle einer offenen Telnetsession )
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3client_startstreaming( unsigned char * streamingPROTO, unsigned char * streamingURL, unsigned long streamingIP, unsigned int streamingPORT, unsigned char * streamingFILE, unsigned int socket )
{
	
	unsigned int TEMP_MP3_SOCKET = SOCKET_ERROR;
	int REQUESTCODE;
	struct STDOUT oldSTDOUT;

	STDOUT_save( &oldSTDOUT );
	
	// mindestens IP und PORT gesetzt ?
	if ( streamingIP == 0  || streamingPORT == 0 ) return;

	// schon ein stream offen ? Wenn ja killen
	if ( MP3_SOCKET != SOCKET_ERROR )
	{
		printf_P( PSTR("Highjack current stream and kill them on Socket %d\r\n"), MP3_SOCKET );
		mp3client_stopstreaming ( );
	}
	printf_P( PSTR("Connect to %s"), streamingURL );
	
	// neue Verbindung auf machen
	TEMP_MP3_SOCKET = Connect2IP( streamingIP , streamingPORT );
				
	// neue Verbindung geöffnet ?
	if ( TEMP_MP3_SOCKET != SOCKET_ERROR )
	{
		printf_P( PSTR(" connected ;-D\r\n"), streamingURL );
	}
	else
	{
		printf_P( PSTR(" fail to connect ;-(\r\n"), streamingURL );
	}
	
	STDOUT_Flush();
	
	// Wenn Verbindung geöffnet, weiter weiter :-)
	if ( TEMP_MP3_SOCKET != SOCKET_ERROR )
	{
		if ( STDOUT_set( _TCP, TEMP_MP3_SOCKET ) == -1 )
		{
			STDOUT_restore( &oldSTDOUT );
			return;
		}
		// Request senden
		printf_P( PSTR("GET /%s HTTP/1.1\r\n" \
					   "Host: %s\r\n" \
					   "User-Agent: mp3-streamingclient v0.1a (ccc-berlin)\r\n" \
					   "Icy-MetaData: %d\r\n" \
					   "Keep-Alive: \r\n" \
					   "Connection: TE, Keep-Alive\r\n" \
					   "TE: trailers\r\n\r\n" ), streamingFILE , streamingFILE , metainfo );

		STDOUT_restore( &oldSTDOUT );

		printf_P( PSTR("Request on TCP-Socket %d\r\n"), TEMP_MP3_SOCKET );
		
		// Stream setzen
		MP3_SOCKET = TEMP_MP3_SOCKET;
		
		// Lautstärke sichern
		timer0_stop();
		vol = VS10xx_read( VS10xx_Register_VOL );

		// VS10xx reseten
		VS10xx_reset( );

		// gesicherte Lautstärke setzen
		VS10xx_write( VS10xx_Register_VOL, vol );
		
		printf_P( PSTR("MP3 buffering ( FIFO = %d with %dBytes )\r\n"), MP3_FIFO, mp3_buffer_size );
						
		// Timeout für pufferauffüllen setzen
		CLOCK_SetCountdownTimer ( mp3timer, 1000, MSECOUND );
		
		// alten MP3-Puffer löschen
		Flush_FIFO ( MP3_FIFO );
		
		STDOUT_set( NONE, 0 );
		
		REQUESTCODE = mp3client_pharseheader( MP3_SOCKET );

		STDOUT_restore( &oldSTDOUT );
		
		if ( REQUESTCODE != 200 )
		{
			switch ( REQUESTCODE )
			{
				case -1:		printf_P( PSTR("Timeout:") );
								break;
				case 400:		printf_P( PSTR("Requestcode: %d Bad Request:"), REQUESTCODE );
								break;
				case 401:		printf_P( PSTR("Requestcode: %d Unauthorized:"), REQUESTCODE );
								break;
				case 403:		printf_P( PSTR("Requestcode: %d Forbidden:"), REQUESTCODE );
								break;
				case 404:		printf_P( PSTR("Requestcode: %d Not Found:"), REQUESTCODE );
								break;
				case 405:		printf_P( PSTR("Requestcode: %d Method Not Allowed:"), REQUESTCODE );
								break;
				case 500: 		printf_P( PSTR("Requestcode: %d Internal Server Error:"), REQUESTCODE );			
								break;
				default:		printf_P( PSTR("Requestcode: %d Unknow:"), REQUESTCODE );			
								break;
			}
			
			printf_P( PSTR("Stream failed\r\n") );
			STDOUT_Flush();
			MP3_SOCKET = SOCKET_ERROR;
			return;
		}
		// wenn alles schick mit dem stream dann:
		printf_P( PSTR("Requestcode: %d - Stream okay, Tsunami can highjack them totaly\r\n"), REQUESTCODE );

		metaintcounter = 0;
		
		slowstart = 1;
		
		// Counter setzen für die statsupdates
		CLOCK_SetCountdownTimer ( mp3timer, 50, MSECOUND );

	}
	
	STDOUT_Flush();

}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Hält einen gerade abgespielten MP3-Stream an.
 * \param	socket			TCP-handle um Parameter der Aushandlung auf den TCP-Socket auszugeben ( z.B. Handle einer offenen Telnetsession )
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3client_stopstreaming( void )
{
	timer0_stop();
	// stream offen ?
	if ( MP3_SOCKET != SOCKET_ERROR )
		CloseTCPSocket ( MP3_SOCKET );

	// Meldung ausgeben
	printf_P( PSTR("Connection closed.\r\n") );
	
	// MP3-Puffer leeren
	Flush_FIFO( MP3_FIFO );

	printf_P( PSTR("FIFO flushed.\r\n") );
	
	STDOUT_Flush();
	
	// socket ungültig setzen
	MP3_SOCKET = SOCKET_ERROR;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Toggelt das Verboselevel auf der RS232.
 * \param	NONE
 * \return	level	Das Verboselevel 0 oder 1.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char mp3client_setmetainfo( void )
{
	// Wenn verbose auf 0 dann auf 1 setzen, sonst umgekehrt
	if ( metainfo == 0 )
		metainfo = 1;
	else
		metainfo = 0;
	
	// Level zurückgeben
	return( metainfo );
}
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Toggelt das Verboselevel auf der RS232.
 * \param	NONE
 * \return	level	Das Verboselevel 0 oder 1.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char mp3client_setverboselevel( void )
{
	// Wenn verbose auf 0 dann auf 1 setzen, sonst umgekehrt
	if ( verboselevel == 0 )
		verboselevel = 1;
	else
		verboselevel = 0;
	
	// Level zurückgeben
	return( verboselevel );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Leist den HTTP-header.
 * \param	socket	Der TCP-Socket von wo die HTTP-header lesen werden soll und speichert Infos zum Stream.
 * \return	REQUEST Der Returncode auf dem HTTP-header.
 */
/*------------------------------------------------------------------------------------------------------------*/
int mp3client_pharseheader ( unsigned int socket )
{
	char pharsebuffer[256], Data;
	int pharsebufferpos=0;
	int REQUEST = 0;
	
	metaint = 0;
	
	if ( verboselevel != 0 ) printf_P( PSTR("\r\n\r\nGet Headerinformation\r\n---------------------\r\n") );

	while( 1 )
	{

		while ( GetBytesInSocketData( MP3_SOCKET ) == 0 && CLOCK_GetCountdownTimer( mp3timer ) != 0)
		{
			if ( CLOCK_GetCountdownTimer( mp3timer ) == 0 )
				return( -1 );			
		}
		
		Data = GetByteFromSocketData ( socket );
 		if ( verboselevel != 0 ) printf_P( PSTR("%c"), Data);
		
		if ( CLOCK_GetCountdownTimer( mp3timer ) == 0 )
			return( -1 );
		
		if ( Data != 0x0a )
		{
			if ( pharsebufferpos < 255 )
			{
				if ( Data != 0x0d )
				{
					pharsebuffer[ pharsebufferpos++ ] = Data;
					pharsebuffer[ pharsebufferpos ] = '\0';
				}
			}
			if ( Data == 0x0d )
			{
				if ( !memcmp( &pharsebuffer[0] , "ICY" , 3 ) )
				{
					REQUEST = atoi( &pharsebuffer[4] );
					if ( REQUEST != 200 ) return( REQUEST );
				}
				else if ( !memcmp( &pharsebuffer[0] , "HTTP/1.1" , 8 ) )
				{
					REQUEST = atoi( &pharsebuffer[9] );
					if ( REQUEST != 200 ) return( REQUEST );
				}
				else if ( !memcmp( &pharsebuffer[0] , "icy-metaint:" , 11 ) )
				{
					metaint = atoi( &pharsebuffer[12] );
				}
				else if ( !memcmp( &pharsebuffer[0] , "icy-name" , 8 ) )
				{
					mp3clientupdateNAME( &pharsebuffer[9] );
				}
				else if ( !memcmp( &pharsebuffer[0] , "icy-url" , 7 ) )
				{
					mp3clientupdateMETAURL( &pharsebuffer[8] );
				}
				else if ( !memcmp( &pharsebuffer[0] , "icy-genre" , 9 ) )
				{
					mp3clientupdateGENRE( &pharsebuffer[10] );
				}
				else if ( !memcmp( &pharsebuffer[0] , "icy-br" , 6 ) )
				{
					mp3clientupdateBITRATE( atoi( &pharsebuffer[7] ) );
				}

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
	
	if ( verboselevel != 0 ) printf_P( PSTR("----------\r\nHeader End\r\n") );

	return( REQUEST );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Liest die Metainfo aus dem Stream aus und speichert ihn ab.
 * \param	socket		Der TCP-Socket von wo aus die Metainfo gelesen werden soll
 * \param	metainfolen	Länge der Metainfo in Bytes.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3client_readmetainfo( unsigned int socket, unsigned int metainfolen )
{
	char data,buffer[256];
	int i,x=0;

	if ( verboselevel != 0 ) printf_P( PSTR("\r%c[2K"),27);
	
	for ( i = 0 ; i < metainfolen ; i++ )
	{
		while ( GetBytesInSocketData( socket ) == 0 );
		data = GetByteFromSocketData ( socket );
		if ( verboselevel != 0 ) printf_P( PSTR("%c"), data );

		// Info auseinander bauen
		if ( data == ';' )
		{
			buffer[x] = '\0';
			if ( !memcmp( &buffer[0] , "StreamTitle=" , 11 ) )
			{
				if ( strlen( &buffer[12] ) != 0 )
					mp3clientupdateMETATITLE( &buffer[12] );
			}
			if ( !memcmp( &buffer[0] , "StreamUrl=" , 9 ) )
			{
				if ( strlen( &buffer[10] ) != 0 )
					mp3clientupdateMETAURL( &buffer[10] );
			}
			x = 0;
		}
		else if ( data == '\'' )
		{
		}
		else
		{
			if ( x < 255 )
			{
				buffer[x] = data;
				buffer[x+1] = '\0';
				x++;
			}
		}
	}
	if ( verboselevel != 0 ) printf_P( PSTR("\r\n"));
}
#endif

//@}

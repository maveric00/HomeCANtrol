/*!\file mp3-clientserver.c \brief Das Tsunami-Steuerinferface */
//***************************************************************************
//*            mp3-clientserver.c
//*
//*  Mon May 18:46:47 2008
//*  Copyright  2008  sharandac
//*  Email: sharandac@snafu.de
///	\ingroup hardware
///	\defgroup VS1001 Das Tsunami-Steuerinferface (mp3-clientserver.c)
///	\code #include "mp3-clientserver.h" \endcode
///	\par Uebersicht
///		Die Lib stellt Funktionen um die Tsunami-Streamingengine per
///		Console zu steuern zur Verfügung.
//****************************************************************************/
//@{
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
 
#include <avr/io.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/version.h>
#include <math.h>

#include "config.h"

#if defined(HTTPSERVER_STREAM)

#include "mp3-clientserver.h"
#include "mp3-streaming.h"

#include "hardware/uart/uart.h"
#if defined(VS10XX)
	#include "hardware/vs10xx/vs10xx.h"
#else
	#error "Bitte unterstützung für den VS10xx in der config.h aktivieren."
#endif
#include "hardware/led/led_core.h"

#include "system/net/tcp.h"
#include "system/stdout/stdout.h"
#include "system/net/ip.h"
#include "system/net/ethernet.h"
#include "system/net/dns.h"

struct MP3_STATS mp3_stats;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Biete eine Schnittstelle zum steuern des MP3-client zum streamen.
 * \param 	CMD_Buffer	Pointer auf den Puffer der die Befehle zur Steuerung enthält.
 * \param 	socket		Das TCP-Socket auf dem eventuelle Ausgaben laufen.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3clientcommand( unsigned char * CMD_Buffer , unsigned int socket )
{
	if ( !memcmp( &CMD_Buffer[0] , "play" , 4 ) )
	{
		printf_P( PSTR("starte Streaming\r\n") );
		if ( strlen( &CMD_Buffer[ 0 ] ) > 5 )
		{
			PlayURL( &CMD_Buffer[ 5 ], socket );
		}
	}
	else if ( !memcmp( &CMD_Buffer[0] , "replay" , 6 ) )
	{
		printf_P( PSTR("restarte Streaming\r\n") );
		mp3client_startstreaming( mp3_stats.streamingPROTO, mp3_stats.streamingURL, mp3_stats.streamingIP,  mp3_stats.streamingPORT, mp3_stats.streamingFILE, socket );
		mp3client_PrintHeaderinfo ();
	}
	else if ( !memcmp( &CMD_Buffer[0] , "stop" , 4 ) )
	{
		printf_P( PSTR("beende Streaming\r\n") );
		mp3client_stopstreaming();
	}
	else if ( !memcmp( &CMD_Buffer[0] , "bass" , 4 ) )
	{
		printf_P( PSTR("bass \r\n") );
		VS10xx_write( VS10xx_Register_MODE, 128 );
	}
	else if ( !memcmp( &CMD_Buffer[0] , "metainfo" , 8 ) )
	{
		printf_P( PSTR("set metainfo to %d\r\n"),  mp3client_setmetainfo () );
	}
	else if ( !memcmp( &CMD_Buffer[0] , "verbose" , 7 ) )
	{
		printf_P( PSTR("set verboselevel to %d\r\n"),  mp3client_setverboselevel () );
	}
	else if ( !memcmp( &CMD_Buffer[0] , "info" , 4 ) )
	{
		mp3clientPrintInfo( socket, TELNET );
	}
	else if ( !memcmp( &CMD_Buffer[0] , "vol" , 3 ) )
	{
		if ( atoi( &CMD_Buffer[4] ) <= 255 && atoi( &CMD_Buffer[4] ) >= 0 && CMD_Buffer[3] == ' ')
		{
			printf_P( PSTR("vol = %d\r\n"), atoi( &CMD_Buffer[4] ) );
			VS10xx_vol( (unsigned char) atoi( &CMD_Buffer[4] ), (unsigned char) atoi( &CMD_Buffer[4] ) );
		}
		else
		{
			printf_P( PSTR("Vol sollte zwischen 0 .. 255 liegen!\r\n") );
			printf_P( PSTR("Aktueller Werte links %d, rechts %d.\r\n"), 255 - ( VS10xx_read(VS10xx_Register_VOL)>>8 ) , 255 - VS10xx_read(VS10xx_Register_VOL) & 0xff );
		}
	}
	else
	{
		printf_P( PSTR(	"MP3-Streamingclient v0.2a\r\n\r\n"
						"play URL		spielt einen Stream von URL ab\r\n"
						"replay			spielt den angehalten Stream ab\r\n"
						"stop			stoppt den Stream\r\n"
						"info			gibt Infos zum Stream aus\r\n"
						"vol n			stellt die Laufstaerke auf n ein,leise -> 0..255-> laut\r\n"
						"bass			schaltet den Bassenhancer ein\r\n"
						"verbose		toggelt das Verboselevel fuer Ausgaben auf der RS232\r\n"
						"metainfo		schaltet das Metastreaming ein fuer Infos \r\n"
					    "               waehrend des Streamings(experimentel)\r\n"
						"? oder help	Hilfe, was sonst :-)\r\n\r\n"));
	}
	STDOUT_Flush();
	
	return;
}


void RePlayURL( unsigned int socket )
{
	mp3client_startstreaming( mp3_stats.streamingPROTO, mp3_stats.streamingURL, mp3_stats.streamingIP,  mp3_stats.streamingPORT, mp3_stats.streamingFILE, socket );
	mp3client_PrintHeaderinfo ();	
}

void PlayURL( char * URL, unsigned int socket )
{
	PharseURL( URL );
	mp3client_startstreaming( mp3_stats.streamingPROTO, mp3_stats.streamingURL, mp3_stats.streamingIP,  mp3_stats.streamingPORT, mp3_stats.streamingFILE, socket );
	mp3client_PrintHeaderinfo ();	
}

void StopPlay( void )
{
	mp3client_stopstreaming ();	
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Zerlegt die URL in ihre Bestandteile und speichert diese.
 * \param 	URLBuffer	Pointer auf den Puffer der die Befehle zur Steuerung enthält.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void PharseURL( unsigned char * URLBuffer )
{
	unsigned int i,y,argv=0,len,proto=0;
	unsigned char * argc[16];
	
	if ( URLBuffer[ strlen( URLBuffer ) - 1 ] == '\r' || URLBuffer[ strlen( URLBuffer ) - 1 ] == '\n' )
		URLBuffer[ strlen( URLBuffer ) - 1 ] = '\0';
	
	len = strlen( URLBuffer );
	
	argc[ argv++ ] = &URLBuffer[0];
	
	mp3_stats.streamingIP = 0;
	mp3_stats.streamingURL[0] = '\0';
	mp3_stats.streamingPORT = 80;
	mp3_stats.streamingFILE[0] = '\0';
	mp3_stats.streamingMETATITLE[0] = '\0';
	mp3_stats.streamingGENRE[0] = '\0';
	mp3_stats.streamingMETAURL[0] = '\0';
	mp3_stats.streamingNAME[0] = '\0';
	mp3_stats.streamingBITRATE = 0;

	for( i = 0 ; ( i < len ) && ( argv < 16 ) ; i ++ )
	{
		if ( !memcmp( &URLBuffer[i] , "://" , 3 ) )
		{
			URLBuffer[i] = '\0';
			i = i + 3 ;
			argc[ argv++ ] = &URLBuffer[i];
			proto = 1;
		}
		if( URLBuffer[i] == ':' )
		{
			URLBuffer[i++] = '\0';
			argc[ argv++ ] = &URLBuffer[i];
		}
		if( URLBuffer[i] == '/' )
		{
			URLBuffer[i++] = '\0';
			argc[ argv++ ] = &URLBuffer[i];
			break;
		}
	}

	for ( i = 0 ; i < argv ; i ++ )
	{
		if ( proto == 0 )
		{
			strcpy( mp3_stats.streamingURL, argc[i++] );
			strcpy_P( mp3_stats.streamingPROTO , PSTR("HTTP") );
		}
		else
		{
			strcpy( mp3_stats.streamingPROTO , argc[i++] );
			strcpy( mp3_stats.streamingURL, argc[i++] );
		}
		
		if ( atoi( argc[i] ) != 0 )
		{
			if ( i < argv )
				mp3_stats.streamingPORT = atoi( argc[i++]);
			if ( i < argv )
				strcpy( mp3_stats.streamingFILE, argc[ i++ ]);
		}
		else
		{
			if ( i < argv )
				strcpy( mp3_stats.streamingFILE, argc[ i++ ]);
		}
	}
	
	if( strtoip ( mp3_stats.streamingURL ) != 0 )
		mp3_stats.streamingIP = strtoip ( mp3_stats.streamingURL );
	else
		mp3_stats.streamingIP = DNS_ResolveName( mp3_stats.streamingURL );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Updatet die Struct für den mp3-stream.
 * \param 	bandwidth	enthält die benutzte Bandbreite in bit/s.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void mp3clientupdate( unsigned long bandwidth, unsigned int tcpbuffer, unsigned int streamingbuffer, unsigned int decodetime, unsigned char verboselevel )
{
	mp3_stats.streamingrate = bandwidth;
	mp3_stats.streamingtcpbuffer = tcpbuffer;
	mp3_stats.streamingbuffer = streamingbuffer;
	mp3_stats.streamingtime = decodetime;
	mp3_stats.streamingverboselevel = verboselevel;
}

void mp3clientupdateNAME( char * streamingNAME )
{
	strcpy( mp3_stats.streamingNAME , streamingNAME );	
}

void mp3clientupdateGENRE( char * streamingGENRE )
{
	strcpy( mp3_stats.streamingGENRE , streamingGENRE );	
}

void mp3clientupdateMETATITLE( char * streamingMETATITLE )
{
	strcpy( mp3_stats.streamingMETATITLE , streamingMETATITLE );	
}

void mp3clientupdateMETAURL( char * streamingMETAURL )
{
	strcpy( mp3_stats.streamingMETAURL , streamingMETAURL );	
}

void mp3clientupdateBITRATE( int streamingBITRATE )
{
	mp3_stats.streamingBITRATE = streamingBITRATE;	
}

void mp3clientPrintInfo( unsigned int socket , char type )
{
		char html[] = "<br>";
		char telnet[] = "\r\n";
		char * TYPE;
	
		if ( type == HTML ) TYPE = html;
		else TYPE = telnet;

		if ( type == TELNET )
		{
			printf_P( PSTR("streaminginfo\r\n") );
			printf_P( PSTR("  URL = %s://%s:%d/%s\r\n"), mp3_stats.streamingPROTO, mp3_stats.streamingURL, mp3_stats.streamingPORT, mp3_stats.streamingFILE );
			printf_P( PSTR("  PROTOCOL = %s\r\n"), mp3_stats.streamingPROTO );
			printf_P( PSTR("  URL = %s\r\n"), mp3_stats.streamingURL );
			printf_P( PSTR("  IP = %08lx\r\n"), mp3_stats.streamingIP  );
			printf_P( PSTR("  PORT = %u\r\n"), mp3_stats.streamingPORT  );
			printf_P( PSTR("  FILE = %s\r\n"), mp3_stats.streamingFILE  );
		}
		printf_P( PSTR("Name = %s%s"), mp3_stats.streamingNAME, TYPE );
		printf_P( PSTR("Genre = %s%s"), mp3_stats.streamingGENRE, TYPE );
		printf_P( PSTR("Titel = %s%s"), mp3_stats.streamingMETATITLE, TYPE );
		printf_P( PSTR("URL = "));

		if ( type == HTML )
		{
			printf_P( PSTR("<a href=\"%s\">%s</a>%s"), mp3_stats.streamingMETAURL, mp3_stats.streamingMETAURL , TYPE );
			printf_P( PSTR("Stream-URL = %s://%s:%u/%s\r\n"), mp3_stats.streamingPROTO, mp3_stats.streamingURL, mp3_stats.streamingPORT, mp3_stats.streamingFILE );
		}
		else
			printf_P( PSTR("%s\r\n"), mp3_stats.streamingMETAURL);	
	
		printf_P( PSTR("Bitrate = %d kbit/s%s"), mp3_stats.streamingBITRATE, TYPE );
		printf_P( PSTR("Bandbreitenausnutzung = %ld kbit/s (%2d kbyte/s)\r\n"), ( mp3_stats.streamingrate / 125l ), mp3_stats.streamingrate / 1000l );
		printf_P( PSTR("TCP-Puffer = %d Byte%s"), mp3_stats.streamingtcpbuffer, TYPE );
		printf_P( PSTR("MP3-Puffer = %d Byte%s"), mp3_stats.streamingbuffer, TYPE );
		printf_P( PSTR("Zeit = %d sec\r\n"), mp3_stats.streamingtime );
}

void mp3client_PrintHeaderinfo( void )
{
		if ( strlen ( mp3_stats.streamingNAME ) != 0 )
			printf_P( PSTR("Name = %s\r\n"), mp3_stats.streamingNAME );

		if ( strlen ( mp3_stats.streamingGENRE ) != 0 )
			printf_P( PSTR("Genre = %s\r\n"), mp3_stats.streamingGENRE );
	
		if ( strlen ( mp3_stats.streamingMETAURL ) != 0 )
			printf_P( PSTR("MetaURL = %s\r\n"), mp3_stats.streamingMETAURL );

		if ( mp3_stats.streamingBITRATE != 0 )
			printf_P( PSTR("Bitrate = %d kbit/s\r\n"), mp3_stats.streamingBITRATE );
}
#endif

//@}

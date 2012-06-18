/*! \file httpd2.c \brief Ein sehr einfacher HTTP-Server */
//***************************************************************************
//*            httpd2.c
//*
//*  Mon Jun 23 14:19:16 2008
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup httpd2 Ein sehr einfacher HTTP-Server (httpd2.c)
///	\code #include "httpd2.h" \endcode
///	\par Uebersicht
/// 	Es handelt sich hier um einen einfachen HTTP-Server der nur eine statische
/// Seite ausliefert. 
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
#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/clock/clock.h"
#include "system/net/ethernet.h"
#include "system/net/tcp.h"
#include "system/stdout/stdout.h"
#include "system/thread/thread.h"

#include "hardware/uart/uart.h"

#include "httpd2.h"
#include "httpd2_pharse.h"
#include "cgibin/cgi-bin.h"
#include "files.h"


static struct HTTP_REQUEST http_request;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void httpd_init( void )
	{
		cgi_init();
		RegisterTCPPort( HTTP_PORT );
		http_request.HTTP_SOCKET == SOCKET_ERROR;
		printf_P( PSTR("HTTP-Server gestartet auf Port %d.\r\n") , HTTP_PORT);
		THREAD_RegisterThread( httpd_thread, PSTR("httpd"));
	}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Der eigentlich HTTP-Server. Hier werden die Daten ausgewertet und entsprechend reagiert. 
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void httpd_thread( void )
{
	char Data;
	
	struct STDOUT oldstream;
	
	// keine alte Verbindung offen?
	if ( http_request.HTTP_SOCKET == SOCKET_ERROR )
	{ 	
		// auf neue Verbindung testen
		http_request.HTTP_SOCKET = CheckPortRequest( HTTP_PORT );
		if ( http_request.HTTP_SOCKET != SOCKET_ERROR )
		{	
			http_request.STATE = CONNECTED;
			http_request.HTTP_POS = 0;
			http_request.HTTP_LINEBUFFER[ 0 ] = '\0';
			http_request.GET_FILE[ 0 ] = '\0';
			http_request.GET_DATA[ 0 ] = '\0';
			http_request.argc = 0 ;
		}
	}
	else
	{
		// checken ob noch offen ist
		if( CheckSocketState( http_request.HTTP_SOCKET ) == SOCKET_NOT_USE )
		{
			CloseTCPSocket( http_request.HTTP_SOCKET );
			http_request.HTTP_SOCKET = SOCKET_ERROR;
			http_request.STATE = DISCONNECT;
		}
		else
		{			
			if ( http_request.STATE == CONNECTED )
			{	
				while( GetBytesInSocketData( http_request.HTTP_SOCKET ) >= 1 )
				{
					Data = ( GetByteFromSocketData ( http_request.HTTP_SOCKET ) );
					if ( Data != 0x0a )
					{
						// Daten im Puffer sichern
						if ( http_request.HTTP_POS < ( REQUEST_BUFFERLEN - 1 ) )
						{
							http_request.HTTP_LINEBUFFER[ http_request.HTTP_POS++ ] = Data;
							http_request.HTTP_LINEBUFFER[ http_request.HTTP_POS ] = '\0';
						}
						
						// Zeilenende erreicht?
						if ( Data == 0x0d )
						{
							// Steht was im Puffer, wenn ja pharsen
							if ( http_request.HTTP_POS != 0 )
							{
								http_request.HTTP_POS = 0;
								if ( !memcmp( &http_request.HTTP_LINEBUFFER [0] , "GET" , 3 ) )
								{
									http_request.REQUEST_TYPE = GET_REQUEST;
									PharseGetData( &http_request );
									PharseGetFile( &http_request ); 
								}
								else if ( !memcmp( &http_request.HTTP_LINEBUFFER [0] , "POST" , 4 ) )
								{
									http_request.REQUEST_TYPE = POST_REQUEST;
									PharseGetFile( &http_request ); 
								}
								else if ( !memcmp( &http_request.HTTP_LINEBUFFER [0] , "Content-Length: " , 16 ) )
								{
									http_request.REQUEST_LEN = atoi( &http_request.HTTP_LINEBUFFER [16] );
								}								
							}
				
							// Ist es eine Leerzeile, dann ist der Request zu ende und die verarbeitung kann beginnen
							if ( http_request.HTTP_LINEBUFFER[ 0 ] == 0x0d )
							{
								if ( http_request.REQUEST_TYPE == GET_REQUEST )
									FlushSocketData( http_request.HTTP_SOCKET );
								http_request.STATE =  REQUEST_END;
							}
						}
					}
				}
			}
			else if (http_request.STATE == REQUEST_END )
			{
				STDOUT_save( &oldstream );
				STDOUT_set( _TCP, http_request.HTTP_SOCKET );

				if ( check_cgibin ( &http_request ) == 1 )
				{
				}
				else if ( check_files ( &http_request ) == 1 )
				{
				}
				else
				{	
					printf_P( PSTR(	"HTTP/1.0 403\r\n"
									"Content-Type: text/html\r\n"
									"Keep-Alive: close\r\n"
									"\r\n"			
									"<HTML>\r\n"
									"<HEAD>\r\n"
									"<TITLE>403 File not found!</TITLE>\r\n"
									"</HEAD>\r\n"
									"<BODY>\r\n"
									"403 File not found!"
									"</BODY>\r\n"
									"</HTML>\r\n\r\n"));
				}

				STDOUT_Flush();
				STDOUT_restore( &oldstream );
				
				CloseTCPSocket( http_request.HTTP_SOCKET );
				http_request.HTTP_SOCKET = SOCKET_ERROR;
			}
		}
	}
}
//}@

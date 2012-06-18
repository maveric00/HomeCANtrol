/*! \file telnet.c \brief Ein sehr einfacher Telnetclient */
//***************************************************************************
//*            telnet.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup telnet Ein sehr einfacher Telnetclient (telnet.c)
///	\code #include "telnet.h" \endcode
///	\par Uebersicht
/// 	Ein sehr einfacher Telnet-client der auf dem Controller läuft. Ermöglicht
/// das Abfragen vom Status des Controllers und diverse andere Dinge.
/// \date	04-18-2008: -> Der Blödsinn mit Windows ist beseitigt, geht jetzt. Es wird jetzt pro Durchlauf versucht, den Puffer auszulesen und
///			erst wenn eine Eingabe mit 0x0a,0x0d (LF, CR) abgeschlossen wird, gehts ab an die Verarbeitung des Strings. Was für ein Akt :-) und alles wegen
///			Windows.
/// \date   05-25-2009: Umstellung der Telnetstruktur und Ausführung der Kommandos.
/// \date	01-01-2010:	Umstellung der Telnetstruktur für das ausführen und registrieren von Telnetkommandos. Diese werden jetzt von extern Registriert, so das ein
///			saubere Trennung besteht.
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
#include <avr/version.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "system/net/tcp.h"
#include "system/stdout/stdout.h"
#include "system/thread/thread.h"

#include "telnet.h"
#include "config.h"

struct DYN_TELNET telnet_cmd_table [ MAX_TELNET_ENTRYS ];

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert den Telnet-clinet und registriert den Port auf welchen dieser lauschen soll.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void telnet_init()
{
	int i;

	RegisterTCPPort( TELNET_PORT );
	printf_P( PSTR("Telnet-Server gestartet auf Port %d.\r\n") , TELNET_PORT );

	// Befehlstabelle löschen
	for ( i = 0 ; i < MAX_TELNET_ENTRYS ; i++ )
	{
		telnet_cmd_table[ i ].dyntelnet_function = NULL;
	}

	THREAD_RegisterThread( telnet_thread, PSTR("telnet"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert den Telnet-clinet und registriert den Port auf welchen dieser lauschen soll.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
int telnet_RegisterCMD( DYN_TELNET_CALLBACK dyntelnet_function, const prog_char * funktionname )
{
	int i,retval;

	retval = -1;
	
	for ( i = 0 ; i < MAX_TELNET_ENTRYS ; i++ )
	{
		if ( telnet_cmd_table[ i ].dyntelnet_function == dyntelnet_function )
			break;
		
		if ( telnet_cmd_table[ i ].dyntelnet_function == NULL )
		{
			telnet_cmd_table[ i ].dyntelnet_function = dyntelnet_function;			
			telnet_cmd_table[ i ].functionname = funktionname;
			retval = 0;
			break;
		}
	}
	return( retval );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Der Telnet-client an sich. Er wird zyklisch aufgerufen und schaut nach ob eine Verbindung auf den
 * registrierten Port eingegangen ist. Wenn ja holt er sich die Socketnummer der Verbindung und speichert diese.
 * Wenn eine Verbindung zustande gekommen ist wird diese wiederrum zyklisch nach neuen Daten abgefragt und entsprechend
 * reagiert.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
void telnet_thread()
	{
		static struct TELNET_SESSION telnet_session;
		struct STDOUT oldstream;
		char Data; 
		int i;
		
		// keine alte Verbindung offen?
		if ( telnet_session.SOCKET == NO_SOCKET_USED )
		{ 	
			// auf neue Verbindung testen
			telnet_session.SOCKET = CheckPortRequest( TELNET_PORT );
			if ( telnet_session.SOCKET != NO_SOCKET_USED )
			{	
				STDOUT_save( &oldstream );
				STDOUT_set( _TCP, telnet_session.SOCKET );
				// Wenn ja, Startmeldung ausgeben und startzustand herstellen für telnet
				printf_P( PSTR( "Welcome on Tiny-Telnetserver 0.2!\r\n"
								"Und, los geht's...!!! (\"help\" hilft :-))\r\n> " ));
				STDOUT_restore( &oldstream );

				// TELNET_BUFFER leeren und auf Ausgangszustand setzen
				telnet_session.STATE = 0;
				telnet_session.POS = 0;
				telnet_session.BUFFER[0] = '\0';
				telnet_session.STATE = 0;
				
				FlushSocketData( telnet_session.SOCKET );
			}
		}
		// Wenn alte Verbindung offen hier weiter:
		else
		{
			// checken, ob noch offen ist
			if( CheckSocketState( telnet_session.SOCKET ) == SOCKET_NOT_USE )
			{
				CloseTCPSocket( telnet_session.SOCKET );
				telnet_session.SOCKET = NO_SOCKET_USED;
				return;
			}

			// Auf neue Daten zum zusammenbauen testen
			// hier wird der TELNET_BUFFER aufgefüllt bis 0x0a oder 0x0d eintreffen. der Puffer ist statisch
			// Wenn ein 0x0a oder 0x0d empfangen wurde, wird der TELNET_STATE auf 1 gesetzt, damit er verarbeitet werden kann
			if ( telnet_session.STATE == 0 )
			{	
				
				while( GetBytesInSocketData( telnet_session.SOCKET ) >= 1 )
				{
					Data = ( GetByteFromSocketData ( telnet_session.SOCKET ) );
					if ( Data != 0x0a && Data <= 0x7f)
					{
						if ( telnet_session.POS < TELNET_BUFFER_LEN )
						{
							telnet_session.BUFFER[ telnet_session.POS++ ] = Data;
							telnet_session.BUFFER[ telnet_session.POS ] = '\0';
						}
						if ( Data == 0x0d )
						{
							telnet_session.STATE = 1;
							break;
						}
					}
				}
			}	

			// Wenn TELNET_BUFFER eine Zeile vollständig hat gehts hier weiter
			if ( telnet_session.STATE == 1 )
			{

				STDOUT_save( &oldstream );
				STDOUT_set( _TCP, telnet_session.SOCKET );

				telnet_session.argc = TELNET_pharse( telnet_session.BUFFER, telnet_session.argv, MAX_ARGC );
				TELNET_runcmd( telnet_session.argc, telnet_session.argv );

				// auf QUIT checken
				if ( !strcmp_P( telnet_session.argv[ 0 ] , PSTR("quit") ) ) 
				{
					// Socket schließen
					printf_P( PSTR("Verbindung wird geschlossen\r\n") );
					STDOUT_Flush();
					STDOUT_set( RS232, 0);
					CloseTCPSocket( telnet_session.SOCKET );
					telnet_session.SOCKET = NO_SOCKET_USED;
					return;
				}
				else if( !strcmp_P( telnet_session.argv[ 0 ] , PSTR("help") ) )
				{
					printf_P( PSTR("Folgende Befehle sind vorhanden:\r\n\r\n") );
					for( i = 0 ; i < MAX_TELNET_ENTRYS ; i++ )
					{
						if ( telnet_cmd_table[ i ].functionname != NULL )
						{
							printf_P( telnet_cmd_table[ i ].functionname );
							printf_P( PSTR("\r\n") );
						}
					}
				}

				printf_P( PSTR("> ") );
				
				STDOUT_restore( &oldstream );

				telnet_session.STATE = 0;
				telnet_session.POS = 0;
				telnet_session.BUFFER[0] = '\0';
				telnet_session.STATE = 0;
			}
		}
	}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Ruft ein Kommando extern.
 * \param 	cmdstring   Kommandostring der ausgeführt werden soll.
 * \return	int			0 wenn Kommando gefunden, bei Fehler -1.
 */
/*------------------------------------------------------------------------------------------------------------*/
int TELNET_pharse( char * BUFFER, char ** argv, int max_argc )
{
		
	int i = 0;
	int _argc = 0;
	char toggle = 0;
	int stringlen = strlen( BUFFER );
	
	argv[ _argc ] = &BUFFER[ i ]; 
	
	for( i = 0 ; i < stringlen && _argc < max_argc ; i++ )
	{
		switch( BUFFER[ i ] )
		{
			
		case '\0':		i = stringlen;
						break;
		case 0x0d:		BUFFER[ i ] = '\0';
						i = stringlen;
						break;
		case '\"':		if ( toggle == 0 )
						{	
							toggle = 1;
							BUFFER[ i ] = '\0';
							argv[ _argc ] = &BUFFER[ i + 1];
						}
						else
						{
							toggle = 0;
							BUFFER[ i ] = '\0';
						}
						break;
		case ' ':		if ( toggle == 0 )						
						{
							BUFFER[ i ] = '\0';
							_argc++;
							argv[ _argc ] = &BUFFER[ i + 1 ];
						}
						break;
		}
	}
	return( _argc + 1 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Ruft ein Kommando extern.
 * \param 	cmdstring   Kommandostring der ausgeführt werden soll.
 * \return	int			0 wenn Kommando gefunden, bei Fehler -1.
 */
/*------------------------------------------------------------------------------------------------------------*/
int TELNET_runcmd( int argc, char ** argv )
{
	int i, returncode = -1;
	
	for( i = 0 ; i < MAX_TELNET_ENTRYS ; i++ )
	{
		if ( telnet_cmd_table[ i ].functionname == NULL )
		{
			break;
		}
		if ( !strcmp_P( argv[0] , telnet_cmd_table[ i ].functionname ) )
		{
			telnet_cmd_table[i].dyntelnet_function( argc, argv );
			returncode = 0;
			break;
		}
	}
	return( returncode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Ruft ein Kommando extern auf.
 * \param 	cmdstring   Kommandostring der ausgeführt werden soll.
 * \return	int			0 wenn Kommando gefunden, bei Fehler -1.
 */
/*------------------------------------------------------------------------------------------------------------*/
int TELNET_runcmdextern( char * cmdstring )
{
	char * argv[ MAX_ARGC ];
	int argc;
	
	argc = TELNET_pharse( cmdstring, argv, MAX_ARGC );
	
	return( TELNET_runcmd( argc, argv ) );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Ruft ein Kommando extern auf.
 * \param 	cmdstring   Kommandostring der ausgeführt werden soll der im Flash liegt.
 * \return	int			0 wenn Kommando gefunden, bei Fehler -1.
 */
/*------------------------------------------------------------------------------------------------------------*/
int TELNET_runcmdextern_P( const prog_char * cmdstring_P )
{
	char * argv[ MAX_ARGC ];
	int argc;
	char cmdstring[ TELNET_BUFFER_LEN ];
	
	strcpy_P( cmdstring, cmdstring_P );
	
	argc = TELNET_pharse( cmdstring, argv, MAX_ARGC );
	
	return( TELNET_runcmd( argc, argv ) );
}

//@}

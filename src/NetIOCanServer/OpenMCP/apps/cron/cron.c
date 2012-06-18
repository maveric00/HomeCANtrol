/*! \file "cron.c" \brief Der Crondienst */
//***************************************************************************
//*            checksum.c
//*
//*  Wed Jun  17 23:18:31 2009
//*  Copyright  2009  Dirk Broßwick
//*  Email
//****************************************************************************/
///	\ingroup cron
///	\defgroup Der Crondienst (cron.c)
///	\code #include "cron.h" \endcode
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "apps/telnet/telnet.h"
#include "system/clock/clock.h"
#include "system/config/config.h"
#include "system/stdout/stdout.h"
#include "system/thread/thread.h"

#include "cron.h"

const char entrystring[] PROGMEM = "CRON_ENTRY_%02d";

struct CRON crontable[ MAX_CRON ];

struct TIME lasttime;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den Cron-Dienst.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CRON_init( void )
{
	int i;
	struct TIME nowtime;
	
	for ( i = 0 ; i < MAX_CRON ; i++ )
		crontable[ i ].CMD_ENTRY = -1 ;
		
	CLOCK_GetTime ( &nowtime );

	lasttime.hh = nowtime.hh;
	lasttime.mm = nowtime.mm;

	THREAD_RegisterThread( CRON_thread, PSTR("cron"));
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Diese Funktion sollte zyklisch ausgerufen werden. Hier wird überprüft ob ein Cron ausgeführt werden soll.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CRON_thread( void )
{
	int i;
	
	struct TIME nowtime;
	
	CLOCK_GetTime ( &nowtime );
	
	if ( lasttime.hh != nowtime.hh || lasttime.mm != nowtime.mm )
	{
		lasttime.hh = nowtime.hh;
		lasttime.mm = nowtime.mm;
		
		for ( i = 0 ; i < MAX_CRON ; i++ )
		{
			if ( CRON_checkhit( i ) )
			{
				CRON_exec( i );
			}
		}
	}
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Überprüft ob es zu einen Treffer gekommen ist und sucht den passen Cron-Eintrag erraus.
 * \param 	entry	   Eintrag in der Crontabelle der überprüft werden soll, ob er ausgeführt werden soll.
 * \return	int
 * \retval  0			Kein Treffer.
 * \retval  1			Treffer! Cron kann ausgeführt werden.
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_checkhit( int entry )
{
	int returnval = -1;
	char HHhit = 0;
	char MMhit = 0;
	
	if ( crontable[ entry ].CMD_ENTRY == -1 )
		return( 0 );
	
	if ( crontable[ entry ].hh < 0 )
	{
		if ( lasttime.hh % ( crontable[ entry ].hh*-1 ) == 0 )
			HHhit = 1 ;
	}
	else if ( crontable[ entry ].hh == lasttime.hh )
		HHhit = 1 ;
	
	if ( crontable[ entry ].mm < 0 )
	{
		if ( lasttime.mm % ( crontable[ entry ].mm * -1 ) == 0 )
			MMhit = 1 ;
	}
	else if ( crontable[ entry ].mm == lasttime.mm )
		MMhit = 1 ;

	if ( HHhit == 1 && MMhit == 1 )
		return( 1 );
	else
		return( 0 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Führt einen Croneintrag aus.
 * \param 	entry	   Eintrag der ausgeführt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CRON_exec( int entry )
{
	struct STDOUT oldstream;

	char string[32];
	int argc;
	char * argv[ MAX_CRON_ARGC ];

	CRON_getentry( string, entry );

	argc = CRON_pharseentry( string, argv );
	
	STDOUT_save( &oldstream );
	STDOUT_set( NONE, 0 );

	TELNET_runcmdextern( argv[2] );

	STDOUT_restore( &oldstream );
}
	
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sucht den nächsten freien Eintrag in der Crontabelle im Configspeicher.
 * \param 	NONE
 * \return	int
 * \retval  -1			Kein freier Platz vorhanden.
 * \retval  >0			Nummer des freien Speicherplatzes.
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_getfreeentry( void )
{
	char string[32];
	
	int i;
	
	for( i = 0 ; i < MAX_CRON ; i++ )
	{
		sprintf_P( string, entrystring, i );
		if ( checkConfigName( string ) == -1 )
			return( i );
	}
	return( -1 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Speichert ein Cronentrag im Configspeicher ab.
 * \param 	cronstring  String der von der Shell ausgeführt werden soll.
 * \return	int
 * \retval  -1			Kein freier Platz vorhanden.
 * \retval  >0			Nummer des freien Speicherplatzes.
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_addentry( char * cronstring )
{
	char string[32];
	int i;
	
	i = CRON_getfreeentry( );
	
	if ( i != -1 )
	{
		sprintf_P( string, entrystring, i );
		writeConfig( string, cronstring );
	}	
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Löscht einen Eintrag aus der Crontabelle.
 * \param 	entry		Eintrag der gelöscht werden soll.
 * \return	int
 * \retval  -1			Kein freier Platz vorhanden.
 * \retval  >0			Nummer des freien Speicherplatzes.
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_delentry( int entry )
{
	char string[32];

	sprintf_P( string, entrystring, entry );
	deleteConfig( string );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt einen Croneintrag.
 * \param 	cronentry		Zeiger auf einen Speicherbereich in dem der String gespeichert werden soll.
 * \param 	entry			Eintrag der geholt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_findentry( char * cronstring )
{
	int i;
	char string[32];

	for( i = 0 ; i < MAX_CRON ; i++ )
	{
		CRON_getentry( string , i );
		if ( !strcmp( string, cronstring ) )
		    return( i );
	}
	
	return( -1 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt einen Croneintrag.
 * \param 	cronentry		Zeiger auf einen Speicherbereich in dem der String gespeichert werden soll.
 * \param 	entry			Eintrag der geholt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_getentry( char * cronstring, int entry )
{
	char string[32];
	
	sprintf_P( string, entrystring, entry );
	return( readConfig( string, cronstring ) );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Zerlegt einen Croneintrag.
 * \param 	cronentry		Zeiger auf einen Speicherbereich in dem der Croneintrag liegt.
 * \param 	entry			Eintrag der geholt werden soll.
 * \return	int				Anzahl der Argumente
 */
/*------------------------------------------------------------------------------------------------------------*/
int CRON_pharseentry( char * cronstring, char ** argv )
{
	int argc;
	
	argc = TELNET_pharse( cronstring, argv, MAX_CRON_ARGC );
	return( argc );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt die Aktuelle Crontabelle aus.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CRON_printcrontable( void )
{
	int i;
	char string[ 32 ];
	int argc;
	char * argv[ MAX_CRON_ARGC ];
	
	CRON_reloadcrontable();
	
	for( i = 0 ; i < MAX_CRON ; i++ )
	{
		if( crontable[ i ].CMD_ENTRY != -1 )
		{
			sprintf_P( string, entrystring, i );
			readConfig( string, string );
			argc = CRON_pharseentry( string, argv );
			printf_P( PSTR("%d: %d %d %s\r\n"), i, crontable[ i ].hh, crontable[ i ].mm, argv[ 2 ] );
		}
		else
		{
			printf_P( PSTR("%d: nicht gelegt\r\n"), i );
		}
	}
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Lädt die Crontabelle neu aus dem Configspeicher.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CRON_reloadcrontable( void )
{
	int i;
	int entry = 0;
	char string[ 32 ];
	char HH;
	char MM;
	int argc;
	char * argv[ MAX_CRON_ARGC ];
	
	CRON_init();
	
	for ( i = 0 ; i < MAX_CRON ; i++ )
	{
		sprintf_P( string, entrystring, i );
		if ( readConfig( string, string ) != -1 )
		{
			argc = CRON_pharseentry( string, argv );
			if ( argc == MAX_CRON_ARGC )
			{
				crontable[ entry ].CMD_ENTRY = i ;
				crontable[ entry ].hh = atoi( argv[ 0 ] );
				crontable[ entry ].mm = atoi( argv[ 1 ] );
				entry++;
			}
		}		
	}
}
//@}

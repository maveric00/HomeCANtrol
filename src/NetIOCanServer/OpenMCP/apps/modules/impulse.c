/***************************************************************************
 *            impulse.c
 *
 *  Tue Nov 17 18:31:24 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "config.h"

#if defined(IMPULSCOUNTER)

#include "impulse.h"
// für das Einbinden des CGI
#include "apps/httpd/cgibin/cgi-bin.h" 
#include "apps/httpd/httpd2.h" 
#include "apps/httpd/httpd2_pharse.h" 

// der PinChange Treiber
#if defined(PC_INT)
	#include "hardware/pcint/pcint.h"
#else
	#error "Bitte PC_INT in der config.h aktivieren"
#endif

// Modul zum einlesen der Config aus dem Configspeicher
#include "system/config/config.h"

static long IMPULS_COUNTER;

void IMPULS_init( void )
{
	char String[16];
	char sreg_tmp;
	
	if ( readConfig_P( PSTR("IMPULS_COUNTER"), String ) != -1 )
		IMPULS_COUNTER = atol( String );

	sreg_tmp = SREG;    /* Sichern */
	cli();

	PORTB |= (1<<PB6);

	PCINT_set ( 1, IMPULS_Interrupt );	
	PCINT_enablePIN( CounterPin, 1 );
	PCINT_enablePCINT( 1 );

	SREG = sreg_tmp;

	sreg_tmp = SREG;    /* Sichern */
	cli();

	IMPULS_COUNTER = 0;	

	SREG = sreg_tmp;

#if defined(HTTPSERVER)
	cgi_RegisterCGI( cgi_impuls, PSTR("impuls.cgi") );
#endif
}

long IMPULS_getCounter( void )
{
	long temp;

	cli();
	temp = IMPULS_COUNTER;
	sei();
	
	return( temp );
}

void IMPULS_setCounter( long Impulse )
{
	cli();
	IMPULS_COUNTER = Impulse;
	sei();
}

void IMPULS_saveAll( void )
{
	char String[16];

	cli();
	ltoa( IMPULS_COUNTER , String, 10 );
	sei();

	changeConfig_P( PSTR("IMPULS_COUNTER"), String );
}

void IMPULS_setPrescaler( long Prescaler )
{
	char String[16];

	ltoa( Prescaler, String, 10 );
	changeConfig_P( PSTR("IMPULS_PRESCALER"), String );
	
}

long IMPULS_getPrescaler( void )
{
	long temp;
	char String[16];

	temp = 1;
	
	if ( readConfig_P( PSTR("IMPULS_PRESCALER"), String ) != -1 )
		temp = atol( String );

	return( temp );
}

void IMPULS_setUnit( char * Unit )
{
	changeConfig_P( PSTR("IMPULS_UNIT"), Unit );
}

char * IMPULS_getUnit( char * Unit )
{
	if ( readConfig_P( PSTR("IMPULS_UNIT"), Unit ) != -1 )
		return( Unit );

	return( NULL );
}

void IMPULS_Interrupt( void )
{
	static char PRESCALE = 0;

	if ( PRESCALE == 0 )
	{
		IMPULS_COUNTER++;
		PRESCALE = 1;
	}
	else
		PRESCALE = 0;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface für zum Anzeigen der Konfigurationsdaten im EEProm. 
 * \param 	pStruct	Struktur auf den HTTP_Request
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_impuls( void * pStruct )
{
	long PreScaler;
	char UnitString[16];
	char SaveString[16];
	
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;

	PreScaler = IMPULS_getPrescaler();

	if ( IMPULS_getUnit( UnitString ) == NULL )
		UnitString[0] = '\0';

	cgi_PrintHttpheaderStart();

/*	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"<TITLE>Impulsz&auml;hler</TITLE>"
					"</HEAD>"
					"<BODY>")); */

	if ( http_request->argc == 0 )
	{
		printf_P( PSTR("Zaehler: %ld,%03ld %s (%ld Impulse)<BR><BR>"
		               "<a href=\"impuls.cgi?config\">Eingestellungen</a>"), IMPULS_getCounter() / PreScaler, ( ( 1000 / PreScaler ) * ( IMPULS_getCounter() % PreScaler ) ), UnitString, IMPULS_getCounter() );
	}
	else 
	{
		if( PharseCheckName_P( http_request, PSTR("config") ) )
		{
			if ( readConfig_P( PSTR("IMPULS_AUTOSAVE"), SaveString ) != -1)
				if ( !strcmp_P( SaveString, PSTR("on") ) )
					sprintf_P( SaveString, PSTR("checked"));
			else
				SaveString[0] = '\0';
			
			printf_P( PSTR(	"<form action=\"impuls.cgi\">"
						   	"<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\">"
  							"<tr>"
						   	"<td align=\"right\">AutoSave aktivieren</td>"
						    "<td><input type=\"checkbox\" name=\"AUTOSAVE\" value=\"on\" %s >"
							"</td>"
							"<tr>"
						   	"<td align=\"right\">PreScaler</td>"
  							"<td><input name=\"PRESCALER\" type=\"text\" size=\"10\" value=\"%ld\" maxlength=\"10\"></td>"
  							"</tr>"
							"<tr>"
						   	"<td align=\"right\">Einheit</td>"
							"<td><input name=\"UNIT\" type=\"text\" size=\"8\" value=\"%s\" maxlength=\"8\"></td>"
  							"</tr>"
 							"<tr>"
   							"<td></td><td><input type=\"submit\" value=\" Einstellung &Uuml;bernehmen \"></td>"
  							"</tr>"
						   	"</table>"
							"</form>" ) , SaveString, PreScaler, UnitString );
		}
		else
		{
			if ( PharseCheckName_P( http_request, PSTR("PRESCALER") ) )
			{
				changeConfig_P( PSTR("IMPULS_PRESCALER"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("PRESCALER") ) ] );
			}

			if ( PharseCheckName_P( http_request, PSTR("UNIT") ) )
			{
				changeConfig_P( PSTR("IMPULS_UNIT"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("UNIT") ) ] );
			}

			if ( PharseCheckName_P( http_request, PSTR("AUTOSAVE") ) )
			{
				changeConfig_P( PSTR("IMPULS_AUTOSAVE"), http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("AUTOSAVE") ) ] );
			}
			else
			{
				changeConfig_P( PSTR("IMPULS_AUTOSAVE"), "off" );	
			}
			
			printf_P( PSTR("Einstellungen uebernommen!"));
		}
	}

	cgi_PrintHttpheaderEnd();

/*	printf_P( PSTR( "</BODY>"
					"</HTML>"
					"\r\n\r\n"));
*/
}

#endif
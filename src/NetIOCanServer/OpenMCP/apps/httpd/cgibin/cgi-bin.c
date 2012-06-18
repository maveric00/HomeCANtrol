/*! \file cgi-bin.c \brief CGI-BIN Programme und Funktionen */
//***************************************************************************
//*            cgi-bin.c
//*
//*  Sat May  10 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup cgibin CGI-BIN Programme und Funktionen (cgi-bin.c)
///	\code #include "cgi-bin.h" \endcode
///	\par Uebersicht
// \date	01-01-2010:	Umstellung der CGI-Programme auf dynamisches CGI. Jetzt werden die CGI-Module extern registriert.
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


#include "../httpd2.h" 
#include "../httpd2_pharse.h" 
#include "cgi-bin.h"
#include "config.h"

struct DYN_CGIBIN cgi_table [ MAX_CGI_ENTRYS ];

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert das CGI-BIN Modul
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_init( void )
{
	int i;

	for ( i = 0 ; i < MAX_CGI_ENTRYS ; i++ )
	{
		cgi_table[ i ].dyncgi_function = NULL;
	}
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert das CGI-BIN Modul
 * \code
 * void init_foobar_cgi( )
 * {
 *		cgi_RegisterCGI( cgi_foobar , PSTR("foobar.cgi") )
 * }
 * \endcode
 * \param 	dyncgi_function		Pointer auf die CGI-Funktion die aufgerufen werden soll
 * \param	funktionname		Pointer auf den Namen im Flash unter der das CGI aufgerufen wird
 * \returns	-1 Failed, 1 CGI eingtragen
 */
/*------------------------------------------------------------------------------------------------------------*/
int cgi_RegisterCGI( DYN_CGI_CALLBACK dyncgi_function, const prog_char * funktionname )
{
	int i,retval;

	retval = -1;
	
	for ( i = 0 ; i < MAX_CGI_ENTRYS ; i++ )
	{
		if ( cgi_table[ i ].dyncgi_function == dyncgi_function )
			break;
		
		if ( cgi_table[ i ].dyncgi_function == NULL )
		{
			cgi_table[ i ].dyncgi_function = dyncgi_function;			
			cgi_table[ i ].funktionname = funktionname;
			retval = 0;
			break;
		}
	}
	return( retval );
}
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Das CGI-Interface. Sucht nach einen entsprechnden Eintrag und führt wenn vorhanden das passende CGI aus.
 * Als Template für das einfügen eines eigenen CGI sollte folgender Code benutzt werden. Damit das passende CGI gefunden
 * und ausgeführt werden kann muss der Name und die Funktion noch dem CGI-Modul übergeben mit cgi_RegisterCGI().
 *
 * \code
 * void cgi_foobar( void * pStruct )
 * {
 *    struct HTTP_REQUEST * http_request;
 *    http_request = (struct HTTP_REQUEST *) pStruct;
 *
 *    // Hier sollte dann der eigene Code stehen, mit den obrigen Zeilen wird die Umgebung eingerichtet.
 *    // Danach stehen die Daten die übergeben werden zur Verfügung.
 *    // Eine einfache Abfrage ob z.b. eine Variable mit den Namen "foo" übergeben wurde könnte so aussehen.
 *    // Nicht zu vergessen ist, das ein kompletter HTML-Body gesendet werden muss. Alle ausgaben per printf
 *    // werden automatisch an den Client der die Abfrage gestellt hat übertragen.
 *
 *    printf_P( PSTR( "<HTML>\r\n"
 *                    "<HEAD>\r\n"));
 *
 *    if ( PharseCheckName_P ( http_request, PSTR("foo") ) )
 *    {
 *       printf_P( PSTR("Variable \"foo\" wurde gefunden, sie enthält folgenden Wert %s.\r\n"), 
				  http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("foo") ) ] );   
 *    }
 *    else
 *    {
 *       printf_P( PSTR("Keine Variable \"foo\" übergeben!\r\n"));
 *    }
 *
 *    printf_P( PSTR( "</BODY>\r\n"
 *                    "</HTML>\r\n"
 *                    "\r\n"));
 * }
 * \endcode
 * \param 	pStruct		Pointer auf Struktur auf den HTTP_Request
 * \returns	-1 Failed, 1 CGI gefunden
 */
/*------------------------------------------------------------------------------------------------------------*/
int check_cgibin( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	int i, returnvalue = -1;
	
	for( i = 0 ; i < MAX_CGI_ENTRYS ; i++ )
	{
		if ( cgi_table[ i ].funktionname == NULL )
			break;
		
		if ( !strcmp_P( http_request->GET_FILE, cgi_table[ i ].funktionname ) )
		{
			printf_P( PSTR(		"HTTP/1.0 200 Document follows\r\n"
								"Content-Type: text/html\r\n"
								"Keep-Alive: timeout=1, max=5\r\n"
								"Connection: close\r\n"
								"\r\n"));
//			STDOUT_Flush();
			cgi_table[ i ].dyncgi_function( http_request );
			returnvalue = 1 ;
			break;
		}
	}
	return( returnvalue );
}	

void cgi_PrintHttpheaderStart( void )
{
	printf_P( PSTR(	"<HTML>"
					"<HEAD>"
					"</HEAD>"
					"<BODY>"));
}

void cgi_PrintHttpheaderEnd( void )
{
	printf_P( PSTR(	"</BODY>"
					"</HTML>"
					"\r\n\r\n"));
}
//@}

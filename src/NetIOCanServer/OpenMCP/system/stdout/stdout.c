/*! \file stdout.c \brief STDOUT-Funktion um die Ausgaben umzulenken */
//***************************************************************************
//*            stdout.c
//*
//*  Sat July  13 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup stdout Stdout Funktionen (stdout.c)
///	\code #include "stdout.h" \endcode
///	\par Uebersicht
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
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "stdout.h"
#include "config.h"
#include "hardware/uart/uart.h"
#if defined(TCP)
	#include "system/net/tcp.h"
#endif
#if defined(LCD)
	#include "hardware/lcd/lcd.h"
#endif
#if defined(LEDTAFEL)
	#include "system/led-tafel/tafel.h"
#endif

#define STDIO_BUFFER 64

static unsigned char BUFFER[ STDIO_BUFFER ];
 
struct STDOUT streamout;

// stdout auf UART_Send_Byte verbiegen
static FILE mystdout = FDEV_SETUP_STREAM( STDOUT_Send_Byte , STDOUT_Get_Byte , _FDEV_SETUP_RW );

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die StandartOut.
 * \retval	void		NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void STDOUT_INIT( void )
{
	// printf auf uart umbiegen
	stdout = stdin = &mystdout;
	streamout.TYPE = UNKNOWN;
	streamout.BUFFER = BUFFER ;
	streamout.BUFFER_POS = 0 ;
	streamout.XPOS = 1;
	streamout.YPOS = 1;
	streamout.SEND = NULL;
	streamout.GET = NULL;
}	

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet ein Byte auf den StandartOut Stream
 * \param   Byte		Byte welchen gesendet werden soll.
 * \param   stream		Pointer auf den Stream.
 * \retval  int			keiner.
/*------------------------------------------------------------------------------------------------------------*/
int STDOUT_Send_Byte ( char Byte, FILE * stream )
{
	switch ( streamout.TYPE )
	{
		
		case RS232:		UART_Send_Byte ( Byte );
						break;		
#if defined(TCP)
		case _TCP:		if ( streamout.BUFFER_POS == STDIO_BUFFER )
							STDOUT_Flush();
		
						streamout.BUFFER[ streamout.BUFFER_POS++ ] = Byte;
						break;
#endif
#if defined(LCD)
	#if defined(__AVR_ATmega644P__) && defined(myAVR)
		case _LCD:		LCD_sendCHAR( Byte );
	#else
		#error "LCD Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
#if defined(LEDTAFEL)
		case TAFEL:		if ( Byte == '\n' )
						{
							STDOUT_Flush();
							streamout.YPOS++;
							if ( streamout.YPOS >= TafelYlen )
								streamout.YPOS = 1;
							break;
						}
						
						if ( streamout.BUFFER_POS < TafelXlen )
						{
							if ( Byte >= 0x20 && Byte != '\r' )
							{
								streamout.BUFFER[ streamout.BUFFER_POS++ ] = Byte;
								streamout.BUFFER[ streamout.BUFFER_POS ] = '\0';
							}
						}
						break;
#endif
		default:		break;
	}
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt ein Byte von der StandartOut.
 * \param   stream		Pointer auf den Stream über den sendet werden soll.
 * \retval  int			Das Byte oder End Of File (EOF). Derzeit wird immer EOF zurück geliefert.
/*------------------------------------------------------------------------------------------------------------*/
int STDOUT_Get_Byte ( FILE * stream )
{
/*	int Data = 0;
	
	switch ( streamout.TYPE )
	{
		case RS232:		UART_Get_Byte ( (char *) Data );
						break;		
		#ifdef TCP
		case _TCP:		if ( streamout.BUFFER_POS == MAX_TCP_Datalenght )
							STDOUT_Flush();		
						streamout.BUFFER[ streamout.BUFFER_POS++ ] = Byte;
						break;
		#endif
		default:		break;
	} */
	return( EOF );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Setzt eine neue StandartOut.
 * \code
 *
 *		// Neuen Stream einstellen, z.b. eine TCP-Verbindung
 *		STDOUT_set( _TCP , SOCKET );
 *
 * \endcode
 * \param 	TYPE		Typ auf den umgebogen werden soll.
 * \param 	DEVICE		Devicenummer oder Socket auf den umgebogen werden soll.
 * \return	int			0 alles Okay. Fehler gleich -1.
 */
/*------------------------------------------------------------------------------------------------------------*/
int STDOUT_set( char TYPE, int DEVICE )
{
	STDOUT_Flush();
	
	streamout.TYPE = TYPE;
	streamout.DEVICE = DEVICE;
	streamout.BUFFER_POS = 0;
	
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sichert die aktuelle Standart Out.
 * \code
 *
 *		// Struktur anlegen für STDOUT
 *		struct STDOUT oldstream;
 *
 *		// alte Struktur sichern
 *		STDOUT_save( &oldstream );
 *
 * \endcode
 * \param 	pStruct		Pointer auf Struktur vom Typ streamout in den der aktuelle Standart Out gesichert werden soll.
 * \return	int			0 alles Okay.
 */
/*------------------------------------------------------------------------------------------------------------*/
int STDOUT_save( void * pStruct )
{
	STDOUT_Flush();
	memcpy( pStruct, &streamout, sizeof( streamout ) );
	return( 0 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Stelle den gesicherten Standart Out wieder her.
 * \code *
 *		// Struktur anlegen für STDOUT
 *		struct STDOUT oldstream;
 *
 *		// alte Struktur sichern
 *		STDOUT_save( &oldstream );
 *		// Neuen Stream einstellen, z.b. eine TCP-Verbindung
 *		STDOUT_set( _TCP , SOCKET );
 *
 *		// mach mal nen paar andere Sachen
 *		foorbar();
 *
 *		STDOUT_restore( &oldstream );
 *
 * \endcode
 * \param 	pStruct		Pointer auf Struktur vom Typ streamout in den der wieder hergestellt werden soll.
 * \return	int			0 alles Okay.
 */
/*------------------------------------------------------------------------------------------------------------*/
int STDOUT_restore( void * pStruct )
{
	STDOUT_Flush();
	memcpy( &streamout, pStruct, sizeof( streamout ) );
	return( 0 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schickt alle Daten die sich um Puffer befinden könnten raus.
 */
/*------------------------------------------------------------------------------------------------------------*/
void STDOUT_Flush( void )
{
	switch( streamout.TYPE )
	{
		case	RS232:		break;
#ifdef TCP
		case	_TCP:		if ( streamout.BUFFER_POS != 0 )
								PutSocketData_RPE ( streamout.DEVICE, streamout.BUFFER_POS, (unsigned char *) streamout.BUFFER, RAM );
							streamout.BUFFER_POS = 0;
							break;
#endif

#if defined(LCD)
	#if defined(__AVR_ATmega644P__) && defined(myAVR)
		case	_LCD:		break;
	#else
		#error "LCD Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
#if defined(LEDTAFEL)
		case	TAFEL:		if ( streamout.BUFFER_POS != 0 )
							{	
								tafel_print ( 2 , 1 , streamout.YPOS , streamout.XPOS , streamout.BUFFER );
								streamout.BUFFER_POS = 0;
								streamout.BUFFER[0] = 0 ;
							}
							break;
#endif
		default:			break;
	}
}

void STDOUT_SetXY( char x, char y )
{
	switch( streamout.TYPE )
	{
		case	RS232:		break;
		case	_TCP:		break;
#if defined(LEDTAFEL)
		case	TAFEL:		if ( x > 0 && x <= TafelXlen )
								streamout.XPOS = x;
							else
								streamout.XPOS = 1;
		
							if ( y > 0 && y <= TafelYlen )
								streamout.YPOS = y;
							else
								streamout.YPOS = 1;
							break;
#endif
		default:			break;
	}
}

//@}

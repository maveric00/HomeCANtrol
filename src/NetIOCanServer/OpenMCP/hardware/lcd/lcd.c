/*!\file lcd.c \brief Stellt Funktionen für ein LCD bereit */
/***************************************************************************
 *            lcd.c
 *
 *  Thu Nov  5 17:02:56 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup hardware
///	\defgroup lcd Stellt Funktionen für ein LCD bereit (lcd.c)
///	\code #include "lcd.h" \endcode
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

#include "config.h"

#if defined(LCD)

#include "system/clock/clock.h"
#include "lcd.h"
#include "hardware/timer1/timer1.h"
#include "config.h"

static char	row;
static char	column;

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Initialisiert das LCD.
 * \param   NONE		None
 * \return	NONE		None
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void LCD_init( void )
{
#if defined(myAVR_LCD)

	unsigned char i;

	// LCD-Spannung erzeugen am Ausgang OC2a
    PORTB &= ~(1<<PD3);  // clear port
    DDRB |= (1<<PD3);  // set as output

    TCCR0A = ( 0<<WGM21 ) | ( 1<<WGM20 ) | ( 1<<COM2A1 );
    TCCR0B = ( 0<<WGM22 ) | ( 1<<CS20 );

    TCNT0=0;

    OCR0A=0;
	
	// die 4 Ausgänge für Daten ans das Display auf 0 setzen
	DATAPORT_1 |= ( DATAMASK_1 );
	DATADDR_1 |= ( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );
	DATADDR_2 |= ( DATAMASK_2 );

	// Enable auf 1 setzen
	E_PORT |= ( 1<<E_PIN );
	E_DDR |= ( 1<<E_PIN );
	
	// Register Select auf 1 setzen
	RS_PORT |= ( 1<<RS_PIN );
	RS_DDR |= ( 1<<RS_PIN );

	// erster Reset in den 8-Bit Modus
	RS_PORT &= ~( 1<<RS_PIN );
	DATAPORT_1 &= ~( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );
	DATAPORT_1 |= ( DATAMASK_1 & ( 0x30<<2 ) );
	DATAPORT_2 |= ( DATAMASK_2 & ( 0x30>>6 ) );
	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );
	CLOCK_delay( 10 );

	// zweiter Reset in den 8-Bit Modus
	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );
	CLOCK_delay( 10 );

	// und jetzt endlich in den 4-Bit Modus
	DATAPORT_1 &= ~( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );
	DATAPORT_1 |= ( DATAMASK_1 & ( 0x20<<2 ) );
	DATAPORT_2 |= ( DATAMASK_2 & ( 0x20>>6 ) );
	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );
	CLOCK_delay( 10 );
#endif
	
	// Display einstellen, 4Bit, 2 Lines und Zeichensatz 5x8 Pixel
	LCD_sendCMD( LCD_CMD_4BIT | LCD_CMD_2LINES | LCD_CMD_5x8 ); // Zwei Zeilen, 5x8
	// clear screen
	LCD_sendCMD( LCD_CMD_CLR );
	// Cursor auf Homeposition setzen
	LCD_sendCMD( LCD_CMD_HOME );
	// Display einschalten
	LCD_sendCMD( LCD_CMD_ONOFF );

	row = 0;
	column = 0;

#if defined(myAVR_LCD)
	// Backlight auf 100% dimmen
	for ( i = 0 ; i < 255 ; i++ )
	{
        OCR0A=i;
		CLOCK_delay( 10 );
	}	
    OCR0A=255;
#endif
	
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Sendet Kommandos ans Display.
 * \param   CMD			Kommando fürs Display im Datamodus
 * \return	NONE		None
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void LCD_sendCMD( unsigned char CMD )
{
#if defined(myAVR_LCD)
	RS_PORT &= ~( 1<<RS_PIN );

	DATAPORT_1 &= ~( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );

	DATAPORT_1 |= ( DATAMASK_1 & ( CMD<<2 ) );
	DATAPORT_2 |= ( DATAMASK_2 & ( CMD>>6 ) );

	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );

	DATAPORT_1 &= ~( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );

	DATAPORT_1 |= ( DATAMASK_1 & ( CMD<<6 ) );
	DATAPORT_2 |= ( DATAMASK_2 & ( CMD>>2 ) );

	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );

	CLOCK_delay( 10 );
#endif
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Sendet Daten ans Display.
 * \param   DATA		Daten fürs Display im Datamodus
 * \return	NONE		None
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void LCD_sendDATA( unsigned char DATA )
{
#if defined(myAVR_LCD)
	RS_PORT |= ( 1<<RS_PIN );

	DATAPORT_1 &= ~( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );

	DATAPORT_1 |= ( DATAMASK_1 & ( DATA<<2 ) );
	DATAPORT_2 |= ( DATAMASK_2 & ( DATA>>6 ) );

	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );

	DATAPORT_1 &= ~( DATAMASK_1 );
	DATAPORT_2 &= ~( DATAMASK_2 );

	DATAPORT_1 |= ( DATAMASK_1 & ( DATA<<6 ) );
	DATAPORT_2 |= ( DATAMASK_2 & ( DATA>>2 ) );

	E_PORT &= ~( 1<<E_PIN );
	E_PORT |= ( 1<<E_PIN );

	CLOCK_delay( 10 );
#endif
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Löscht das Display und setzt den Cursor auf die Homeposition.
 * \param   NONE		None
 * \return	NONE		None
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void LCD_clrscreen( void )
{
	LCD_sendCMD( LCD_CMD_CLR );
	LCD_sendCMD( LCD_CMD_HOME );
	row = 0;
	column = 0;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Setzt den Cursor auf die angebene Position.
 * \param   XPos		Die X-Position des Cursors.
 * \param   YPos		Die Y-Position des Cursors.
 * \return	NONE		None
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void LCD_setXY( char XPos, char YPos )
{
	row = YPos;
	column = XPos;
	
	switch( YPos )
	{
		case	0:		LCD_sendCMD( LCD_CMD_DDRAM + LCD_ROW_1 + XPos );
						break;
		case	1:		LCD_sendCMD( LCD_CMD_DDRAM + LCD_ROW_2 + XPos );
						break;
#if defined(LCD_4x16)
		case	2:		LCD_sendCMD( LCD_CMD_DDRAM + LCD_ROW_3 + XPos );
						break;
		case	3:		LCD_sendCMD( LCD_CMD_DDRAM + LCD_ROW_4 + XPos );
						break;
#endif
		default:		break;
	}
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Sendet ein Zeichen ans Display.
 * \param   DATA		Zeichen das ausgeben werden soll.
 * \return	NONE		None
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void LCD_sendCHAR( char DATA )
{	 
	// Wenn es ein Steuerzeichen ist, ignorieren
	if ( DATA < 10)
		return;

	// Wenn Return, dann zum Zeilenanfang springen
	if ( DATA == '\r' )
	{
		LCD_setXY( 0, row );
		return;
	}
	
	// Wenn Newline, dann eine Zeil weiter runter springen
	if ( DATA == '\n' )
	{
		row++;
		if ( row < LCD_Rows )
			LCD_setXY( 0, row );
		else
			LCD_clrscreen();
		return;
	}

	// Sind wir schon am rechten Rand? Wenn nein, weiter, sonst Zeilensprung
	if ( column < LCD_Columns )
	{
		LCD_sendDATA( DATA );
		column++;
	}
	else
	{
		// eine Zeile weiter
		row++;
		// Sind wir schon am unteren Rand? Wenn nein, eine Zeile weiter
		// sonst Display löschen und wieder oben anfangen
		if ( row < LCD_Rows )
		{
			column = 0;
			LCD_setXY( column, row );
			LCD_sendDATA( DATA );
			column++;
		}
		else
			LCD_clrscreen();
	}
}

#endif

//@}


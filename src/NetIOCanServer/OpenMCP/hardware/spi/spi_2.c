/*! \file spi_2.c \brief Stellt die SPI2-Schnittstelle bereit */
//***************************************************************************
//*            spi_2.c
//*
//*  Mon May 5 21:46:47 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
///	\ingroup hardware
///	\defgroup SPI2 Die SPI2-Schnittstelle (spi_2.c)
///	\code #include "spi_2.h" \endcode
///	\par Uebersicht
///		Die SPI2-Schnittstelle fuer den AVR-Controller. Der SPI-Bus ist rein
///	Software basierend. Es wird kein Hardware-SPI gebraucht.
//****************************************************************************/
//@{
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
#include "spi_2.h"
#include <avr/io.h>

#include "system/clock/clock.h"

/* -----------------------------------------------------------------------------------------------------------*/
/*! Die Init fuer dir SPI-Schnittstelle.
 * \param 	Options		Hier kann die Geschwindigkeit der SPI eingestellt werden.
 * \retval	Null wenn alles richtig eingestellt wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI2_init( void )
{

	// SCK auf Low setzen
	SPI2_PORT &= ~( 1<<SCK2 );
	// MOSI, SCK, SS als Output
	SPI2_DDR  |= 1<<MOSI2 | 1<<SCK2; // mosi, sck, ss output
	// MISO als Input
	SPI2_DDR  &= ~( 1<<MISO2 ); // miso input
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Schreibt einen Wert auf den SPI-Bus. Gleichzeitig wird ein Wert von diesem im Takt eingelesen.
 * \warning	Auf den SPI-Bus sollte vorher per Chip-select ein Baustein ausgewaehlt werden. Dies geschied nicht in der SPI-Routine sondern
 * muss von der Aufrufenden Funktion gemacht werden.
 * \param 	Data	Der Wert der uebertragen werden soll.
 * \retval  Data	Der wert der gleichzeit empfangen wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/
char SPI2_ReadWrite( char Data )
{
	unsigned char i, InData = 0;
		
	for( i = 0 ; i < 8 ; i++ )
	{
		// InDaten shiften;
		InData = InData<<1;
		
		// Kieck mal ob MSB 1 ist
		if ( ( Data & 0x80 ) > 0 )
			SPI2_PORT |= ( 1<<MOSI2 );
		else
			SPI2_PORT &= ~( 1<<MOSI2 );
		
		// SCK auf High setzen
		SPI2_PORT |= ( 1<<SCK2 );
				
		// MISO einlesen
		if ( bit_is_set( SPI2_PIN, MISO2 ) > 0 )
			InData |= 1;
		
		// SCK auf Low setzen
		SPI2_PORT &= ~( 1<<SCK2 );
		
		Data <<= 1;
	}
	
	return( InData );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle MEM->SPI Blocksende Routine mit optimierungen auf Speed.
 * \param	buffer		Zeiger auf den Puffer der gesendet werden soll.
 * \param	Datalenght	Anzahl der Bytes die gesedet werden soll.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI2_FastMem2Write( char * buffer, int Datalenght )
{
	unsigned int Counter = 0;
	unsigned char data,i;
	
	while( Counter < Datalenght )
	{
		data = buffer[ Counter ];
		
		for( i = 0 ; i < 8 ; i++ )
		{
			// Kieck mal ob MSB 1 ist
			if ( ( data & 0x80 ) > 0 )
				SPI2_PORT |= ( 1<<MOSI2 );
			else
				SPI2_PORT &= ~( 1<<MOSI2 );
			
			// SCK auf High setzen
			SPI2_PORT |= ( 1<<SCK2 );
			
			data <<= 1;

			// SCK auf Low setzen
			SPI2_PORT &= ~( 1<<SCK2 );

		}
		
		Counter++;
		
	}
	return;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle SPI->MEM Blockempfangroutine mit optimierungen auf Speed.
 * \warning Auf einigen Controller laufen die Optimierungen nicht richtig. Bitte teil des Sourcecode der dies verursacht ist auskommentiert.
 * \param	buffer		Zeiger auf den Puffer wohin die Daten geschrieben werden sollen.
 * \param	Datalenght	Anzahl der Bytes die empfangen werden sollen.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI2_FastRead2Mem( char * buffer, int Datalenght )
{
	unsigned int Counter = 0;
	unsigned char InData = 0,i;
	
	SPI2_PORT &= ~( 1<<MOSI2 );

	while( Counter < Datalenght )
	{
		for( i = 0 ; i < 8 ; i++ )
		{
			// InDaten shiften;
			InData = InData<<1;
			
			// SCK auf High setzen
			SPI2_PORT |= ( 1<<SCK2 );
			
			// MISO einlesen
			if ( bit_is_set( PINB,MISO2 ) > 0 )
				InData |= 1;
		
			// SCK auf Low setzen
			SPI2_PORT &= ~( 1<<SCK2 );
		}
		*buffer++ = InData;
		Counter++;
	}
	return;
}
//@}

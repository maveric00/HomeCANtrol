/*! \file spi_1.c \brief Stellt die SPI1-Schnittstelle bereit */
//***************************************************************************
//*            spi_1.c
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
///	\ingroup hardware
///	\defgroup SPI1 Die SPI1-Schnittstelle (spi_1.c)
///	\code #include "spi_1.h" \endcode
///	\par Uebersicht
///		Die SPI1-Schnittstelle fuer den AVR-Controller.
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

#include "spi_1.h"

/* -----------------------------------------------------------------------------------------------------------*/
/*! Die Init fuer dir SPI-Schnittstelle.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI1_init( void )
{
#ifdef __AVR_ATmega2561__
		UBRR0 = 0;
		/* Setting the XCKn port pin as output, enables master mode. */
		SPI_DDR |= ( 1 << XCK1 );
		/* Set MSPI mode of operation and SPI data mode 0. */
		UCSR0C = (1<<UMSEL01)|(1<<UMSEL00);
		/* Enable receiver and transmitter. */
		UCSR0B = (1<<RXEN0)|(1<<TXEN0);
		/* Set baud rate. */
		/* IMPORTANT: The Baud Rate must be set after the transmitter is enabled */
		UBRR0 = 0;
#endif
#ifdef __AVR_ATmega644P__
		UBRR1 = 0;
		/* Setting the XCKn port pin as output, enables master mode. */
		SPI_DDR |= ( 1 << XCK1 );
		/* Set MSPI mode of operation and SPI data mode 0. */
		UCSR1C = (1<<UMSEL11)|(1<<UMSEL10);
		/* Enable receiver and transmitter. */
		UCSR1B = (1<<RXEN1)|(1<<TXEN1);
		/* Set baud rate. */
		/* IMPORTANT: The Baud Rate must be set after the transmitter is enabled */
		UBRR1 = 0;
#endif
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Schreibt einen Wert auf den SPI-Bus. Gleichzeitig wird ein Wert von diesem im Takt eingelesen.
 * \warning	Auf den SPI-Bus sollte vorher per Chip-select ein Baustein ausgewaehlt werden. Dies geschied nicht in der SPI-Routine sondern
 * muss von der Aufrufenden Funktion gemacht werden.
 * \param 	Data	Der Wert der uebertragen werden soll.
 * \retval  Data	Der wert der gleichzeit empfangen wurde.
 */
/* -----------------------------------------------------------------------------------------------------------*/
char SPI1_ReadWrite( char Data)
{
#ifdef __AVR_ATmega2561__
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) );
	/* Put data into buffer, sends the data */
	UDR0 = Data;
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) );
	/* Get and return received data from buffer */
	return UDR0;
#endif
#ifdef __AVR_ATmega644P__
	/* Wait for empty transmit buffer */
	while ( !( UCSR1A & (1<<UDRE1)) );
	/* Put data into buffer, sends the data */
	UDR1 = Data;
	/* Wait for data to be received */
	while ( !(UCSR1A & (1<<RXC1)) );
	/* Get and return received data from buffer */
	return UDR1;
#endif
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle MEM->SPI Blocksende Routine mit optimierungen auf Speed.
 * \param	buffer		Zeiger auf den Puffer der gesendet werden soll.
 * \param	Datalenght	Anzahl der Bytes die gesedet werden soll.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI1_FastMem2Write( char * buffer, int Datalenght )
{
	int Counter = 0;
	char data;
	
#ifdef __AVR_ATmega2561__
	// erten Wert senden
	UDR0 = buffer[ Counter++ ];
	while( Counter < Datalenght )
	{
		// Wert schon mal in Register holen, schneller da der Wert jetzt in einem Register steht und nicht mehr aus dem RAM geholt werden muss
		// nachdem das senden des vorherigen Wertes fertig ist,
		data = buffer[ Counter ];
		// warten auf fertig
		while ( !( UCSR0A & (1<<UDRE0)) );
		// Wert aus Register senden
		UDR0 = data;
		Counter++;
	}
	while ( !(UCSR0A & (1<<RXC0)) );
#endif
#ifdef __AVR_ATmega644P__
	// erten Wert senden
	UDR1 = buffer[ Counter++ ];
	while( Counter < Datalenght )
	{
		// Wert schon mal in Register holen, schneller da der Wert jetzt in einem Register steht und nicht mehr aus dem RAM geholt werden muss
		// nachdem das senden des vorherigen Wertes fertig ist,
		data = buffer[ Counter ];
		// warten auf fertig
		while ( !( UCSR1A & (1<<UDRE1)) );
		// Wert aus Register senden
		UDR1 = data;
		Counter++;
	}
	while ( !(UCSR1A & (1<<RXC1)) );
#endif
	return;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! Eine schnelle SPI->MEM Blockempfangroutine mit optimierungen auf Speed.
 * \warning Auf einigen Controller laufen die Optimierungen nicht richtig. Bitte teil des Sourcecode der dies verursacht ist auskommentiert.
 * \param	buffer		Zeiger auf den Puffer wohin die Daten geschrieben werden sollen.
 * \param	Datalenght	Anzahl der Bytes die empfangen werden sollen.
 */
/* -----------------------------------------------------------------------------------------------------------*/
void SPI1_FastRead2Mem( char * buffer, int Datalenght )
{
	int Counter = 0;
	char data;
	
#ifdef __AVR_ATmega2561__
	while( Counter <= Datalenght )
	{
		// warten auf fertig
		while ( !( UCSR0A & (1<<UDRE0)) );
		// dummywrite
		UDR0 = 0xFF;

		while ( !(UCSR0A & (1<<RXC0)) );
		// Daten einlesen
		data = UDR0;

		// speichern
		buffer[ Counter++ ] = data;
		// buffer[ Counter++ ] = SPI1_ReadWrite( 0x00 );
	}
#endif
#ifdef __AVR_ATmega644P__
	while( Counter <= Datalenght )
	{
		// warten auf fertig
		while ( !( UCSR1A & (1<<UDRE1)) );
		// dummywrite
		UDR1 = 0x00;

		while ( !(UCSR1A & (1<<RXC1)) );
		// Daten einlesen
		data = UDR1;

		// speichern
		buffer[ Counter++ ] = data;
		// buffer[ Counter++ ] = SPI1_ReadWrite( 0x00 );
	}
#endif
	return;
}
//@}

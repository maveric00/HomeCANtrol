/*!\file spi_0.h \brief Stellt die SPI0-Schnittstelle bereit*/
//***************************************************************************
//*            spi_0.h
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu
///	\ingroup hardware
///	\defgroup SPI0 Die SPI0-Schnittstelle (spi_0.h)
///	\code #include "spi_0.h" \endcode
///	\par Uebersicht
///		Die SPI0-Schnittstelle fuer den AVR-Controller
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
 
#ifndef _SPI_0_H
	#define _SPI_0_H
	
	#include <avr/io.h>

	void SPI0_init( void );
	char SPI0_ReadWrite( char Data );
	void SPI0_FastRead2Mem( char * buffer, int Datalenght );
	void SPI0_FastMem2Write( char * buffer, int Datalenght );
	
	#define SPI0_PORT		PORTB
	#define SPI0_DDR		DDRB
	
	#ifdef __AVR_ATmega2561__
		#define MISO			PB3
		#define MOSI			PB2
		#define SCK				PB1
	#endif

	#ifdef __AVR_ATmega644__
		#define MISO			PB6
		#define MOSI			PB5
		#define SCK				PB7
	#endif

	#ifdef __AVR_ATmega644P__
		#define MISO			PB6
		#define MOSI			PB5
		#define SCK				PB7
	#endif

#endif /* _SPI_0_H */
//@}

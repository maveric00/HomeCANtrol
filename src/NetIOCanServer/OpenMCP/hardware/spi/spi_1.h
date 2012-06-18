/*!\file spi_1.h \brief Stellt die SPI1-Schnittstelle bereit */
//***************************************************************************
// *            spi_1.h
// *
// *  Sat Jun  3 23:01:49 2006
// *  Copyright  2006  User
// *  Email
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
 
#ifndef _SPI_1_H
	#define SPI_1_H

	void SPI1_init( void );
	char SPI1_ReadWrite( char Data);
	void SPI1_FastMem2Write( char * buffer, int Datalenght );
	void SPI1_FastRead2Mem( char * buffer, int Datalenght );

#ifdef __AVR_ATmega2561__
	#define SPI_PORT		PORTE
	#define SPI_DDR			DDRE
	#define XCK1			PE2
#endif

#ifdef __AVR_ATmega644P__
	#define SPI_PORT		PORTD
	#define SPI_DDR			DDRD
	#define XCK1			PD4
#endif

#endif /* SPI_1_H */

 

/***************************************************************************
 *            spi_core.h
 *
 *  Sun Jan 18 11:40:26 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
///	\ingroup hardware
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
 
#ifndef _SPI_CORE_H
	#define SPI_CORE_H

	void SPI_InitCore( void );

	void SPI_init( int SPI_ID );
	char SPI_ReadWrite( int SPI_ID, char Data );
	void SPI_WriteBlock( int SPI_ID, char * Block, int len );
	void SPI_ReadBlock( int SPI_ID, char * Block, int len );

	typedef void pSPI_INIT ( void );
	typedef char pSPI_READWRITE ( char Data );
	typedef void pSPI_WRITEBLOCK ( char * Block, int len );
	typedef void pSPI_READBLOCK ( char * Block, int len );

	typedef struct {
		pSPI_INIT			* INIT;
		pSPI_READWRITE	  	* ReadWrite;
		pSPI_WRITEBLOCK	  	* WriteBlock;
		pSPI_READBLOCK	  	* ReadBlock;
	} const SPI_BUS ;		

#endif /* SPI_CORE_H */


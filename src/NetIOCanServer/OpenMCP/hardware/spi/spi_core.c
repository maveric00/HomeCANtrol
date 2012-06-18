/***************************************************************************
 *            spi_core.c
 *
 *  Sun Jan 18 11:39:35 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
///	\ingroup hardware
 ****************************************************************************/
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
 
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spi_core.h"

#ifdef __AVR_ATmega2561__
	#include "spi_0.h"
	#include "spi_1.h"
	#include "spi_2.h"

	SPI_BUS spi_bus[ ] = {
		{ SPI0_init, SPI0_ReadWrite, SPI0_FastMem2Write, SPI0_FastRead2Mem },
		{ SPI1_init, SPI1_ReadWrite, SPI1_FastMem2Write, SPI1_FastRead2Mem },
		{ SPI2_init, SPI2_ReadWrite, SPI2_FastMem2Write, SPI2_FastRead2Mem } };
#endif


#ifdef __AVR_ATmega644__
	#include "spi_0.h"
	#include "spi_2.h"

	SPI_BUS spi_bus[ ] = {
		{ SPI0_init, SPI0_ReadWrite, SPI0_FastMem2Write, SPI0_FastRead2Mem },
		{ SPI2_init, SPI2_ReadWrite, SPI2_FastMem2Write, SPI2_FastRead2Mem } };
#endif

#ifdef __AVR_ATmega644P__
	#include "spi_0.h"
	#include "spi_1.h"

	SPI_BUS spi_bus[ ] = {
		{ SPI0_init, SPI0_ReadWrite, SPI0_FastMem2Write, SPI0_FastRead2Mem },
		{ SPI1_init, SPI1_ReadWrite, SPI1_FastMem2Write, SPI1_FastRead2Mem } };
#endif

void SPI_InitCore( void )
{
	
}

void SPI_init( int SPI_ID )
{
	spi_bus[ SPI_ID ].INIT( );
}

char SPI_ReadWrite( int SPI_ID, char Data )
{
	return( spi_bus[ SPI_ID ].ReadWrite( Data ) );	
}

void SPI_WriteBlock( int SPI_ID, char * Block, int len )
{
	spi_bus[ SPI_ID ].WriteBlock( Block, len );	
}

void SPI_ReadBlock( int SPI_ID, char * Block, int len )
{
	spi_bus[ SPI_ID ].ReadBlock( Block, len );		
}
//}@

/*! \file vs10xx.c \brief Stellt Funktionen für den VS10xx Decoder bereit */
//***************************************************************************
//*            vs10xx.c
//*
//*  Mon May 12 17:46:47 2008
//*  Copyright  2008 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup hardware
///	\defgroup VS10xx Funktionen für den VS10xx (vs10xx.c)
///	\code #include "vs10xx.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen für den MP3-Decoder VS10xx von VLSI bereit. 
///	Unter www.vlsi.fi finden sich weiter Beispiele zu weiteren Decoder von
/// VLSI. Einige Funktionen entstammen dem Yampp-Projekt und sind an diese
///	angelehnt.
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
#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>

#include "config.h"

#if defined(VS10XX)

#include "vs10xx.h"
#include "system/clock/clock.h"
#include "system/buffer/fifo.h"
#include "hardware/spi/spi_2.h"
#include "hardware/spi/spi_0.h"
#include "hardware/spi/spi_core.h"

static int SPI_BUSID = -1;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert den VS1001k.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char VS10xx_INIT( void )
{

	printf_P( PSTR("VS10xx initialisieren, found an "));

	// Steuerleitungen einrichten für MP3-Decoder
	POWER_DDR |= ( 1<<POWER );
	RESET_DDR |= ( 1<<RESET );
	BSYNC_DDR |= ( 1<<BSYNC );
	DREQ_DDR &= ~( 1<<DREQ );
	SS2_DDR |= ( 1<<SS2 );
	
	// Steuerleitungen auf Startzustand setzen
	RESET_PORT |= ( 1<<RESET );
	BSYNC_PORT &= ~( 1<<BSYNC );
	DREQ_PORT &= ~( 1<<DREQ );
	SS2_PORT |= ( 1<<SS2 );
		
	SPI_BUSID = 2;
	SPI_init( SPI_BUSID );

	// MP3-Decoder einschalten
	POWER_PORT |= ( 1<<POWER );	

	// 100ms warten bis Spannung stabil ist und Controller sie initialisiert hat
	CLOCK_delay( 100 );
	
	// reset the decoder
	VS10xx_reset( );

	VS10xx_set_xtal ( VS10xx_CLOCKF );
	// Wenn xtal nicht erfolgreich gesetzt ist REv.2 der Hardware vorhanden und muss per 
	// HardwareSPI angesteuert werden
	if ( VS10xx_read( VS10xx_Register_CLOCKF ) != VS10xx_CLOCKF )
	{
		SPI_BUSID = 2;
		SPI_init( SPI_BUSID );

		// reset the decoder
		CLOCK_delay(30);

		VS10xx_set_xtal ( VS10xx_CLOCKF );

		if ( VS10xx_read( VS10xx_Register_CLOCKF ) != VS10xx_CLOCKF )
		{
			printf_P( PSTR("failed\r\n") );
			SPI_BUSID = -1;
			return( RESET_FAILED );
		}
	}

	VS10xx_vol ( 255, 255 );

	switch( VS10xx_GetVersion() )
	{
		case 0: 	printf_P( PSTR("VS1001,"));
					break;
		case 1: 	printf_P( PSTR("VS1011,"));
					break;
		case 2: 	printf_P( PSTR("VS1002 or VS1011e,"));
					break;
		case 3: 	printf_P( PSTR("VS1003,"));
					break;
		default:	break;
	}
	
	printf_P( PSTR(" Clockspeed = %d.%03dMHz\r\n"), (VS10xx_read( VS10xx_Register_CLOCKF ) * 2)/1000 ,  (VS10xx_read( VS10xx_Register_CLOCKF ) * 2)%1000 );

	return( RESET_OK );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt den Typ des Decoders.
 * \param 	NONE
 * \return	Version
 */
/*------------------------------------------------------------------------------------------------------------*/

int VS10xx_GetVersion( void )
{
	int Version;
	
	Version = VS10xx_read( VS10xx_Register_STATUS ) & ( 5<<5 );
	
	return( Version );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Liest ein Register vom VS10xx aus.
 * \param 	address	Registernummer im VS10xx.
 * \return	Value	Der Ihanlt des 16-Bit-Register.
 */
/*------------------------------------------------------------------------------------------------------------*/
int VS10xx_read( char address)
{
	unsigned int Data;
	
	SS2_PORT &= ~( 1<<SS2 );
	
	SPI_ReadWrite( SPI_BUSID , VS10xx_READ);
	SPI_ReadWrite( SPI_BUSID ,address);

	Data = SPI_ReadWrite( SPI_BUSID , 0) << 8;
	Data |= SPI_ReadWrite( SPI_BUSID , 0);
	
	SS2_PORT |= ( 1<<SS2 );
	
	return( Data );
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Beschreibt ein Register im VS10xx.
 * \param 	adress		Registernummer im VS10xx.
 * \param 	Data		Daten die ins Register sollen.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_write( char address, int Data)
{
	SS2_PORT &= ~( 1<<SS2 );

	SPI_ReadWrite( SPI_BUSID , VS10xx_WRITE);
	SPI_ReadWrite( SPI_BUSID , address);
	
	SPI_ReadWrite( SPI_BUSID , ( char) (Data >> 8));
	SPI_ReadWrite( SPI_BUSID , ( char) (Data ));
	
	SS2_PORT |= ( 1<<SS2 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Befüllt den FIFO des Decoders des VS10xx
 * \param 	FIFO		Die Nummer des FIFO aus dem der VS10xx gefüllt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_flush_from_FIFO( int FIFO )
{

	BSYNC_PORT |= ( 1<<BSYNC );

	char buffer[32];
	
	while( bit_is_set( DREQ_PIN , DREQ ) && Get_Bytes_in_FIFO ( FIFO ) >= 32 )
	{
		Get_Block_from_FIFO ( FIFO, 32, buffer );
		SPI_WriteBlock (  SPI_BUSID , buffer, 32 );
	}

	BSYNC_PORT &= ~( 1<<BSYNC );

}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet 32Byte an den FIFO des  Decoders.
 * \param 	FIFO		Die Nummer des FIFO aus dem der VS10xx gefüllt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_send_32_from_FIFO( int FIFO )
{

	BSYNC_PORT |= ( 1<<BSYNC );

	int j;
	
	for (j=0;j<32;j++)
	{
		SPI_ReadWrite( SPI_BUSID , Get_Byte_from_FIFO ( FIFO ) );
	}

	BSYNC_PORT &= ~( 1<<BSYNC );

}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet 32Byte an den FIFO des  Decoders.
 * \param 	pBuffer		Pointer auf einen 32 Byte größen Puffer.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_send_32( char *pBuffer )
{

	BSYNC_PORT |= ( 1<<BSYNC );

	int j;
	
	for (j=0;j<32;j++)
	{
		SPI_ReadWrite( SPI_BUSID , *pBuffer );
		pBuffer++;
	}

	BSYNC_PORT &= ~( 1<<BSYNC );

}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet ein Byte an den FIFO des VS10xx.
 * \param 	Data		Das Byte welches rein soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_send_data( char Data )
{
	
	BSYNC_PORT |= ( 1<<BSYNC );

	SPI_ReadWrite( SPI_BUSID , Data );		// send data

	BSYNC_PORT &= ~( 1<<BSYNC );

}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert den VS1001k.
 * \param 	reset_e r	Softreset oder Hardwarereset das ist hier die frage.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char VS10xx_reset( void )
{

	RESET_PORT &= ~( 1<<RESET );
	CLOCK_delay(10);	// 10 mS	    
	RESET_PORT |= ( 1<<RESET );
	CLOCK_delay(50);	// 50 mS	  
	
	VS10xx_set_xtal( VS10xx_CLOCKF );

	return( RESET_OK );
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schreibt anzahl von nullen an den VS10xx, wird für reset gebraucht.
 * \param 	nNulls		Anzahl der Nullen die geschrieben werden sollen.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_nulls( int nNulls)
{
	while (nNulls--)
		VS10xx_send_data(0);
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Sinustest ein.
 * \param 	freq		Die Frequenz, sieht Datenblatt.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_sine_on( char freq)
{
	unsigned char i,buf[4];
	
	// sine on
	buf[0] = 0x53;	buf[1] = 0xEF;	buf[2] = 0x6E;	buf[3] = freq;

	for (i=0;i<4;i++)
		VS10xx_send_data(buf[i]);
	VS10xx_nulls(4);
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schlatet den Sinustest aus.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_sine_off(void)
{
	unsigned char i,buf[4];

	// sine off
	buf[0] = 0x45;	buf[1] = 0x78;	buf[2] = 0x69;	buf[3] = 0x74;

	for (i=0;i<4;i++)
		VS10xx_send_data(buf[i]);
	VS10xx_nulls(4);
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Durchlaufender Sinustest.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char VS10xx_sine_sweep(void)
{
	char i;

	for (i=48;i<119;i++)
	{
		VS10xx_sine_off();
		VS10xx_sine_on(i);	
		CLOCK_delay(250);
	}
	VS10xx_sine_off();
	return 0;	
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Setzt die Lautstärke des Decoders. 0-255.
 * \param 	Lvol	Linke Seite die Laustärke.
 * \param 	Rvol	Rechte Seite die Laustärke.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_vol( unsigned char Lvol, unsigned char Rvol )
{
	VS10xx_write ( VS10xx_Register_VOL, ( ( 255 - Lvol )<<8 ) | ( 255 - Rvol ) );
}


/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Setzt die Frequenz des XTAL.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void VS10xx_set_xtal( int clock )
{
	VS10xx_write( VS10xx_Register_CLOCKF, clock );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Zeit die der Decoder schon decodiert.
 * \param 	NONE
 * \return	time	Zeit in Sekunden.
 */
/*------------------------------------------------------------------------------------------------------------*/
int VS10xx_get_decodetime( void )
{
	return ( VS10xx_read( VS10xx_Register_DECODE_TIME ) );
}

#endif

//@}

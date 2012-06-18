/*! \file led-tafel.c \brief LED-Tafel Ansteuerung */
//***************************************************************************
//*            led-tafel.c
//*
//*  Mon Aug 29 19:19:16 2008
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup ledtafel LED-Tafel Ansteuerung (led-tafel.c)
///	\code #include "led-tafel.h" \endcode
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
#include <avr/pgmspace.h>

#include "system/stdout/stdout.h"
#include "system/clock/clock.h"
#include "hardware/uart/uart.h"

#include "config.h"

#if defined(LEDTAFEL)

#include "tafel.h"


/*------------------------------------------------------------------------------------------------------------*/
/* \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char tafel_init( void )
{
	CLOCK_init();

	STDOUT_set( TAFEL, 0);

	if ( tafel_wait4ack ( 30 ) == 0xff )
	{
		printf_P( PSTR("Warmstart!!! \r\n"));
		STDOUT_INIT ();
		tafel_clr ();
	}
	else
	{
		printf_P( PSTR("Kaltstart!!! \r\n"));
		STDOUT_INIT ();
		tafel_clr ();
	}
	
	STDOUT_set( TAFEL, 0 );

	return(0);
	
}

/*------------------------------------------------------------------------------------------------------------*/
/* \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char tafel_print (int tafel, int seite, int zeile, int spalte,unsigned char *string)
{

	int len = strlen(string), i;
	unsigned char header[6], checksum;
	static unsigned char etx = CMD_ETX;
	static unsigned char eot = CMD_EOT;
	
	header[0] = CMD_SOH;
	header[1] = tafel + 0x20;
	header[2] = seite + 0x20;
	header[3] = zeile + 0x20;
	header[4] = spalte + 0x20;
	header[5] = CMD_STX;
   
	checksum = header[0] + header[1] + header[2] + header[3] + header[4] + header[5];
   
	for (i=0; i < 6; i++)
    	UART_Send_Byte (header[i]);
	
	for (i=0; i < len; i++)
    	UART_Send_Byte (string[i]);
	   
	UART_Send_Byte (etx);
   
	for (i=0; i < len; i++)
    	checksum += string[i];

	checksum += etx;
      
	UART_Send_Byte (checksum);
	UART_Send_Byte (eot);
   	
	return( tafel_wait4ack( 30 ) );
	// return( 0 );
}

char tafel_wait4ack( int timeout )
{
	char byte = 0, buffer[ ackbufferlen ] ;
	int i = 0;
	
	unsigned char timer;
	
	timer = CLOCK_RegisterCoundowntimer( );
	CLOCK_SetCountdownTimer ( timer , timeout , SECOUND );
	
	do
	{
		if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
		{
			CLOCK_ReleaseCountdownTimer( timer );
			return( 0xff );
		}
		
		if ( UART_Get_Bytes_in_Buffer() != 0 )
		{
			byte = UART_Get_Byte();
			
			if ( i < ackbufferlen )
				buffer[ i++ ] = byte;
			
/*			switch ( byte )
			{
				case CMD_SOH:	printf_P( PSTR("SOH "));
								break;
				case CMD_STX:	printf_P( PSTR("STX "));
								break;
				case CMD_ETX:	printf_P( PSTR("ETX "));
								break;
				case CMD_EOT:	printf_P( PSTR("EOT "));
								break;
				default:		printf_P( PSTR("%02x "), byte );
								break;
			}
			STDOUT_Flush(); */
			
		}
	} while ( byte != 0x04 );
	
	return( buffer[ 5 ] );
}

void tafel_clr( void )
{
	unsigned char clrstring[] = { 27,'L',0 };
	tafel_print ( 2 , 1 , 1 , 1 , clrstring );
}

void tafel_test( void )
{
	char c=128,a;
	int i ;
	for ( i = 128 ; i < 256 ; )
	{
		for ( a = 0 ; a < 8 ; a++ )
			{
				printf_P( PSTR("%02x= %c "),c,c );
				c++;
				i++;
			}
		printf_P( PSTR("\n"));
	}
}

void tafel_widget( char x, char y, char lenx, char leny, char * string)
{
	char px,py;
	char widgetstring[ TafelXlen +1 ];
	int  stringlen ;
	
	stringlen = strlen ( string ) ;
	STDOUT_SetXY ( x, y );
	py = y;
	
	if ( stringlen > lenx - 4 )
		string[ lenx - 3 ] = '\0';
	
	stringlen = strlen ( string ) ;

	printf_P( PSTR("%c[%s]"), 0xda, string );
	for ( px = ( x + 1 ) + stringlen  ; px < ( x + lenx - 2 ) ; px++ )
		printf_P( PSTR("%c"), 0xc4 );
	printf_P( PSTR("%c"), 0xbf );
	STDOUT_Flush ();
	
	for ( py = y + 1 ; py < ( y + leny ) ; py++ )
	{
		STDOUT_SetXY ( x , py );
		printf_P( PSTR("%c"), 0xb3 );
		for ( px = 0 ; px < ( lenx - 1 ) ; px++ )
			printf_P( PSTR(" ") );
		printf_P( PSTR("%c"), 0xb3 );
		STDOUT_Flush ();
	}
	
	py = y + leny;
	printf_P( PSTR("%c"), 0xc0 );
	for ( px = x + 1 ; px < ( x + lenx ) ; px++ )
		printf_P( PSTR("%c"), 0xc4 );
	printf_P( PSTR("%c"), 0xd9 );
	STDOUT_Flush ();

}

#endif
//@}

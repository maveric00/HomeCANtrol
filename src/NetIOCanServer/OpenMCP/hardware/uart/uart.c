/*!\file uart.c \brief Stellt die UART-Schnittstelle bereit */
//***************************************************************************
//*            uart.c
//*
//*  Mon Jul 31 21:46:47 2007
//*  Copyright  2007 Dirk Broßwick
//*  Email
///	\ingroup hardware
///	\defgroup UART Die UART-Schnittstelle (uart.c)
///	\code #include "uart.h" \endcode
///	\par Uebersicht
///		Die UART-Schnittstelle fuer den AVR-Controller
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
#include "uart.h"
#include "system/buffer/fifo.h"
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

volatile unsigned char TX_state = TX_complete;

unsigned char TX_Buffer[ TX_Bufferlen ];
unsigned char RX_Buffer[ RX_Bufferlen ];

volatile unsigned long TX_Counter = 0;
volatile unsigned long RX_Counter = 0;

unsigned int RX_fifo;
unsigned int TX_fifo;

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die Uart1 des ATmega2561.
 * \param	NONE
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void UART_init( void )
{	
	// FIFO initialisieren wenn nicht schon passiert
	INIT_FIFO ();
		
	// FIFO reservieren für RX und TX
	RX_fifo = Get_FIFO( RX_Buffer, RX_Bufferlen );
	TX_fifo = Get_FIFO( TX_Buffer, TX_Bufferlen );

	// TX_state auf complete setzen, da ja nix gesendet wurde
	TX_state = TX_complete;
					
	#ifdef __AVR_ATmega2561__
		// Bitrate einstellen
		UBRR1H = (unsigned char) ( UBRR_VAL>>8 );
		UBRR1L = (unsigned char) UBRR_VAL;									
	
		// UART Tx,Rx,RxINT einschalten, UDRIW und RXCIE werden erst wärend der benutzung freigeben nach bedarf
		UCSR1B |= ( 1<<TXEN1 ) | ( 1<<RXEN1 ) | ( 1<<RXCIE1 ) ; 
		// Asynchron 8N1
		UCSR1C |= ( 1<< USBS1 ) | ( 1<<UCSZ11 ) ;    								 
	#endif
	#ifdef __AVR_ATmega644__
		// Bitrate einstellen
		UBRR0H = (unsigned char) ( UBRR_VAL>>8 );
		UBRR0L = (unsigned char) UBRR_VAL;									
	
		// UART Tx,Rx,RxINT einschalten, UDRIW und RXCIE werden erst wärend der benutzung freigeben nach bedarf
		UCSR0B = ( 1<<TXEN0 ) | ( 1<<RXEN0 ) | ( 1<<RXCIE0 ) ; 
		// Asynchron 8N1
		UCSR0C = ( 1<< USBS0 ) | ( 1<<UCSZ01 ) | ( 1<<UCSZ00 ) ;    								 
	#endif
	#ifdef __AVR_ATmega644P__
		// Bitrate einstellen
		UBRR0H = (unsigned char) ( UBRR_VAL>>8 );
		UBRR0L = (unsigned char) UBRR_VAL;									
	
		// UART Tx,Rx,RxINT einschalten, UDRIW und RXCIE werden erst wärend der benutzung freigeben nach bedarf
		UCSR0B = ( 1<<TXEN0 ) | ( 1<<RXEN0 ) | ( 1<<RXCIE0 ) ; 
		// Asynchron 8N1
		UCSR0C = ( 1<< USBS0 ) | ( 1<<UCSZ01 ) | ( 1<<UCSZ00 ) ;    								 
	#endif
	
	return;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Interruptroutine zum senden, wenn Transmiter ist fertig und keine weiteren Daten im Puffer
 */
/* -----------------------------------------------------------------------------------------------------------*/
#ifdef __AVR_ATmega2561__
	ISR( USART1_TX_vect )
#endif
#ifdef __AVR_ATmega644__
	ISR( USART0_TX_vect )
#endif
#ifdef __AVR_ATmega644P__
	ISR( USART0_TX_vect )
#endif
{
	// Wenn der TX_puffer leer ist, Interrupt sperren
	#ifdef __AVR_ATmega2561__
		UCSR1B &= ~( 1<<TXCIE1 );
		TX_state = TX_complete;
	#endif
	#ifdef __AVR_ATmega644__
		UCSR0B &= ~( 1<<TXCIE0 );
		TX_state = TX_complete;
	#endif
	#ifdef __AVR_ATmega644P__
		UCSR0B &= ~( 1<<TXCIE0 );
		TX_state = TX_complete;
	#endif
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Interruptroutine zum senden, wird aufgerufen wenn UART-Register leer, aber Transmitter noch am senden
 */
/* -----------------------------------------------------------------------------------------------------------*/
#ifdef __AVR_ATmega2561__
	ISR( USART1_UDRE_vect )
#endif
#ifdef __AVR_ATmega644__
	ISR( USART0_UDRE_vect )
#endif
#ifdef __AVR_ATmega644P__
	ISR( USART0_UDRE_vect )
#endif
{

	// checken ob noch bytes im Buffer sind
	if ( Get_Bytes_in_FIFO ( TX_fifo ) != 0 )
	{
		#ifdef __AVR_ATmega2561__
			UDR1 = Get_Byte_from_FIFO ( TX_fifo );
		#endif
		#ifdef __AVR_ATmega644__
			UDR0 = Get_Byte_from_FIFO ( TX_fifo );
		#endif
		#ifdef __AVR_ATmega644P__
			UDR0 = Get_Byte_from_FIFO ( TX_fifo );
		#endif
	}
	else
	{
		// Wenn nicht was es das letzte Byte im Puffer, dann diesen Interrupt sperren und TX-Interrupt freigeben
		#ifdef __AVR_ATmega2561__
			UCSR1B |= ( 1<<TXCIE1 );
			UCSR1B &= ~( 1<<UDRIE1 );
		#endif
		#ifdef __AVR_ATmega644__
			UCSR0B |= ( 1<<TXCIE0 );
			UCSR0B &= ~( 1<<UDRIE0 );
		#endif
		#ifdef __AVR_ATmega644P__
			UCSR0B |= ( 1<<TXCIE0 );
			UCSR0B &= ~( 1<<UDRIE0 );
		#endif
	}
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Interruptroutine zum empfangen
 */
/* -----------------------------------------------------------------------------------------------------------*/
#ifdef __AVR_ATmega2561__
	ISR( USART1_RX_vect )
#endif
#ifdef __AVR_ATmega644__
	ISR( USART0_RX_vect )
#endif
#ifdef __AVR_ATmega644P__
	ISR( USART0_RX_vect )
#endif
{
	unsigned char Byte;
	
	RX_Counter++;
		
	#ifdef __AVR_ATmega2561__
		Byte = UDR1;
	#endif
	#ifdef __AVR_ATmega644__
		Byte = UDR0;
	#endif
	#ifdef __AVR_ATmega644P__
		Byte = UDR0;
	#endif
	
	if ( Get_Bytes_in_FIFO ( RX_fifo ) < RX_Bufferlen )
		Put_Byte_in_FIFO ( RX_fifo, Byte );
}


/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet ein Byte über die Uart1.
 * \param	ein Byte vom typ unsigned char
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void UART_Send_Byte( unsigned char Byte )
{

	cli();
	
	TX_Counter++;
	
	if ( Get_Bytes_in_FIFO ( TX_fifo ) == 0 )
	{
		// Wenn Controller noch mit senden eines Byte beschäftig, ab in den Puffer
		if ( TX_state == TX_sending )
		{			
			// Byte in Buffer schreiben
			Put_Byte_in_FIFO ( TX_fifo, Byte );
			
			// Buffer Emty freigeben, TX complete sperren
			#ifdef __AVR_ATmega2561__
				UCSR1B &= ~( 1<<TXCIE1 );
				UCSR1B |= ( 1<<UDRIE1 );
			#endif
			#ifdef __AVR_ATmega644__
				UCSR0B &= ~( 1<<TXCIE0 );
				UCSR0B |= ( 1<<UDRIE0 );
			#endif
			#ifdef __AVR_ATmega644P__
				UCSR0B &= ~( 1<<TXCIE0 );
				UCSR0B |= ( 1<<UDRIE0 );
			#endif
		}
		else
		{
			#ifdef __AVR_ATmega2561__
				UDR1 = Byte;
			
				// Buffer Emty sperren, TX_complete freigeben
				UCSR1B |= ( 1<<TXCIE1 );
				UCSR1B &= ~( 1<<UDRIE1 );
			#endif
			#ifdef __AVR_ATmega644__
				UDR0 = Byte;
			
				// Buffer Emty sperren, TX_complete freigeben
				UCSR0B |= ( 1<<TXCIE0 );
				UCSR0B &= ~( 1<<UDRIE0 );
			#endif
			#ifdef __AVR_ATmega644P__
				UDR0 = Byte;
			
				// Buffer Emty sperren, TX_complete freigeben
				UCSR0B |= ( 1<<TXCIE0 );
				UCSR0B &= ~( 1<<UDRIE0 );
			#endif
			
			TX_state = TX_sending;
		}
	}
	else
	{	
		// Wenn Puffer voll, warten bis wieder was rein paßt
		while ( 1 )
		{
			cli();
			
			if ( Get_Bytes_in_FIFO ( TX_fifo ) < TX_Bufferlen ) break;
			
			sei();
		}

		// Byte in Buffer schreiben
		Put_Byte_in_FIFO ( TX_fifo, Byte );
	}
	
	sei();
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Empfängt ein Byte über die Uart1.
 * \param	NONE
 * \return  Byte aus den Puffer
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned char UART_Get_Byte( void )
{
	unsigned char Byte=0;
	
	cli();
	
	// check ob überhaupt noch Bytes im Puffer sind, und wenn ja, auslesen
	if ( Get_Bytes_in_FIFO ( RX_fifo ) != 0 )
		Byte = Get_Byte_from_FIFO ( RX_fifo );
	
	sei();
	
	return(Byte);
}


/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Liest die Anzahl der Byte die im Empfangsbuffer sind.
 * \param	NONE
 * \return  Anzahl der Byte im Enpfangpuffer
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned int UART_Get_Bytes_in_Buffer( void )
{
	unsigned int BytesInBuffer=1;
	
	cli();
	// Anzahl der Bytes holen
	BytesInBuffer = Get_Bytes_in_FIFO ( RX_fifo );
	
	sei();
	
	return( BytesInBuffer );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Liest die Anzahl der Byte die im Empfangsbuffer sind.
 * \param	NONE
 * \return  Anzahl der Byte im Enpfangpuffer
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned int UART_Get_Bytes_in_RxBuffer( void )
{
	unsigned int BytesInBuffer=1;
	
	cli();
	// Anzahl der Bytes holen
	BytesInBuffer = Get_Bytes_in_FIFO ( RX_fifo );
	
	sei();
	
	return( BytesInBuffer );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Liest die Anzahl der Byte die im Empfangsbuffer sind.
 * \param	NONE
 * \return  Anzahl der Byte im Enpfangpuffer
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned int UART_Get_Bytes_in_TxBuffer( void )
{
	unsigned int BytesInBuffer=1;
	
	cli();
	// Anzahl der Bytes holen
	BytesInBuffer = Get_Bytes_in_FIFO ( TX_fifo );
	
	sei();
	
	return( BytesInBuffer );
}

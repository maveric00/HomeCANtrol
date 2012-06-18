/*!\file ext_int.c \brief Stellt die externen Interrupts bereit*/
//***************************************************************************
//*            ext_int.c
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  User
//*  Email
///	\ingroup hardware
///	\defgroup ext_int externe Interrupts mit Callback (ext_int.c)
///	\code #include "ext_int.h" \endcode
///	\par Uebersicht
///		Behandelt die externen Interrupts und verwaltet die Callbacks.
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
 
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "config.h"

#if defined(EXTINT)

#include "hardware/ext_int/ext_int.h"

EXT_INT_CALLBACK_FUNC extint_CallBack_Table[ MAX_EXT_INT ];

void EXTINT_init( void )
{
	EICRA = 0;
	#ifdef __AVR_ATmega2561__
		EICRB = 0;
	#endif
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Initialisiert die externen Interrupts und legt den Mode fest.
 * \param 	interrupt_number		Nummer des externen Interrupts.
 * \param 	interrupt_sensemode		Der Mode auf welchen der Interrupt reagiert. Mögliche Wert: SENSE_LOW, SENSE_RISING, SENSE_FALLING, SENSE_CHANGE
 * \param 	pFunc					Zeiger auf die Funktion, die bei einem Interrupt aufgerufen werden soll.
 * \retval  						Gibt ERROR_SENSEMODE, ERROR_INTERRUPTNUMBER oder OK zurück.
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned char EXTINT_set ( unsigned char interrupt_number, unsigned char interrupt_sensemode, EXT_INT_CALLBACK_FUNC pFunc )
{
	// ist der der interrupt giltig ?
	if ( interrupt_number < MAX_EXT_INT )
	{
		// sensemode gültig ?
		if ( interrupt_sensemode < 4 )
		{
			// SENSEMODE auf die Register aufteilen
			#ifdef __AVR_ATmega2561__
				if ( interrupt_number < 4 )
			#endif
			#ifdef __AVR_ATmega644__
				if ( interrupt_number < 3 )
			#endif
			#ifdef __AVR_ATmega644P__
				if ( interrupt_number < 3 )
			#endif
			{
				EICRA &= ~( 3 << ( interrupt_number << 1 ) );
				EICRA |= ( interrupt_sensemode << ( interrupt_number << 1 ) );
			}
			#ifdef __AVR_ATmega2561__
			else
			{
				EICRB &= ~( 3 << ( interrupt_number << 1 ) );
				EICRB |= ( interrupt_sensemode << ( interrupt_number << 1 ) );
			}
			#endif
		}
		else
		{
			return( ERROR_SENSEMODE );
		}
		
		// CallbackFunc eintragen
		extint_CallBack_Table[ interrupt_number ] = pFunc ;
		
		// Interrupt freigeben
		EXTINT_free( interrupt_number );
	
		return( OK );		
	}
	else
	{
		return( ERROR_INTERRUPTNUMBER );
	}
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Gibt einen externen Interrupt wieder frei.
 * \param 	interrupt_number		Nummer des externen Interrupts.
 * \retval	NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void EXTINT_free ( unsigned char interrupt_number )
{
	if ( interrupt_number < MAX_EXT_INT )
		EIMSK |= ( 1 << interrupt_number );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Sperrt einen externen Interrupt.
 * \param 	interrupt_number		Nummer des externen Interrupts.
 * \retval	NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void EXTINT_block ( unsigned char interrupt_number )
{
	if ( interrupt_number < MAX_EXT_INT )
		EIMSK &= ~( 1 << interrupt_number );	
}

ISR(INT0_vect)
{
	extint_CallBack_Table[0]();
}

ISR(INT1_vect)
{
	extint_CallBack_Table[1]();
}

ISR(INT2_vect)
{
	extint_CallBack_Table[2]();
}

#ifdef __AVR_ATmega2561__
ISR(INT3_vect)
{
	extint_CallBack_Table[3]();
}

ISR(INT4_vect)
{
	extint_CallBack_Table[4]();
} 

ISR(INT5_vect)
{
	extint_CallBack_Table[5]();
}

ISR(INT6_vect)
{
	extint_CallBack_Table[6]();
}

ISR(INT7_vect)
{
	extint_CallBack_Table[7]();
}
#endif
#endif

//@}

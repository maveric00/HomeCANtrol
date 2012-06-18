/*! \file adc.c \brief Stellt die ADC Funkionalitaet bereit */
/***************************************************************************
 *            adc.c
 *
 *  Sun Mar  8 18:44:14 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup hardware
///	\defgroup ADC Funktionen für den ADC (adc.c)
///	\code #include "adc.h" \endcode
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
#include "config.h"

#if defined(ANALOG)

#include "adc.h"

/*------------------------------------------------------------------------------------------------------------*/
/*!\group ADC
 * \brief Hier wird der ADC Initialisiert.
 * \param		NONE
 * \return		NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void ADC_init( void )
{
#ifdef __AVR_ATmega2561__
	#if defined(OpenMCP)
		// ADC einschalten
		ADCSRA = ( 1<<ADEN ) | ( 1<<ADPS2 ) ;
		// AREF extern an AREF
		ADMUX = ( 1<<REFS0 );

		// starte erste conversation
		ADCSRA |= ( 1<<ADSC );
		
		while( bit_is_set( ADCSRA , ADSC ) );
	#endif
#endif
#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
		// ADC einschalten
		ADCSRA = ( 1<<ADEN ) | ( 1<<ADPS2 ) ;
		// AREF extern an AREF
		ADMUX = ( 1<<REFS0 );

		// starte erste conversation
		ADCSRA |= ( 1<<ADSC );
		
		while( bit_is_set( ADCSRA , ADSC ) );
	#endif
#endif
#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
		// ADC einschalten
		ADCSRA = ( 1<<ADEN ) | ( 1<<ADPS2 ) ;
		// AREF extern an AREF
		ADMUX = ( 1<<REFS0 );
	
		// starte erste conversation
		ADCSRA |= ( 1<<ADSC );
		
		while( bit_is_set( ADCSRA , ADSC ) );
	#endif
#endif
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\group ADC
 * \brief Holt von eine ADC-Eingang den digitalisierten Wert
 * \param		Channel	 Der ADC-Eingang von den digitalisiert werden soll
 * \return		Der digitalisiert Wert.
 * \retval		Der digitalisiert Wert.
 */
/*------------------------------------------------------------------------------------------------------------*/
int ADC_GetValue( char Channel )
{
#ifdef __AVR_ATmega2561__
	#if defined(OpenMCP)
	Channel = Channel & 0x7;
	#endif
#endif

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
	Channel = Channel & 0x7;
	#endif
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
//	if ( Channel < MAX_ADC )
//		Channel = (Channel & 0x1) + 3;
//	else
//		return(0);
	Channel = 3;
	#endif
#endif

	ADMUX &= ~( 0x7 );
	
	ADMUX |= Channel;
	
	// starte erste conversation
	ADCSRA |= ( 1<<ADSC );

	while( bit_is_set( ADCSRA , ADSC ) );
	
	return( ADC );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\group ADC
 * \brief Rechnet einen Wert in Millivolt um.
 * \param		ADC_Wert		Der Wert der umgerechnet werden soll.
 * \param		ADC_mVoltmax	Der Maximalwert in den umgerechnet werden soll
 * \return		Wert in Millivolt.
 * \retval		Wert in Millivolt.
 */
/*------------------------------------------------------------------------------------------------------------*/
int ADC_mVolt( int ADC_Wert, int ADC_mVoltmax )
{
	long temp;
	
	temp = ( ( ( long ) ADC_Wert ) * ADC_mVoltmax ) / 1000 ;
	temp = temp - ( temp / 64 ) - ( temp / 128 );

	return( (int) temp );
}

#endif

//@}

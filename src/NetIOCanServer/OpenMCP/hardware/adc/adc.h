/*! \file adc.h \brief Stellt die ADC Funkionalitaet bereit */
/***************************************************************************
 *            adc.h
 *
 *  Sun Mar  8 18:45:01 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup hardware
///	\defgroup ADC Funktionen für den ADC (adc.h)
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
#ifndef _ADC_H
	#define ADC_H
	
	void ADC_init( void );
	int ADC_GetValue( char channel );
	int ADC_mVolt( int ADC_Wert, int ADC_mVoltmax );

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
		#define MAX_ADC 	8
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

#ifdef __AVR_ATmega2561__
	#if defined(OpenMCP)
		#define MAX_ADC 	8
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

#if defined(__AVR_ATmega644P__)
	#if defined(myAVR)
		#define MAX_ADC 	1
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

#endif /* ADC_H */
//@}

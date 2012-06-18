/*! \file led_core.h \brief Stellt die LED Funkionalitaet bereit */
//***************************************************************************
//*            led_core.h
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  User
//*  Email
//****************************************************************************/
///	\ingroup hardware
///	\defgroup LED Stellt die LED Funkionalitaet bereit (led_core.h)
///	\code #include "led_core.h" \endcode
///	\par Uebersicht
///		Stellt funktionen bereit um LED über ein definiertes interface anzusprechen.
//****************************************************************************/
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
//@{

#ifndef _LED_CORE_H
	#define LED_CORE_H

	#include <stddef.h>

	typedef void ( * LED_CALLBACK_FUNC ) ( void );

	void LED_init( void );
	char LED_Register( LED_CALLBACK_FUNC Led_on, LED_CALLBACK_FUNC Led_off, LED_CALLBACK_FUNC Led_toggle );
	void LED_on ( unsigned char LED_index );
	void LED_off ( unsigned char LED_index );
	void LED_toggle ( unsigned char LED_index );

	struct LED_STRUCT {
		volatile LED_CALLBACK_FUNC 	LED_ON;
		volatile LED_CALLBACK_FUNC 	LED_OFF;
		volatile LED_CALLBACK_FUNC 	LED_TOGGLE;
		char		 				LED_STATE;
	};

#if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega644P__)
	#ifdef __AVR_ATmega2561__
		#define MAX_LED		3
	#endif
	#ifdef __AVR_ATmega644P__
		#define MAX_LED		2
	#endif
	
		#define	ON			0
		#define OFF			1
		#define	UNABLE		~ON
#else
	#error "LED wird auf dieser Hardwareplatform nicht unterstützt. Bitte in der config.h abwählen!"
#endif

#endif /* LED_CORE_H */

 

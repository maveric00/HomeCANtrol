/*! \file led_core.c \brief Stellt die LED Funkionalitaet bereit */
//***************************************************************************
//*            led_core.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  User
//*  Email
//****************************************************************************/
///	\ingroup hardware
///	\defgroup LED Stellt die LED Funkionalitaet bereit (led_core.c)
///	\code #include "led_core.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen bereit um LED über ein definiertes Interface anzusprechen.
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

#include <stdio.h>
#include <avr/pgmspace.h>

#include "config.h"

#if defined(LED)

#include "led_core.h"
#include "led_0.h"
#include "led_1.h"
#include "led_2.h"

struct LED_STRUCT Led_Table [ MAX_LED ];

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Hier wird der LED_Core initialisiert.
 * \param	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void LED_init( void )
{
	unsigned int	i;
	
	for ( i = 0 ; i < MAX_LED ; i++ )
	{
		Led_Table[ i ].LED_ON = NULL;
		Led_Table[ i ].LED_OFF = NULL;
		Led_Table[ i ].LED_TOGGLE = NULL;
		Led_Table[ i ].LED_STATE =UNABLE;
	}

#if defined(__AVR_ATmega2561__) && defined(OpenMCP)
	LED_0_init ();
	LED_1_init ();
	LED_2_init ();
#endif
#if defined(__AVR_ATmega644P__) && defined(myAVR)
	LED_0_init ();
	LED_1_init ();
#endif
	
	return;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Mit dieser Funktion kann eine LED registriert werden.
 * \param	Led_on		Pointer auf die Funktion die eine LED einschaltet.
 * \param	Led_off		Pointer auf die Funktion die eine LED ausschaltet.
 * \param	Led_toggle	Pointer auf die Funktion die eine LED umschaltet.
 * \return	LED			Liefert die nummer der LED zurück unter sie jetzt registriert ist oder -1 wenn Fehler.
 */
/*------------------------------------------------------------------------------------------------------------*/
char LED_Register( LED_CALLBACK_FUNC Led_on, LED_CALLBACK_FUNC Led_off, LED_CALLBACK_FUNC Led_toggle )
{
	unsigned int 	i;
	
	for ( i = 0 ; i < MAX_LED ; i++ )
	{
		if ( ( Led_Table[ i ].LED_ON == NULL ) && ( Led_Table[ i ].LED_OFF == NULL ) && ( Led_Table[ i ].LED_ON == NULL ) )
		{
			Led_Table[ i ].LED_ON = Led_on;
			Led_Table[ i ].LED_OFF = Led_off;
			Led_Table[ i ].LED_TOGGLE = Led_toggle;
			return( i );
		}
	}
	return( -1 );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schalten eine LED ein.
 * \param	LED_index	Die Nummer und unter der die LED registriert ist.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void LED_on ( unsigned char LED_index )
{
	if ( ( LED_index >= MAX_LED ) || ( Led_Table[ LED_index ].LED_ON == NULL ) )
		return;
	
	Led_Table[ LED_index ].LED_ON();
	
	return;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schaltet eine LED aus.
 * \param	LED_index	Die Nummer und unter der die LED registriert ist.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void LED_off ( unsigned char LED_index )
{
	if ( ( LED_index >= MAX_LED ) || ( Led_Table[ LED_index ].LED_OFF == NULL ) )
		return;
	
	Led_Table[ LED_index ].LED_OFF();
	
	return;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Toggelt eine LED.
 * \param	LED_index	Die Nummer und unter der die LED registriert ist.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void LED_toggle ( unsigned char LED_index )
{
	if ( ( LED_index >= MAX_LED ) || ( Led_Table[ LED_index ].LED_TOGGLE == NULL ) )
		return;
	
	Led_Table[ LED_index ].LED_TOGGLE();
	
	return;
}

#endif

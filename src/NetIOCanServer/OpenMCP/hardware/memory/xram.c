/*! \file xram.c \brief Aktiviert das externe RAM-Interface */
//***************************************************************************
//*            xram.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  User
//*  Email
//****************************************************************************/
///	\ingroup hardware
///	\defgroup xram Aktiviert das externe RAM-Interface (xram.c)
///	\code #include "xram.h" \endcode
///	\par Uebersicht
/// Aktiviert das externe RAM-Interface. Wenn die xram.h eingebunden wird, wird
/// automatisch die Aktivierung in .init eingetragen und steht somit sofort zur
/// Verfügung.
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

#include <avr/io.h>

#if defined(__AVR_ATmega2561__)

#include "xram.h"

#if defined(LED)
	#include "hardware/led/led_0.h"
	#include "hardware/led/led_1.h"
	#include "hardware/led/led_2.h"
#endif

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Aktiviert das externe RAM-Interface und testet den externen RAM
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void init_xram (void)
{
	// externes RAM-Interface freigeben
	XMCRA |= ( 1<<SRE ) ;
	// A16 freigeben, hängt an PD7, damit der RAM funktioniert, wenn dies nicht gemacht wird, ist A16 tristate und der RAM macht komische sachen :-)
	DDRD |= ( 1<<PD7 );
	PORTD |= ( 1<<PD7 );

#if defined(LED)
	LED_0_init();
	LED_1_init();
	LED_2_init();
	
	LED_0_ON();
#endif
	
	unsigned char * p = (unsigned char *) 0x2200;
	
	unsigned char data = 0, buffer;
	unsigned long fehler = 0,adress = 1;
	unsigned int i,a;
	int adressbit = 0 ;

	for( i = 0x2200 ; i<0xffff ; i++ )
	{
		p++;

		data=1;
		
		for( a = 0 ; a < 8 ; a++ )
		{
			data = 1<<a;
			*p = data;
			if ( *p != data )
			{
				fehler++;
				break;
			}
		}		
		*p = 0x00;
	}

	if ( fehler != 0 ) 
		while(1);
#if defined(LED)
	else
		LED_1_ON();
#endif
	
	p = adress;
	
	for( adressbit = 0 ; adressbit < 16 ; adressbit++ )
	{
		p = adress * 2 ;
		adress = p ;
		*p = 0x00;
		
		fehler=0;
		
		for( i = 0x8000 ; i<0xffff ; i++ )
		{
			p = i ;
			if ( *p != 0x00 )
			{
				fehler++;
			}
		}
		if ( fehler != 0 ) while(1);
				
		p = ( 1 << adressbit ) ;
		*p = 0x00;
	}
#if defined(LED)
	LED_2_ON();
#endif
	
	return;
}
#endif

//@}

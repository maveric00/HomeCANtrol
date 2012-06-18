/*!\file 1wire.h \brief Stellt Treiber für 1-Wire zu Verfügung */
/***************************************************************************
 *            1wire.h
 *
 *  Mon Apr 27 23:28:56 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 *
 * Der Code ist in Anlehnung an Peter Dannegger seinem Code geschrieben.
 * Veröffentlicht unter http://www.mikrocontroller.net/topic/14792 .
 ****************************************************************************/
///	\ingroup hardware
///	\defgroup 1wire Stellt Treiber für 1-Wire zu Verfügung (1wire.h)
///	\par Uebersicht
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

#ifndef _1WIRE_H
	#define _1WIRE_H

	#include <avr/pgmspace.h>

/*
	#define DQ_PIN	PA5
	#define DQ_IN	PINA
	#define DQ_OUT	PORTA
	#define DQ_DDR	DDRA
*/

#ifdef __AVR_ATmega2561__
	#error "1-Wire wird für diese Hardwareplatform nicht unterstützt, bitte in der config.h abwählen!"
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
		#define DQ_PIN	PB7
		#define DQ_IN	PINB
		#define DQ_OUT	PORTB
		#define DQ_DDR	DDRB
	#else
		#error "1-Wire wird für diese Hardwareplatform nicht unterstützt, bitte in der config.h abwählen!"
	#endif
#endif

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
		#define DQ_PIN	PB0
		#define DQ_IN	PINB
		#define DQ_OUT	PORTB
		#define DQ_DDR	DDRB
	#else
		#error "1-Wire wird für diese Hardwareplatform nicht unterstützt, bitte in der config.h abwählen!"
	#endif
#endif

	#define MATCH_ROM		0x55
	#define SKIP_ROM		0xCC
	#define	SEARCH_ROM		0xF0
	#define CONVERT_T		0x44		// DS1820 commands
	#define READ			0xBE
	#define WRITE			0x4E
	#define EE_WRITE		0x48
	#define EE_RECALL		0xB8

	#define	SEARCH_FIRST	0xFF		// start new search
	#define	PRESENCE_ERR	0xFF
	#define	DATA_ERR		0xFE
	#define LAST_DEVICE		0x00		// last device found


	char ONEWIRE_reset( void );
	char ONEWIRE_bitio( char BIT );
	unsigned int ONEWIRE_writebyte( unsigned char BYTE );
	unsigned int ONEWIRE_readbyte( void );
	void ONEWIRE_ParasitepowerOn( void );
	char ONEWIRE_searchrom( char diff, char * id );
	void ONEWIRE_command( char command, char * id );

	const char * ONEWIRE_getfamilycode2string( char id );

	typedef struct {
		const prog_char	  id;
		const prog_char * familystring;
	} const FAMILY ;

#endif /* 1WIRE_H */

//@}

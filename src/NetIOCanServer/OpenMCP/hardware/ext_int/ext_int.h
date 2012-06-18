/*!\file ext_int.h \brief Stellt die externen Interrupts bereit*/
//***************************************************************************
//*            ext_int.h
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  User
//*  Email
///	\ingroup hardware
///	\defgroup ext_int externe Interrupts mit Callback (ext_int.h)
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
 
#ifndef _EXT_INT_H
	#define EXT_INT_H

	typedef void ( * EXT_INT_CALLBACK_FUNC ) ( void );

	void EXTINT_init( void );
	unsigned char EXTINT_set ( unsigned char interrupt_number, unsigned char interrupt_sensemode, EXT_INT_CALLBACK_FUNC pFunc );
	void EXTINT_free ( unsigned char interrupt_number );
	void EXTINT_block ( unsigned char interrupt_number );

	#ifdef __AVR_ATmega2561__
		#define 	MAX_EXT_INT				8
	#else
	#ifdef __AVR_ATmega644__
		#define 	MAX_EXT_INT				3
	#endif
	#ifdef __AVR_ATmega644P__
		#define 	MAX_EXT_INT				3
	#endif
	#endif


	#define		ERROR_SENSEMODE			0xff
	#define		ERROR_INTERRUPTNUMBER	0xfe
	#define		OK						0x00

	#define     SENSE_LOW				0x00
	#define		SENSE_CHANGE			0x01
	#define		SENSE_FALLING			0x02
	#define		SENSE_RISING			0x03

#endif /* EXT_INT_H */

 

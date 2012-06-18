/***************************************************************************
 *            pcint.h
 *
 *  Wed Sep 30 11:29:22 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/

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

#ifndef __PCINT_H__
	#define __PCINT_H__

#if defined(__AVR_ATmega644P__)||defined(__AVR_ATmega644__)
	#define MAX_PCINT	4

	typedef void ( * PCINT_CALLBACK_FUNC ) ( void );

	void PCINT_init ( void );
	int PCINT_set ( unsigned char PCINT, PCINT_CALLBACK_FUNC pFunc );
	int PCINT_enablePCINT( unsigned char PCINT );
	int PCINT_disablePCINT( unsigned char PCINT );
	int PCINT_enablePIN( unsigned char PIN, unsigned char PCINT );
	int PCINT_disablePIN( unsigned char PIN, unsigned char PCINT );

	struct PCINT_STRUCT {
		volatile PCINT_CALLBACK_FUNC 	FUNCTION;
	};

#else
	#error "PCINT wird für diese Hardwareplatform nicht unterstützt, bitte in der config.h abwählen!"
#endif

#endif


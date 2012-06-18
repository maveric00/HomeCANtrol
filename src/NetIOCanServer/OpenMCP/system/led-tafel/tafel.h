/*! \file led-tafel.h \brief LED-Tafel Ansteuerung */
//***************************************************************************
//*            led-tafel.h
//*
//*  Mon Aug 29 19:19:16 2008
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup ledtafel LED-Tafel Ansteuerung (led-tafel.h)
///	\code #include "led-tafel.h" \endcode
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

#ifndef _TAGEL_H
	#define TAFEL_H

	#define CMD_SOH 0x01
	#define CMD_STX 0x02
	#define CMD_ETX 0x03
	#define CMD_EOT 0x04

	#define ackbufferlen 16

	#define TafelXlen	56
	#define TafelYlen	20

	char tafel_init( void );
	char tafel_print (int tafel, int seite, int zeile, int spalte, unsigned char *string);
	char tafel_wait4ack( int timeout );
	void tafel_clr( void );
	void tafel_test( void );
	void tafel_widget( char x, char y, char lenx, char leny, char * string );

#endif

//@}

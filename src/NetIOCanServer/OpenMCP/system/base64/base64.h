/*! \file base64.h \brief Stellt Funktionen für Base64 de/encoding bereit. */
//***************************************************************************
//*            base64.h
//*
//*  Son Aug  3 18:41:42 2009
//*  Copyright  2009  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup BASE64 Stellt Funktionen für Base64 de/encoding bereit. (base64.h)
///	\par Uebersicht
///		Stellt Funktionen für Base64 de/encoding bereit.
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

#ifndef _BASE64_H
	#define _BASE64_H

	int base64_decode( unsigned char *out, const unsigned char *in, int out_size);
	char * base64_encode(char *out, int out_size, const unsigned char *in, int in_size);

#endif

//@}

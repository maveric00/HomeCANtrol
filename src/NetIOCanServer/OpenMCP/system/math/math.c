/*! \file math.c \brief Einige Extra Sachen */
//***************************************************************************
//*            math.c
//*
//*  Sun Jun 11 20:30:57 2006
//*  Copyright  2006  User
//*  Email
//****************************************************************************/
///	\ingroup math
///	\defgroup MATH Einige Mathfunktionen die AVR-GCC nicht hat (math.c)
///	\code #include "math.h" \endcode
///	\par Uebersicht
///		Einige Funktionen die AVR-GCC nicht hat wie das tauschen
/// der Bytes in einen int oder long. Big-Endian <-> Little-Endian.
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
unsigned long ChangeEndian32bit( unsigned long Wert)
	{
		Wert = ( ( Wert & 0xff000000 ) >> 24 ) | ( ( Wert & 0x000000ff ) << 24 ) | ( ( Wert & 0x00ff0000 ) >> 8 ) | ( ( Wert & 0x0000ff00 ) << 8 );
		return( Wert );
	}
	
unsigned int ChangeEndian16bit( unsigned int Wert)
	{
		Wert = ( ( Wert & 0xff00 ) >> 8 ) | ( ( Wert & 0x00ff ) << 8 );
		return( Wert );
	}
//@}

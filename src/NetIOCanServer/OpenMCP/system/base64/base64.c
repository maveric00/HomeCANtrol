/*! \file base64.c \brief Stellt Funktionen für Base64 de/encoding bereit. */
//***************************************************************************
//*            base64.c
//*
//*  Son Aug  3 18:41:42 2009
//*  Copyright  2009  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup BASE64 Stellt Funktionen für Base64 de/encoding bereit. (base64.c)
///	\code #include "base64.h" \endcode
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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>

#include "base64.h"

const char map[] PROGMEM =
{
    0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
};

const char b64[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" ;

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Decodiert einen Speicherbereich von Base64 nach Normal.
 * \param	*out			Zeiger auf den Speicherbereich in dem der decodete Bereich gespeichert werden soll.
 * \param	out_size		Größe des Speicherbereiches für den decodierten Speicher.
 * \param   *in				Zeiger auf den Speicherbereich der decodiett werden soll.
 * \return  int				Größe des decodierten Speicherbreiches in Bytes. Wenn -1, Fehler.
 */
/*------------------------------------------------------------------------------------------------------------*/
int base64_decode( unsigned char *out, const unsigned char *in, int out_size)
{
    int i, v;
    unsigned char *dst = out;
	unsigned int index;

    v = 0;
    for (i = 0; in[i] && in[i] != '='; i++)
	{
		index = in[i]-43;

		if ( pgm_read_byte( &map[index] ) == 0xff)
    		return -1;
        
		v = (v << 6) + pgm_read_byte( &map[index] );
        
		if (i & 3)
		{
            if (dst - out < out_size)
			{
                *dst++ = v >> (6 - 2 * (i & 3));
            }
        }
	}
 
    return( dst - out );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Encodet einen Speicherbereich nach Base64.
 * \param	*out			Zeiger auf den Speicherbereich in dem der encodete bereich gespeichert werden soll.
 * \param	out_size		Größe des Speicherbereiches für den encodeten Speicher.
 * \param   *in				Zeiger auf den Speicherbereich der encodet werden soll.
 * \param   in_size			Größe des Speicherbereiches der encodet werden soll.
 * \return	* char			Zeiger auf den encodeten Speicher.
 */
/*------------------------------------------------------------------------------------------------------------*/
char * base64_encode(char *out, int out_size, const unsigned char *in, int in_size)
{
	char *ret, *dst;
    unsigned long i_bits = 0;
    int i_shift = 0;
    int bytes_remaining = in_size;
 
    if ( in_size >= 65535 / 4 || out_size < ( in_size+2 ) / 3 * 4 + 1 )
        return NULL;

    ret = dst = out;
    while (bytes_remaining)
	{
        i_bits = (i_bits << 8) + *in++;
        bytes_remaining--;
        i_shift += 8;

        do
		{
            *dst++ = pgm_read_byte( &b64[ (i_bits << 6 >> i_shift) & 0x3f] ) ;
            i_shift -= 6;
        }
		while (i_shift > 6 || (bytes_remaining == 0 && i_shift > 0));
	}

	while ((dst - ret) & 3)
        *dst++ = '=';

	*dst = '\0';

    return ret;
}

//}@


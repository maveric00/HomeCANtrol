/***************************************************************************
 *            string.c
 *
 *  Sat Dec 26 22:02:07 2009
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

#include "string.h"

/*!\brief Wandelt einen String in eine MAC-Adresse
 * \param strMAC	Pointer auf den String der gewandelt werden soll.
 * \param MAC		Pointer auf den Puffer wo die MAC-Adresse gespeichert werden soll.
 * \return			Rüchgabewert der Funktion.
 * \retval 0		Wandlung war erfolgreich. 
 * \retval -1		Wandlung war nicht erfolgreich. 
 */
char strtobin( char * str, unsigned char * bin, int lenght )
{
	int i,c;
  
	i=0;
	
	while (*str)
	{
		if ( i >= (lenght * 2) ) return( -1 );
  
        if ( (i%2) == 0 )
		{
    		bin[ i/2 ] = atoh( *str ) << 4;
        }
		else
		{
    		bin[ i/2 ] += atoh( *str );
        }
   		i++;
  
        do
		{ 
			// skip junk
    		str++;
        } while ( *str == ':' || *str == '-' ) ;
	}
  
	if ( i!=(lenght*2) ) return( -1 );
	return( 0 );
}

/*!\brief Wandelt ein Hex-nibble in einen Wert.
 * \param Digit		Zeichen welches gewandelt werden soll.
 * \return			Rüchgabewert der Funktion.
 * \retval			Wandlung war erfolgreich. 
 */
char atoh( char Digit )
{
	char zeichen;
	
    if ( Digit >= '0' && Digit <= '9')
		zeichen = ( Digit - '0' );
    else if ( Digit >= 'a' && Digit <= 'f')
        zeichen = ( Digit - 'a') + 10;
    else if (Digit >= 'A' && Digit <= 'F')
        zeichen = ( Digit - 'A' ) + 10;
    else
        zeichen = -1;
	
	return( zeichen );
	
}

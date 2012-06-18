/***************************************************************************
 *            n_buffer.c
 *
 *  Tue Dec 29 18:07:55 2009
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

#include "n_buffer.h"

struct N_BUFFER n_buffer;

void N_BUFFER_clean( void )
{
	int i;
	
	n_buffer.Buffersize = 0;
	n_buffer.Layers = 0;
	n_buffer.N_BUFFER_next_Pointer = NULL;
	
	for( i = 0 ; i < MAX_BUFFER_LAYER ; i++ )
	{
		n_buffer.N_Buffer[i] = NULL;	
		n_buffer.N_Buffersize[i] = 0;	
	}
}

int	N_BUFFER_addLayer( char * N_BUFFER, int N_BUFFER_size, char * N_BUFFER_next_Pointer );
{
	int i;
	int returncode = -1;
	
	for( i = 0 ; i < MAX_BUFFER_LAYER ; i++ )
	{
		if ( n_buffer.N_Buffer[i] == NULL )
		{
			n_buffer.N_BUFFER_next_Pointer = N_BUFFER_next_Pointer;
			n_buffer.Layers++;
			n_buffer.Buffersize =+ N_BUUFER_size;
			n_buffer.N_Buffer[ i ] = N_BUFFER;	
			n_buffer.N_Buffersize[ i ] = N_BUFFER_size;
			returncode = i;
			break;
		}
	}
	
	return( returncode );
}


char * N_BUFFER_getLayerPointer( int Layer );
{
	int i;
	
	if ( Layer >= n_buffer.Layers )
		return( NULL );

	return( n_buffer.N_Buffer[ Layer ] );
}

int N_BUFFER_getLayerSize( int Layer );
{
	int i;

	if ( Layer >= n_buffer.Layers )
		return( NULL );
	
	return( n_buffer.N_Buffersize[ Layer ] );
}

int N_BUFFER_getLayers( void )
{
	return( n_buffer.Layers );
}

int N_BUFFER_getBuffersize( void )
{
	return( n_buffer.Buffersize );
}

char * N_BUFFER_getNextBuffer( void )
{
	return( n_buffer.N_BUFFER_next_Pointer );
}

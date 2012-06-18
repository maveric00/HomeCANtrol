/***************************************************************************
 *            n_buffer.h
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

#ifndef N_BUFFER_H
	#define N_BUFFER_H

	#define MAX_BUFFER_LAYERS	10

	void N_BUFFER_clean( void );
	int	N_BUFFER_addLayer( char * N_BUFFER, int N_BUFFER_size );
	char * N_BUFFER_getlastLayer( void );
	
	struct N_BUFFER {
		int				Buffersize;
		int				Layers;
		char		*	N_BUFFER_next_Pointer;
		char 		*	N_Buffer[ MAX_BUFFER_LAYERS ];		
		int				N_Buffersize[ MAX_BUFFER_LAYERS ];		
	};
#endif

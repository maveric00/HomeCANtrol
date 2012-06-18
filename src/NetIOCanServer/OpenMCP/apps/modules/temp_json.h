/***************************************************************************
 *            temp_json.h
 *
 *  Sun Dec 20 00:51:51 2009
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

#ifndef TEMP_JSON_H
	#define TEMP_JSON_H

	void temp_json_init( void );
	void temp_json_thread( void );
	void temp_json_cgi( void * pStruct );
	void temp_logger_cgi( void * pStruct );
	void temp_logger_clean( void );

	#define TEMP_MAX	24

	struct TEMP_JSON{
		char	Jahr;
		char	Monat;
		char	Tag;
		char	Bezeichner[32];
		int		temp[ TEMP_MAX ];
	};

#endif

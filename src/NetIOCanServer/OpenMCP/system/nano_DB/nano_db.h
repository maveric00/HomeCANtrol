/*! \file nana_db.h \brief Funktionen für das Arbeiten mit nanoDB. */
/***************************************************************************
 *            nano_db.h
 *
 *  Mon Dec 28 20:53:51 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup	nanoDB 
//****************************************************************************
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

#ifndef NANO_DB_H
	#define NANO_DB_H

	#define nanoDB_ERROR	-1
	#define	nanoDB_OK		0

	int nano_DB_makeDB( char * FULLNAME );
	int nano_DB_readDBentry( char * FULLNAME, long DBentryNumber, void * DB, int DBlenght );
	int nano_DB_writeDBentry( char * FULLNAME, long DBentryNumber, void * DB, int DBlenght );
	int nano_DB_getnumbersofDB( char * FULLNAME, int DBlenght );

	char * nano_DB_getfilename( char * FULLNAME );
	void nano_DB_enterDir( char * DIRECTORYNAME, void * dir_struct );

#endif

//@}
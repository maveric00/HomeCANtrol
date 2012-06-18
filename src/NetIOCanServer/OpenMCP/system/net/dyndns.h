/*! \file dyndns.h \brief Stellt Funktionen für dyndns.org bereit. */
/***************************************************************************
 *            dyndns.h
 *
 *  Sat Sep 19 14:57:17 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
//****************************************************************************/
///	\ingroup system
///	\defgroup DYNDNS Stellt Funktionen für dyndns.org bereit. (dyndns.h)
///	\code #include "dyndns.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen für dyndns.org bereit.
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
#ifndef __DYNDNS_H__

	#define __DYNDNS_H__

	int DYNDNS_updateIP( char * userpw, char * domain );
	int DYNDNS_pharseHTTPheader ( unsigned int socket, int * Contentlenght );
	long DYNDNS_getPublicIP( void );

	#define DYNDNS_FAILED	1
	#define	DYNDNS_OK		0

#endif
//@}

/*! \file twitter.h \brief Stellt Funktionen für das twittern bereit. */
//***************************************************************************
//*            twitter.h
//*
//*  Son Aug 15 19:19:28 2009
//*  Copyright  2009  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup TWITTER Stellt Funktionen für das twittern bereit. (twitter.h)
///	\par Uebersicht
///		Stellt Funktionen für das twittern bereit.
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
 
#ifndef _TWITTER_H
	#define TWITTER_H

	#define TWITTER_OK		0
	#define TWITTER_FAILD   1

	int TWITTER_sendtweet( char * tweet, char * userpw );
	int TWITTER_sendtweet_P( const char * tweet, const char * userpw );

#endif /* TWITTER_H */


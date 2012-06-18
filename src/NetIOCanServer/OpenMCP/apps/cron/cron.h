/*! \file "cron.h" \brief Der Crondienst */
//***************************************************************************
//*            cron.c
//*
//*  Wed Jun  17 23:18:31 2009
//*  Copyright  2009  Dirk Broßwick
//*  Email
//****************************************************************************/
///	\ingroup cron
///	\defgroup Der Crondienst (cron.h)
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
#ifndef _CRON_H
	#define CRON_H

	#define MAX_CRON		5
	#define MAX_CRON_ARGC   3

	void CRON_init( void );
	void CRON_thread( void );
	int CRON_checkhit( int entry );
	void CRON_exec( int entry );
	int CRON_getfreeentry( void );
	int CRON_addentry( char * cronstring );
	int CRON_delentry( int entry );
	int CRON_findentry( char * cronstring );
	int CRON_getentry( char * cronstring, int entry );
	int CRON_pharseentry( char * cronstring, char ** argv );
	void CRON_printcrontable( void );
	void CRON_reloadcrontable( void );

	/*! \struct CRON
	 *  \brief Definiert die Struktur eines Cron-Eintrages.
	 */
	struct CRON {
		int	mm;				/*!< Eintrag für die Minute wann der Cron ausgeführt werden soll. */
		int	hh;				/*!< Eintrag für die Sründe wann der Cron ausgeführt werden soll. */
//		char	DD;				
		int		CMD_ENTRY;		/*!< Eintrag welcher Cron ausgeführt werden soll */
	};

#endif /* CRON_H */
//@}

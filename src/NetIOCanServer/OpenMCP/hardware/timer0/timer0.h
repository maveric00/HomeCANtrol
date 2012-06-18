/*!\file timer0.h \brief Stellt den Timer0 im CTC mode bereit */
//***************************************************************************
//*            timer0.h
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  User
//*  Email
//****************************************************************************/
//@{
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
 

#ifndef _TIMER0_H
	#define _TIMER0_H

	typedef void ( * TIMER0_CALLBACK_FUNC ) ( void );
	
	#define MAX_TIMER0_CALLBACKS		1

	#define FALSE				0
	#define TRUE				(!FALSE)
	
	void timer0_init( int Hz );
	void timer0_stop(void);
	void timer0_free(void);
	unsigned char timer0_RegisterCallbackFunction( TIMER0_CALLBACK_FUNC pFunc );	
	unsigned char timer0_RemoveCallbackFunction( TIMER0_CALLBACK_FUNC pFunc );
	
#endif /* _TIMER0_H */
//@}

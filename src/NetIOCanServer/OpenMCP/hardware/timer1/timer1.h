/*!\file timer1.h \brief Stellt den Timer1 im CTC mode bereit */
//***************************************************************************
//*            timer1.h
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

#ifndef _TIMER1_H
	#define _TIMER1_H

	typedef void ( * TIMER1_CALLBACK_FUNC ) ( void );
	
	#define MAX_TIMER1_CALLBACKS		1

	#define FALSE				-1
	#define TRUE				0
	
	int timer1_init( unsigned int Hz , unsigned int timedrift );
	void timer1_stop(void);
	void timer1_free(void);
	unsigned char timer1_RegisterCallbackFunction( TIMER1_CALLBACK_FUNC pFunc );	
	unsigned char timer1_RemoveCallbackFunction( TIMER1_CALLBACK_FUNC pFunc );
	void timer1_wait( int counter );
	unsigned int timer1_getcounter( void );
	unsigned int timer1_gettimerbase( void );

#endif /* _TIMER1_H */

//@}

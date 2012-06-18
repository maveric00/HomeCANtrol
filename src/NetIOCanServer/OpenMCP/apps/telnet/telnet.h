/*! \file telnet.h \brief Ein sehr einfacher Telnetclient */
/***************************************************************************
 *            telnet.h
 *
 *  Mon Jun 19 21:56:05 2006
 *  Copyright  2006 Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
///	\ingroup software
///	\defgroup telnet Ein sehr einfacher Telnetclient (telnet.h)
///	\code #include "telnet.h" \endcode
//****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
//@{
#ifndef _TELNET_H
	#define _TELNET_H

	#include <avr/pgmspace.h>  

	#define TELNET_PORT 23
	#define TELNET_BUFFER_LEN 64

	#define MAX_TELNET_ENTRYS	12
	#define MAX_ARGC	4

	typedef int ( * DYN_TELNET_CALLBACK ) ( int argc, char ** argv );

	struct DYN_TELNET {
		volatile	DYN_TELNET_CALLBACK		dyntelnet_function;
		const 		prog_char 				* functionname;
	};

	struct TELNET_SESSION {
		int			SOCKET;
		char		BUFFER [ TELNET_BUFFER_LEN ];
		int			POS;
		int			STATE;
		int			argc;
		char *		argv[ MAX_ARGC ];
	};

	void telnet_init( void );
	void telnet_thread( void );
	int telnet_RegisterCMD( DYN_TELNET_CALLBACK dyntelnet_function, const prog_char * funktionname );
	int TELNET_pharse( char * BUFFER, char ** argv, int max_argc );
	int TELNET_runcmd( int argc, char ** argv );
	int TELNET_runcmdextern( char * cmdstring );
	int TELNET_runcmdextern_P( const prog_char * cmdstring_P );


#endif /* _TELNET_H */
//@}

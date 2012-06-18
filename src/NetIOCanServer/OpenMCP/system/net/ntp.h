/*! \file "ntp.h" \brief Die ntp-Funktionlitaet */
//***************************************************************************
//*            ntp.h
//*
//*  Mon Aug 28 11:41:21 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************
///	\ingroup network
///	\defgroup NTP NTP-Funktionen (ntp.h)
///	\code #include "ip.h" \endcode
///	\code #include "dns.h" \endcode
///	\code #include "tcp.h" \endcode
///	\code #include "ntp.h" \endcode
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
#ifndef _NTP_H
	#define _NTP_H

	unsigned int NTP_GetTime( unsigned long IP, unsigned char * dnsbuffer, long timedif );
	
	#define NTP_OK		0
	#define NTP_ERROR	0xffff
	
	#define MAX_NTP_FAILED	5

	#define SecondsPerDay	86400
	#define SecondsPerHour	3600
	#define SecondsPerMin	60
	
	union DATE {
		unsigned char DateByte[4];
		unsigned long Date;
	};

#endif /* _NTP_H */
//@}

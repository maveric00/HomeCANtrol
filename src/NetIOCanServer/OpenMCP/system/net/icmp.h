/*! \file icmp.h \brief Stellt Funktionen für ICMP/Ping bereit. */
/***************************************************************************
 *            icmp.h
 *
 *  Sat Jun  3 18:53:56 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
//****************************************************************************/
///	\ingroup network
///	\defgroup ICMP Stellt Funktionen für ICMP/Ping bereit. (icmp.h)
///	\code #include "icmp.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen für ICMP/Ping bereit.
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


#ifndef __ICMP_H__
	
	#define __ICMP_H__

	void icmp( unsigned int packet_lenght, unsigned char *buffer);

	struct ICMP_header{
		unsigned char 	ICMP_type;
		unsigned char 	ICMP_code;
		unsigned char 	ICMP_checksumByteOne;
		unsigned char 	ICMP_checksumByteTwo;
		unsigned int  	ICMP_Identifierer;
		unsigned int  	ICMP_Seqnumber;
		unsigned char	Payload;
	};

	#define ICMP_HEADER_LENGHT	

	#define ICMP_EchoReplay		0
	#define ICMP_EchoRequest	8
	#define ICMP_TimeExceeded	11

	#define ICMP_NoWaitForReplay	0
	#define ICMP_WaitForReplay		1
	#define ICMP_ReplayOkay			2
	#define ICMP_Unknow				-1

#endif

//}@

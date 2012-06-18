/*!\file ip.h \brief Stellt die IP Funktionalitaet bereit*/
//***************************************************************************
//*            ip.h
//*
//*  Mon Jul 31 21:46:47 2007
//*  Copyright  2007  sharandac
//*  Email
///	\ingroup network
///	\defgroup IP Der IP Stack fuer Mikrocontroller (ip.h)
///	\code #include "arp.h" \endcode
///	\code #include "ethernet.h" \endcode
///	\code #include "ip.h" \endcode
///	\code #include "icmp.h" \endcode
///	\code #include "udp.h" \endcode
///	\code #include "tcp.h" \endcode
///	\par Uebersicht
///		Der IP-Stack für Mikrocontroller. Behandelt die Verarbeitung der eingehenden IP
/// Packete und reicht diese weiter an UDP, TCP und ICMP.
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

#ifndef __IP_H__

	#define __IP_H__


// berechnet die Broadcast-Adresse bei gegebener IP-Adresse und Netzmaske
#define CALC_BROADCAST_ADDR( ip, mask ) ( ip | ~mask )

// Testet, ob eine Adresse die Broadcast-Adresse is (zu einer Netzwerkmaske)
#define IS_BROADCAST_ADDR( ip, mask ) ( ( ip & ~mask ) == ~mask)

// Schaut ob Ziel-IP in diesen Subnet liegt 
#define IS_ADDR_IN_MY_SUBNET( ip, mask ) ( ( ip & mask ) == ( myIP & mask ) )

	extern unsigned long myIP;
	extern unsigned long myBroadcast ;

	extern unsigned long Netmask;

	extern unsigned long Gateway;

	extern unsigned long DNSserver;

	void ip( unsigned int packet_lenght , unsigned char *buffer );
	void MakeIPheader( unsigned long SourceIP, unsigned char Protocoll, unsigned int Datalenght , unsigned char *ethernetbuffer );
	unsigned long strtoip( unsigned char * buffer );
	char * iptostr( unsigned long IP, char * strIP );


	#define IP_HEADER_LENGHT 20

	// Convert dot-notation IP address into 32-bit word. Example: IPDOT(192l,168l,1l,1l)
	#define IPDOT( d, c, b, a ) ((a<<24)|(b<<16)|(c<<8)|(d))

	union IP_ADDRESS {
		unsigned char IPbyte[4];
		unsigned long IP;
	};
	
	struct IP_header{
		unsigned char IP_Version_Headerlen;
		unsigned char IP_TOS;
		unsigned int IP_Totallenght;
		unsigned int IP_Identification;
		unsigned char IP_Flags;
		unsigned char IP_Fragmentoffset;
		unsigned char IP_TTL;
		unsigned char IP_Protocol;
		unsigned int IP_Headerchecksum;
		unsigned long IP_SourceIP;
		unsigned long IP_DestinationIP;
	};
	
	#define PROTO_ICMP 0x01
	#define PROTO_TCP 0x06
	#define PROTO_UDP 0x11
	

#endif
//@}

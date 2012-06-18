/*! \file ethernet.h \brief Stellt Funktionen zum senden und empfangen bereit. */
/***************************************************************************
 *            ethernet.h
 *
 *  Sat Jun  3 17:25:42 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
///	\ingroup network
///	\defgroup ETHERNET Stellt Funktionen zum senden und empfangen bereit. (ethernet.h)
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

#ifndef __ETHERNET_H__
	
	#define __ETHERNET_H__
	
	extern unsigned char mymac[6];
	extern unsigned long PacketCounter;
	extern unsigned long ByteCounter;
	
	void ethernetloop( void );
	unsigned int getEthernetframe( unsigned int maxlen, unsigned char *buffer);
	void MakeETHheader( unsigned char * MACadress , unsigned char * buffer );
	void sendEthernetframe( unsigned int packet_lenght, unsigned char *buffer);
	void EthernetInit( void );
	void LockEthernet( void );
	void FreeEthernet( void );
	void alive( void );
	
	#define ETHERNET_MIN_PACKET_LENGTH	0x3C
	#define ETHERNET_HEADER_LENGTH		14

	#ifdef __AVR_ATmega2561__
		#define interrupt					4
	#endif
	#ifdef __AVR_ATmega644__
		#define interrupt					2
	#endif
	#ifdef __AVR_ATmega644P__
		#define interrupt					3
		#define PCINT_Pin					7
	#endif

	struct ETH_header {
		unsigned char ETH_destMac[6];	
		unsigned char ETH_sourceMac[6];
		unsigned int ETH_typefield;
	};

#endif
//@}

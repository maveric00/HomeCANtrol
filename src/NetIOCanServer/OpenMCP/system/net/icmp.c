/*! \file icmp.c \brief Stellt Funktionen für ICMP/Ping bereit. */
/***************************************************************************
 *            icmp.c
 *
 *  Sat Jun  3 18:53:45 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
//****************************************************************************/
///	\ingroup network
///	\defgroup ICMP Stellt Funktionen für ICMP/Ping bereit. (icmp.c)
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

#include "arp.h"
#include "ethernet.h"
#include "ip.h"
#include "icmp.h"
#include "system/math/checksum.h"
#include "system/math/math.h"

static char ICMP_Replaystate = ICMP_Unknow;

void icmp( unsigned int packet_lenght, unsigned char *buffer)
	{
		unsigned char i;
		unsigned int checksum;
		
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *)&buffer[0];
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *)&buffer[ETHERNET_HEADER_LENGTH];
		struct ICMP_header *ICMP_packet;	// ICMP_struct anlegen
		ICMP_packet = ( struct ICMP_header *)&buffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		switch ( ICMP_packet->ICMP_type )
			{
				case ICMP_EchoRequest:			//IP_header unbauen zum versenden 
												IP_packet->IP_DestinationIP = IP_packet->IP_SourceIP;
												IP_packet->IP_SourceIP = myIP;
												//IP_header checksummer ausrechnen
												IP_packet->IP_Headerchecksum = 0x0;
												checksum = Checksum_16( &buffer[14], ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 ) );
												IP_packet->IP_Headerchecksum = ChangeEndian16bit( checksum );
												ICMP_packet->ICMP_type = ICMP_EchoReplay; // auf reply einstellen
												ICMP_packet->ICMP_code = 0x00; 
												//Simple ICMP Checksummenbildung, die Idee stammt von
												//Simon, siehe http://avr.auctionant.de/
												if(ICMP_packet->ICMP_checksumByteOne >  0xFF-0x08)ICMP_packet->ICMP_checksumByteTwo++;
												ICMP_packet->ICMP_checksumByteOne+=0x08;
												// Ethernetframe bauen
												for(i = 0; i < 6 ; i++ ){	
													ETH_packet->ETH_destMac[i] = ETH_packet->ETH_sourceMac[i];	
													ETH_packet->ETH_sourceMac[i] = mymac[i]; }
												// und ab die post
												sendEthernetframe( packet_lenght, buffer); // packet_lenght - 4 weil der Controller die checksumme selber berechnet
												break;

				case ICMP_EchoReplay:			if ( ICMP_packet->ICMP_Identifierer == 0xac1d && ICMP_Replaystate == ICMP_WaitForReplay )
													ICMP_Replaystate = ICMP_ReplayOkay;
												break;
			}
	}

int	icmp_ping( unsigned long IP )
{
}
//}@

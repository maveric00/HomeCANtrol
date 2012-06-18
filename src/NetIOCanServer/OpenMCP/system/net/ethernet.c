/*! \file ethernet.c \brief Stellt Funktionen zum senden und empfangen bereit. */
/***************************************************************************
 *            ethernet.c
 *
 *  Sat Jun  3 17:25:42 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
///	\ingroup network
///	\defgroup ETHERNET Stellt Funktionen zum senden und empfangen bereit. (ethernet.c)
///	\code #include "ethernet.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen zum senden und empfangen bereit.
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

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>

#include "config.h"

#include "hardware/network/enc28j60.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "tcp.h"

/*
#if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#if defined(OpenMCP) || defined(AVRNETIO)
		#include "hardware/ext_int/ext_int.h"
	#elif defined(myAVR)
		#include "hardware/timer0/timer0.h"
		#include "hardware/led/led_core.h"
		#include "hardware/pcint/pcint.h"
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
*/

#if defined(EXTINT)
	#include "hardware/ext_int/ext_int.h"
#endif
#if defined(LED)
	#include "hardware/led/led_core.h"
#endif
#if defined(__AVR_ATmega644P__) && defined(myAVR)
	#if defined(PC_INT)
		#include "hardware/pcint/pcint.h"
	#else
		#error "Bitte PC_INT in der config.h aktivieren!"
	#endif
#endif

unsigned char mymac[6] = { ENC28J60_MAC0,ENC28J60_MAC1,ENC28J60_MAC2,ENC28J60_MAC3,ENC28J60_MAC4,ENC28J60_MAC5 };
unsigned long PacketCounter;
unsigned long ByteCounter;

/*
 -----------------------------------------------------------------------------------------------------------
   Die Routine die die Packete nacheinander abarbeitet
------------------------------------------------------------------------------------------------------------*/

void ethernet(void)
	{
		unsigned int packet_lenght;

		unsigned char ethernetbuffer[ MAX_FRAMELEN ];
		
#if defined(__AVR_ATmega644P__) && defined(myAVR) && defined(LED)
		LED_on(1);
#endif
		// hole ein Frame
		packet_lenght = getEthernetframe( MAX_FRAMELEN, ethernetbuffer);
		// wenn Frame vorhanden packet_lenght != 0
		// arbeite so lange die Frames ab bis keine mehr da sind
		
		while ( packet_lenght != 0 )
			{
				PacketCounter++;
				ByteCounter = ByteCounter + packet_lenght;
				struct ETH_header *ETH_packet; 		//ETH_struc anlegen
				ETH_packet = (struct ETH_header *) ethernetbuffer; 
				switch ( ETH_packet->ETH_typefield ) // welcher type ist gesetzt 
					{
					case 0x0608:		
										#ifdef _DEBUG_
											printf_P( PSTR("-->> ARP\r\n") );
										#endif
										arp( packet_lenght , ethernetbuffer );
										break;
					case 0x0008:		
										#ifdef _DEBUG_
											printf_P( PSTR("-->> IP\r\n") );										
										#endif
										ip( packet_lenght , ethernetbuffer );
										break;
					}
#if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega644__)
	#if defined(OpenMCP) || defined(AVRNETIO)
				packet_lenght = 0;
			}	
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
		
#if defined(__AVR_ATmega644P__)
	#if defined(myAVR)
				packet_lenght = getEthernetframe( MAX_FRAMELEN, ethernetbuffer);
			}
		#if defined(LED)
			LED_off(1);
		#endif
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
		return;
	}

/* -----------------------------------------------------------------------------------------------------------
Holt ein Ethernetframe
------------------------------------------------------------------------------------------------------------*/
unsigned int getEthernetframe( unsigned int maxlen, unsigned char *ethernetbuffer)
	{
		return( enc28j60PacketReceive( maxlen , ethernetbuffer) );
	}
	
/* -----------------------------------------------------------------------------------------------------------
Sendet ein Ethernetframe
------------------------------------------------------------------------------------------------------------*/
void sendEthernetframe( unsigned int packet_lenght, unsigned char *ethernetbuffer)
	{
#if defined(__AVR_ATmega644P__) && defined(myAVR) && defined(LED)
		LED_on(1);
#endif
		PacketCounter++;
		ByteCounter = ByteCounter + packet_lenght;
 		enc28j60PacketSend( packet_lenght, ethernetbuffer );
#if defined(__AVR_ATmega644P__) && defined(myAVR) && defined(LED)
		LED_off(1);
#endif
	}
	
/* -----------------------------------------------------------------------------------------------------------
Erstellt den richtigen Ethernetheader zur passenden Verbindung die gerade mit TCP_socket gewählt ist
------------------------------------------------------------------------------------------------------------*/
void MakeETHheader( unsigned char * MACadress , unsigned char * ethernetbuffer )
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;

		unsigned int i;			

		ETH_packet->ETH_typefield = 0x0008;
		
		for ( i = 0 ; i < 6 ; i++ ) 
		{
			ETH_packet->ETH_sourceMac[i] = mymac[i];			
			ETH_packet->ETH_destMac[i] = MACadress[i];
		}
		return;		
	}

/* -----------------------------------------------------------------------------------------------------------
Erstellt den richtigen Ethernetheader zur passenden Verbindung die gerade mit TCP_socket gewählt ist
------------------------------------------------------------------------------------------------------------*/
void LockEthernet( void )
{

	LockTCP();

#if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#if defined(OpenMCP) || defined(AVRNETIO)
		EXTINT_block( interrupt );
	#elif defined(myAVR)
		PCINT_disablePCINT( interrupt );
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

}

/* -----------------------------------------------------------------------------------------------------------
Erstellt den richtigen Ethernetheader zur passenden Verbindung die gerade mit TCP_socket gewählt ist
------------------------------------------------------------------------------------------------------------*/
void FreeEthernet( void )
{
	FreeTCP();

#if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#if defined(OpenMCP) || defined(AVRNETIO)
		EXTINT_free ( interrupt );
	#elif defined(myAVR)
		char sreg_tmp;
		sreg_tmp = SREG;    /* Sichern */
		cli();
		PCINT_enablePIN( PCINT_Pin, interrupt );
		PCINT_enablePCINT( interrupt );
		SREG = sreg_tmp;
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
}

/* -----------------------------------------------------------------------------------------------------------
führt den Init durch
------------------------------------------------------------------------------------------------------------*/
void EthernetInit( void )
{
		// ENC Initialisieren //
#if defined(__AVR_ATmega644P__)
	#if defined(myAVR)
		PCINT_init();
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
		enc28j60Init();
		nicSetMacAddress( mymac );

		// Alle Packet lesen und ins leere laufen lassen damit ein definierter zustand herrscht
		unsigned char ethernetbuffer[ MAX_FRAMELEN ];
		while ( getEthernetframe( MAX_FRAMELEN, ethernetbuffer) != 0 ) { };
		
#if defined(__AVR_ATmega2561__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)
	#if defined(OpenMCP) || defined(AVRNETIO)
		EXTINT_set ( interrupt , SENSE_LOW , ethernet );
	#elif defined(myAVR)
		PCINT_set( interrupt, ethernet );
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
		// gibt Ethernet frei
		FreeEthernet();
}
//@}

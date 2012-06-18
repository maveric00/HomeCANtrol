/*!\file network.c \brief Init des Netzwerkteils */
/****************************************************************************
 *            network.c
 *
 *  Sat Feb 28 03:26:56 2009
 *  Copyright  2009  Dirk Broßwick
 *  Email sharandac(at)snafu.de
///	\ingroup network
///	\code #include "arp.h" \endcode
///	\code #include "ethernet.h" \endcode
///	\code #include "ip.h" \endcode
///	\code #include "icmp.h" \endcode
///	\code #include "dns.h" \endcode
///	\code #include "dhcp.h" \endcode
///	\code #include "ntp.h" \endcode
///	\code #include "udp.h" \endcode
///	\code #include "tcp.h" \endcode
 ****************************************************************************/
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
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/version.h>

#include "config.h"

#include "system/stdout/stdout.h"
#include "hardware/network/enc28j60.h"
#include "system/net/ethernet.h"
#include "system/net/ip.h"
#include "system/net/arp.h"
#include "system/net/udp.h"
#include "system/net/dhcpc.h"
#include "system/net/ntp.h"
#include "system/clock/clock.h"
#include "system/config/config.h"
#include "system/string/string.h"
#if defined(LED)
	#include "hardware/led/led_core.h"
#endif
#if defined(TCP)
	#include "system/net/tcp.h"
#endif

void network_init( void )
{
	int i;
	char REVID;
	// Union für IP
	union IP_ADDRESS IP;
	
	// struct für die Zeit anlegen
	struct TIME Time;
	
	char ip[32];
	long timedif = 0;
	
	// Ethernet starten
	if (  checkConfigName_P( PSTR("MAC") ) != -1 ) 
	{
		readConfig_P ( PSTR("MAC"), ip );
		strtobin( ip, mymac, 12 );
	}
	
	// Ethernet starten
	EthernetInit();
/*
	REVID = enc28j60Read( EREVID );

	switch( REVID )
	{
		case	0:	i = 0;
					break;
		case	2:	i = 2;
					break;
		case	4:	i = 4;
					break;
		case	5:	i = 5;
					break;
		case	6:	i = 7;
					break;
		default:	i = 0;
					break;
	}

	for ( ; i>0 ; i-- )
	{
		LED_on(0);
		CLOCK_delay( 500 );
		LED_off(0);
		CLOCK_delay( 500 );
	}
	LED_on(0);
*/
	printf_P( PSTR("ENC28j60 initialisiert ( HW-Add: %02x:%02x:%02x:%02x:%02x:%02x )"), mymac[ 0 ] , mymac[ 1 ] , mymac[ 2 ] , mymac[ 3 ] , mymac[ 4 ] , mymac[ 5 ] );
	
	#ifdef ETH_LINK_CONFIG
	if (  checkConfigName_P( PSTR("ETH_FLDX") ) != -1 ) 
	{
		readConfig_P ( PSTR("ETH_FLDX"), ip );
		if ( !strcmp_P( ip, PSTR("on") ) )
		{
			enc28j60EnableFullDuplex();
			printf_P( PSTR(" Fullduplex:"));
		}
		else
		{
			enc28j60EnableHalfDuplex();
			printf_P( PSTR(" Halfduplex:"));
		}			
	}
	else
	{
		enc28j60EnableHalfDuplex();
		printf_P( PSTR(" Halfduplex:"));
	}			
	#endif

	#ifdef ETH_LINK_FULL
		enc28j60EnableFullDuplex();
		printf_P( PSTR(" Fullduplex:"));
	#endif
	
	#ifdef ETH_LINK_HALF
		enc28j60EnableFullDuplex();
		printf_P( PSTR(" Fullduplex:"));
	#endif

	#ifndef ETH_LINK_CONFIG
		#ifndef ETH_LINK_FULL
			#ifndef ETH_LINK_HALF
				enc28j60EnableFullDuplex();
				printf_P( PSTR(" Fullduplex:"));
			#endif
		#endif
	#endif
	
	while( enc28j60Linkcheck() );
	
	printf_P( PSTR(" Link ready\r\n"));

	// ARP starten
	ARP_INIT ();
	printf_P( PSTR("-+-> ARP initialisiert\r\n"));
	
	#ifdef UDP
		// UDP starten
		UDP_init();
		printf_P( PSTR(" |-> UDP (Tornado-engine) initialisiert\r\n"));
	#endif
		
	#ifdef TCP
		// tcp starten
		tcp_init();
		printf_P( PSTR(" |-> TCP (Hurrican-engine) initialisiert\r\n"));
	#endif

#ifdef DHCP
	if (  checkConfigName_P( PSTR("DHCP") ) == -1 ) 
	{
		// DHCP-Config holen
		printf_P( PSTR(" |-> Versuche DHCP-Config zu holen. "));
		if ( !DHCP_GetConfig () )
		{
			printf_P( PSTR("DHCP-Config geholt\r\n"));
		}
		else 
		{
			printf_P( PSTR("DHCP-Config Fehlgeschlagen\r\n"));
		}
	}
	else
	{
		readConfig_P ( PSTR("DHCP"), ip );
		if ( !strcmp_P( ip, PSTR("on") ) )
		{
			// DHCP-Config holen
			printf_P( PSTR(" |-> Versuche DHCP-Config zu holen. "));
			if ( !DHCP_GetConfig () ) printf_P( PSTR("DHCP-Config geholt\r\n"));
			else 
			{
				printf_P( PSTR("DHCP-Config Fehlgeschlagen\r\n"));
				#ifdef READ_CONFIG
					readConfig_P ( PSTR("IP"), ip );
					myIP = strtoip( ip );
					readConfig_P ( PSTR("MASK"), ip );
					Netmask = strtoip( ip );
					readConfig_P ( PSTR("GATE"), ip );
					Gateway = strtoip( ip );
					#ifdef DNS
						readConfig_P ( PSTR("DNS"), ip );
						DNSserver = strtoip( ip );
					#endif
				#endif
			}
		}
		#ifdef READ_CONFIG
		else
		{
			if ( readConfig_P ( PSTR("IP"), ip ) != -1)
				myIP = strtoip( ip );
			if ( readConfig_P ( PSTR("MASK"), ip ) != -1)
				Netmask = strtoip( ip );
			if ( readConfig_P ( PSTR("GATE"), ip ) != -1)
				Gateway = strtoip( ip );
			#ifdef DNS
				if ( readConfig_P ( PSTR("DNS"), ip ) != -1)
					DNSserver = strtoip( ip );
			#endif
		}
		#endif
	}
#endif
	
#ifndef DHCP
	#ifdef READ_CONFIG
		if ( readConfig_P ( PSTR("IP"), ip ) != -1)
			myIP = strtoip( ip );
		if ( readConfig_P ( PSTR("MASK"), ip ) != -1)
			Netmask = strtoip( ip );
		if ( readConfig_P ( PSTR("GATE"), ip ) != -1)
			Gateway = strtoip( ip );
		#ifdef DNS
			if ( readConfig_P ( PSTR("DNS"), ip ) != -1)
				DNSserver = strtoip( ip );
		#endif
	#endif
#endif
	
	myBroadcast = (myIP|0xff000000) ;
	
	printf_P( PSTR(	" |   IP     : %s\r\n"), iptostr( myIP, ip ));
	printf_P( PSTR(	" |   Netmask: %s\r\n"), iptostr( Netmask, ip ));
	printf_P( PSTR(	" |   Gateway: %s\r\n"), iptostr( Gateway, ip ) );
	#ifdef DNS
		printf_P( PSTR(" |   DNS    : %s\r\n"), iptostr( DNSserver, ip ) );
	#endif
		
#ifdef NTP
	// Uhr einstellen
	if( readConfig_P( PSTR("NTP"), ip ) != -1 )
	{
		if ( !strcmp_P( ip, PSTR("on") ) )
		{
			printf_P( PSTR(" |-> NTP-Server Zeit aktualisieren:"));
			if( readConfig_P ( PSTR("UTCZONE"), ip ) != -1 )
			{
				timedif = atol( ip );
				readConfig_P ( PSTR("NTPSERVER"), ip );
				if( NTP_GetTime( 0 , ip, timedif ) == NTP_OK )
				{
					CLOCK_GetTime ( &Time );
					printf_P( PSTR(" Zeit: %02d:%02d:%02d.%02d\r\n"),Time.hh,Time.mm,Time.ss,Time.ms);
				}
				else
					printf_P( PSTR(" fehlgeschlagen\r\n"));
			}
			else
				printf_P( PSTR(" fehlgeschlagen\r\n"));
		}
	}
#endif

#if defined(LED)
	#ifdef __AVR_ATmega2561__
		LED_on(2);
	#endif	
	#if defined(__AVR_ATmega644P__) && defined(myAVR) && defined(LCD)
			STDOUT_set( _LCD, 0);	
	
			printf_P( PSTR("\r\n%s"), iptostr( myIP, ip ) );
	
			STDOUT_set( RS232, 0);	
	#endif
#endif
}
//@}

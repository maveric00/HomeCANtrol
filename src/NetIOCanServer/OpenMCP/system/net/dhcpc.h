/*! \file dhcpc.h \brief Stellt die DHCP-Client Funktionalitaet bereit */
/***************************************************************************
 *            dhcpc.h
 *
 *  Mon Aug 28 22:31:24 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email sharandac(at)snafu.de
 ****************************************************************************/

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
 
#ifndef _DHCPC_H
	#define _DHCPC_H

	unsigned int DHCP_GetConfig( void );
	unsigned int DHCP_SendDiscover( unsigned int SOCKET, unsigned char * DHCPbuffer, unsigned char * Configbuffer );
	unsigned int DHCP_SendRequest( unsigned int SOCKET, unsigned char * DHCPbuffer, unsigned char * Configbuffer );
	unsigned int DHCP_AddOption( unsigned char Option, unsigned long ExtraData, unsigned char * Optionfield );
	unsigned int DHCP_PharseOption( void * Configbuffer, unsigned char * Optionfield );
	void DHCP_buildheader( unsigned char * DHCPbuffer );

	/*! \union LONG_BYTE
	 *  \brief Die Union fuer den Byteweisen zugriff auf long
	 */
	union LONG_BYTE {
		unsigned char BYTE[4];
		unsigned long LONG;
	}; 
	
	#define DHCP_CONFIG_LENGHT			24

	/*! \struct DHCP_CONFIG
	 *  \brief Definiert die Struktur der DHCP-Konfiguration die beim Aushandeln ermittelt wird.
	 */
	struct DHCP_CONFIG {
		union LONG_BYTE Client_IP; 			/*!< Speichert die eigene IP die Zugewiesen wird. */
		union LONG_BYTE Router_IP;			/*!< Speichert die IP des naechsten Gateway. */
		union LONG_BYTE Subnetmask;			/*!< Speichert die zugewiesene Submetmask. */
		union LONG_BYTE DNS_IP;				/*!< Speichert die IP des zugewiesenen DNS-Server. */
		union LONG_BYTE Server_IP;			/*!< Speichert die Server-ID. */
		union LONG_BYTE Leasetime;			/*!< Speichert die Leasetime. */
	};	
		
	/*! \struct DHCP_HEADER
	 *  \brief Definiert die Struktur des DHCP-Header
	 */
	struct DHCP_HEADER {
		unsigned char Messages_Type;
		unsigned char Hardware_Type;
		unsigned char Hardware_Adress_Lenght;
		unsigned char Hops;
		unsigned long Transaction_ID;
		unsigned int Second_elapsed;
		unsigned int Bootp_Flags;
		union LONG_BYTE Client_IP;
		union LONG_BYTE Your_IP;
		union LONG_BYTE Server_IP;
		union LONG_BYTE Relay_IP;
		unsigned char Your_MACADRESS[16];
		unsigned char Servername[64];
		unsigned char Boot_Filename[128];
		unsigned long Magic_Cookie;
		unsigned char Options[96];
	};

	#define DHCP_REQUEST_TIMEOUT		1000 // in 1/100 sekunden
	
	#define SERVERNAME_LENGHT			64
	#define BOOT_FILENAME_LENGHT		128
	#define OPTION_LENGHT				96
	
	#define DHCP_HEADER_LENGHT			336
	
	// Das Magic-Cookie, markiert den Anfang des Optionsblocks
	#define MAGIC_COOKIE				0x63538263
	
	/*!\name Options
	 * Die Optionwerte die im Optionfeld stehen, nach diesem Wert kommt die laenge des Optioneintrages
	 * \{ */
	#define Option_SUBNETMASK			1		
	#define Option_TIME_OFFSET			2
	#define Option_ROUTER				3
	#define Option_DOMAIN_NAME_SRVER	6
	#define Option_HOST_NAME			12
	#define Option_DOMAIN_NAME			15
	#define Option_BROADCAST_ADRESS		28
	#define Option_REQUESTET_IP_ADRESS  50
	#define Option_IP_ADRESS_LEASETIME  51
	#define Option_DHCP_MESSAGES_TYPE 	53
	#define Option_SERVER_ID			54
	#define Option_OPTIONLIST			55
	#define Option_CLIENT_IDENTIFIER	61
	#define Option_END					255
	 /*! \} */

	/*!\name DHCP_MESSAGES_TYPES
	 * Die Werte fuer die DHCP_MESSAGES_TYPES
	 * \{ */
	#define DHCP_DISCOVER				1
	#define DHCP_OFFER					2
	#define DHCP_REQUEST				3
	#define DHCP_ACK					5
	 /*! \} */
	
	// Die Bootp Messages Types
	#define BOOT_REQUEST				1
	#define BOOT_REPLY					2
	
	/*!\name DHCPC_RETURN_VALUE 
	 * Die Returnwerte die nach dem Aufruf von GetDHCPConfig entstehen.
	 * \{
	 */
	#define DHCPC_OK					0
	#define DHCPC_TIMEOUT				0xffff
	/** \} */
	
	#define HARDWARE_TYPE_ETHERNET		1
	
#endif /* _DHCPC_H */

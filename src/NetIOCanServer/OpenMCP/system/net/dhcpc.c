/*! \file dhcpc.c \brief Stellt die DHCP-Client Funktionalität bereit */
//***************************************************************************
//*            dhcpc.c
//*
//*  Mon Aug 28 22:31:14 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email sharandac(at)snafu.de
//****************************************************************************/
///	\ingroup network
///	\defgroup DHCPC DHCP-Client (dhcpc.c)
///	\code #include "dhcpc.h" \endcode
///	\par Uebersicht
///		Ein DHCP-Client. Uebernimmt die automatische Konfiguration der Netwerkparameter.
/// Wenn dies nicht innerhalb einer bestimmten Zeit gelingt, wird die Standarteinstellung
/// auf ip.c benutzt.
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
#include <string.h>
#include <stdlib.h>
#include "dhcpc.h"
#include "ip.h"
#include "udp.h"
#include "ethernet.h"
#include "system/clock/clock.h"

// #define _DEBUG_

#ifdef _DEBUG_
	#include <avr/pgmspace.h>
	#include <stdio.h>
	#include "hardware/uart/uart.h"
#endif

/*!\brief DHCP-Client Funktion.
 * Bezieht von einem DHCP-Server automatisch eine IP-Konfiguration
 * \return		Ob das beziehen der IP-Konfiguration erfolgreich war, ist an einer 0 zu erkennen, die
 *			zurueck gegeben wird, wenn nicht -> ungleich 0
 */
unsigned int DHCP_GetConfig( void )
	{
		unsigned int UDP_socket;
		
		unsigned long SaveIP;
		
		union LONG_BYTE IP;
		
		SaveIP = myIP;
		myIP = 0x0;
		
		unsigned char * DHCPConfig;
		DHCPConfig = (unsigned char*) __builtin_alloca (( size_t ) DHCP_CONFIG_LENGHT );

		unsigned char * DHCPbuffer;
		DHCPbuffer = (unsigned char*) __builtin_alloca (( size_t ) DHCP_HEADER_LENGHT );
		
		// UDP-socket aufmachen für Bootp
		UDP_socket = UDP_RegisterSocket( 0xffffffff , 67 , DHCP_HEADER_LENGHT , DHCPbuffer);
		// Wenn Fehler aufgetretten, return
		if ( UDP_socket == UDP_SOCKET_ERROR ) 
		{
			myIP = myIP;
			return ( DHCPC_TIMEOUT );
		}
		
		// DHCPC-Discover erzeugen und senden
		#ifdef _DEBUG_
			printf_P( PSTR("\r\nDHCP-Client: DISCOVER ->"));
		#endif
		if ( DHCP_SendDiscover( UDP_socket, DHCPbuffer, DHCPConfig ) == DHCPC_TIMEOUT )
		{
			#ifdef _DEBUG_
				printf_P( PSTR(" ERROR -> Default Config\r\n"));
			#endif
			myIP = SaveIP;
			return( DHCPC_TIMEOUT );
		}
		#ifdef _DEBUG_
			printf_P( PSTR(" OFFER -> REQUEST ->"));
		#endif
		
		// DHCPC-Request erzeugen und senden
		if ( DHCP_SendRequest( UDP_socket, DHCPbuffer, DHCPConfig ) == DHCPC_TIMEOUT )
		{
			#ifdef _DEBUG_
				printf_P( PSTR(" ERROR -> Default Config\r\n"));
			#endif
			myIP = SaveIP;
			return( DHCPC_TIMEOUT );
		}
		
		#ifdef _DEBUG_
			printf_P( PSTR(" ACK -> Config\r\n"));
		#endif
		// Socket schliessen
		UDP_CloseSocket( UDP_socket );
		
		return ( DHCPC_OK );
	}
	
/*!\brief Erstellt eine DHCP-Request Nachricht und sendet sie, anschliessend wird auf die Bestaetigung der selben mit einer DHCP-Ack Nachricht gewartet.
 * Dann wird eine erste Auswertung der Optionfelder durchgefuehrt und endgueltig in die DHCP_CONFIG struct eingetragen. War dieser Vorgang erfolgreich
 * wird ein DHCPC-OK als Returnwert zurueckgegeben, sonst DHCPC-TIMEOUT. Danach enthaelt die DHCP_CONFIG struct eine gültige Config die benutzt werden kann
 * \param		SOCKET			Das UDP-Socket, der zur Kommunikation mit den DHCP-Server benutzt wird .
 * \param		DHCPbuffer		Zeiger auf den UDP-Buffer der zur kommunikation benutzt wird. Dieser enthälrt bei der Kommunikation den DHCP_HEADER.
 * \param 		Configbuffer	Zeiger auf den Configbuffer (DHCP_CONFIG) in der die Koniguration zwischengespeichert wird die der DHCP-Server vorschlaegt.
 * \return 		DHCPC_OK (Wert 0) oder DHCPC_TIMEOUT (ungleich 0)
 */
unsigned int DHCP_SendRequest( unsigned int SOCKET, unsigned char * DHCPbuffer, unsigned char * Configbuffer )
	{
		int timer;
		
		struct DHCP_HEADER * dhcpc;
		dhcpc = ( struct DHCP_HEADER *) DHCPbuffer;
			
		struct DHCP_CONFIG * DHCPConfig;
		DHCPConfig = ( struct DHCP_CONFIG *) Configbuffer;
			
		DHCP_buildheader( DHCPbuffer );
		
		DHCP_AddOption( Option_DHCP_MESSAGES_TYPE, DHCP_REQUEST, dhcpc->Options );
		DHCP_AddOption( Option_SERVER_ID, DHCPConfig->Server_IP.LONG , dhcpc->Options );
		DHCP_AddOption( Option_IP_ADRESS_LEASETIME, DHCPConfig->Leasetime.LONG , dhcpc->Options );
		DHCP_AddOption( Option_REQUESTET_IP_ADRESS, DHCPConfig->Client_IP.LONG , dhcpc->Options );
		DHCP_AddOption( Option_CLIENT_IDENTIFIER, 0, dhcpc->Options );
		DHCP_AddOption( Option_OPTIONLIST, 0, dhcpc->Options );

		UDP_SendPacket( SOCKET, DHCP_HEADER_LENGHT , DHCPbuffer);
		
		timer = CLOCK_RegisterCoundowntimer( );
		if( timer == CLOCK_FAILED ) return( DHCPC_TIMEOUT );
		CLOCK_SetCountdownTimer ( timer , DHCP_REQUEST_TIMEOUT, MSECOUND );

		while ( 1 )
		{
			if ( UDP_GetSocketState( SOCKET ) == UDP_SOCKET_BUSY )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				break;
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				CLOCK_ReleaseCountdownTimer( timer );
				UDP_CloseSocket( SOCKET );
				return( DHCPC_TIMEOUT );
			}
		}
		
		if( DHCP_PharseOption( DHCPConfig, dhcpc->Options ) != DHCP_ACK ) return( DHCPC_TIMEOUT );
		
		myIP = DHCPConfig->Client_IP.LONG;
		Netmask = DHCPConfig->Subnetmask.LONG;
		Gateway = DHCPConfig->Router_IP.LONG;
		DNSserver = DHCPConfig->DNS_IP.LONG;
		
		return( DHCPC_OK );
	}
	
/*!\brief Erstellt eine DHCP-Discover Nachricht und sendet sie, anschliessend wird auf die Bestaetigung der selben mit einen DHCP-Offer Nachricht gewartet.
 * Dann wird eine erste Auswertung der Optionfelder durchgefuehrt und in die DHCP_CONFIG struct eingetragen. War dieser Vorgang erfolgreich
 * wird ein DHCPC-OK als Returnwert zurueckgegeben, sonst DHCPC-TIMEOUT.
 * \param		SOCKET			Das UDP-Socket der benutzt wird zur kommunikation mit den DHCP-Server.
 * \param		DHCPbuffer		Zeiger auf den UDP-Buffer der zur kommunikation benutzt wird. Dieser enthälrt bei der Kommunikation den DHCP_HEADER.
 * \param 		Configbuffer	Zeiger auf den Configbuffer (DHCP_CONFIG) in der die Koniguration zwischengespeichert wird die der DHCP-Server vorschlaegt.
 * \return 		DHCPC_OK (Wert 0) oder DHCPC_TIMEOUT (ungleich 0)
 */
unsigned int DHCP_SendDiscover( unsigned int SOCKET, unsigned char * DHCPbuffer, unsigned char * Configbuffer )
	{
		int timer;
		
		struct DHCP_HEADER * dhcpc;
		dhcpc = ( struct DHCP_HEADER *) DHCPbuffer;
			
		struct DHCP_CONFIG * DHCPConfig;
		DHCPConfig = ( struct DHCP_CONFIG *) Configbuffer;
			
		DHCP_buildheader( DHCPbuffer );

		DHCP_AddOption( Option_DHCP_MESSAGES_TYPE, DHCP_DISCOVER, dhcpc->Options );
		DHCP_AddOption( Option_IP_ADRESS_LEASETIME, 0xfffff, dhcpc->Options );
		DHCP_AddOption( Option_CLIENT_IDENTIFIER, 0, dhcpc->Options );
		DHCP_AddOption( Option_OPTIONLIST, 0, dhcpc->Options );
		

		#ifdef _DEBUG_
			printf_P( PSTR("Dicover gesendet\r\n"));
		#endif

		UDP_SendPacket( SOCKET, DHCP_HEADER_LENGHT , DHCPbuffer);
		
		timer = CLOCK_RegisterCoundowntimer( );
		if( timer == CLOCK_FAILED ) return( DHCPC_TIMEOUT );
		CLOCK_SetCountdownTimer( timer , DHCP_REQUEST_TIMEOUT, MSECOUND );
		
		while ( 1 )
		{
			if ( UDP_GetSocketState( SOCKET ) == UDP_SOCKET_BUSY )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				break;
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				CLOCK_ReleaseCountdownTimer( timer );
				UDP_CloseSocket( SOCKET );
				return( DHCPC_TIMEOUT );
			}
		}

		if ( DHCP_PharseOption( DHCPConfig, dhcpc->Options ) != DHCP_OFFER ) return( DHCPC_TIMEOUT );
		
		DHCPConfig->Client_IP.LONG = dhcpc->Your_IP.LONG;
		
		UDP_FreeBuffer( SOCKET );
		
		return( DHCPC_OK );
		
	}

/*!\brief Fuegt einer DHCP-Nachricht im Optionfeld eine Option hinzu.
 * Mit dieser Funktion fuegt man dem Optionfeld eine Option hinzu. Dazu kann ein Parameter uebergeben werden der dann eingetragen wird.
 * \warning 	Nicht alle Optionen werden genutzt oder unterstuetzt!
 * \param		Option			Name oder Nummer der Option die eingetragen werden soll.
 * \param		ExtraData		Wert der in das Optionfeld mit eingetragen werden soll.
 * \param 		Optionfield		Zeiger auf das Optionfeld im DHCP_HEADER.
 * \return 		Anzahl der Byte die eingetragen wurden, wenn 0 zurueck gegeben wird wurde kein Eintrag vorgenommen
 */
unsigned int DHCP_AddOption( unsigned char Option, unsigned long ExtraData, unsigned char * Optionfield )
	{
		unsigned int i = 0 ;
		
		// suche Ende (0xff) vom Optionfeld mit letzten eintrag
		while( i < OPTION_LENGHT )
		{
			if ( Optionfield[i] == 0xff ) break;
			else 
			{
				i++; // um 1 erhöhen um die länge des Optioneintrages zu ermitteln
				i = i + Optionfield[i] + 1; //  i um ( länge + 1 ) des Optioneintrages erhöhen, sollte dann auf den nächsten Eintrag zeigen
			}
		}
			
		switch( Option )
		{
			case Option_DHCP_MESSAGES_TYPE:		Optionfield[ i++ ] = Option_DHCP_MESSAGES_TYPE;
												Optionfield[ i++ ] = 1;
												Optionfield[ i++ ] = ExtraData;
												break;

			case Option_IP_ADRESS_LEASETIME:	Optionfield[ i++ ] = Option_IP_ADRESS_LEASETIME;
												Optionfield[ i++ ] = 4;
												Optionfield[ i++ ] = ExtraData;
												Optionfield[ i++ ] = ExtraData>>8;
												Optionfield[ i++ ] = ExtraData>>16;
												Optionfield[ i++ ] = ExtraData>>24;
												break;
			
			case Option_SERVER_ID:				Optionfield[ i++ ] = Option_SERVER_ID;
												Optionfield[ i++ ] = 4;
												Optionfield[ i++ ] = ExtraData;
												Optionfield[ i++ ] = ExtraData>>8;
												Optionfield[ i++ ] = ExtraData>>16;
												Optionfield[ i++ ] = ExtraData>>24;
												break;
			
			case Option_REQUESTET_IP_ADRESS:	Optionfield[ i++ ] = Option_REQUESTET_IP_ADRESS;
												Optionfield[ i++ ] = 4;
												Optionfield[ i++ ] = ExtraData;
												Optionfield[ i++ ] = ExtraData>>8;
												Optionfield[ i++ ] = ExtraData>>16;
												Optionfield[ i++ ] = ExtraData>>24;
												break;

			case Option_OPTIONLIST:				Optionfield[ i++ ] = Option_OPTIONLIST;
												Optionfield[ i++ ] = 3;
												Optionfield[ i++ ] = Option_SUBNETMASK;
												Optionfield[ i++ ] = Option_ROUTER;
												Optionfield[ i++ ] = Option_DOMAIN_NAME_SRVER;
												break;
			case Option_CLIENT_IDENTIFIER:		Optionfield[ i++ ] = Option_CLIENT_IDENTIFIER;
												Optionfield[ i++ ] = 7;						
												Optionfield[ i++ ] = 1;						
												Optionfield[ i++ ] = mymac [0];					
												Optionfield[ i++ ] = mymac [1];					
												Optionfield[ i++ ] = mymac [2];					
												Optionfield[ i++ ] = mymac [3];					
												Optionfield[ i++ ] = mymac [4];					
												Optionfield[ i++ ] = mymac [5];
												break;
		}
		Optionfield[ i++ ] = 0xff;
		return( i );
	}
	
/*!\brief Ueberprueft das Optionfeld im DHCP_HEADER und Pharst es.
 * Mit dieser Funktion wird das Optionfeld im DHCP_HEADER ueberprueft und bearbeitet, Einstellungen die benoetigt werden werden in DHCP_CONFIG gespeichert.
 * \warning 	Nicht alle Optionen werden genutzt oder unterstuetzt!
 * \param		Configbuffer	Zeiger auf den DHCP-Cofigbuffer in der die Ausgehandelete Config steht.
 * \param 		Optionfield		Zeiger auf das Optionfeld im DHCP_HEADER.
 * \return 		Anzahl der Byte die eingetragen wurden, wenn 0 zurueck gegeben wird wurde kein Eintrag vorgenommen
 */
unsigned int DHCP_PharseOption( void * Configbuffer, unsigned char * Optionfield )
	{
		struct DHCP_CONFIG * DHCPConfig;
		DHCPConfig = ( struct DHCP_CONFIG *) Configbuffer;

		unsigned int OptionPointer = 0;
		unsigned char DHCP_Message_Type = 0;
		// suche Ende (0xff) vom Optionfeld mit letzten eintrag eintrag
		while( OptionPointer < OPTION_LENGHT )
		{
			if ( Optionfield[ OptionPointer ] == 0xff ) break;
			else 
			{
				switch ( Optionfield[ OptionPointer++ ] )
				{
					case Option_SUBNETMASK:						memcpy( &DHCPConfig->Subnetmask.LONG , &Optionfield[ OptionPointer + 1 ], 4);
																OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
																#ifdef _DEBUG_
																	printf_P( PSTR("SUBNETMASK gefunden %08lX\r\n"), DHCPConfig->Subnetmask.LONG );
																#endif
																break;

					case Option_ROUTER:							memcpy( &DHCPConfig->Router_IP.LONG , &Optionfield[ OptionPointer + 1 ], 4);
																OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
																#ifdef _DEBUG_
																	printf_P( PSTR("ROUTER gefunden %08lX\r\n"), DHCPConfig->Router_IP.LONG );
																#endif
																break;

					case Option_DOMAIN_NAME_SRVER:				memcpy( &DHCPConfig->DNS_IP.LONG , &Optionfield[ OptionPointer + 1 ], 4);
																OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
																#ifdef _DEBUG_
																	printf_P( PSTR("DNS-Server gefunden %08lX\r\n"), DHCPConfig->DNS_IP.LONG );
																#endif
																break;

					case Option_IP_ADRESS_LEASETIME:			memcpy( &DHCPConfig->Leasetime.LONG , &Optionfield[ OptionPointer + 1 ], 4);
																OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
																#ifdef _DEBUG_
																	printf_P( PSTR("Leasetime gefunden %08lX\r\n"), DHCPConfig->Leasetime.LONG );
																#endif
																break;

					case Option_SERVER_ID:						memcpy( &DHCPConfig->Server_IP.LONG , &Optionfield[ OptionPointer + 1 ], 4);
																OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
																#ifdef _DEBUG_
																	printf_P( PSTR("Server-ID gefunden %08lX\r\n"), DHCPConfig->Server_IP.LONG );
																#endif
																break;

					case Option_DHCP_MESSAGES_TYPE:				DHCP_Message_Type = Optionfield[ OptionPointer + 1 ];
																OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
																#ifdef _DEBUG_
																	printf_P( PSTR("DHCP_MESSAGES_TYPE gefunden %02X\r\n"), DHCP_Message_Type );
																#endif
																break; 

					default:									OptionPointer = OptionPointer + Optionfield[ OptionPointer++ ];
				}
			}
		}
		return( DHCP_Message_Type );
	}
	
/*!\brief Baut den DHCP_HEADER
 * Baut den DHCP_HEADER und stellt Standartwerte ein.
 * \param		DHCPbuffer		Zeiger auf den Buffer in den der DHCP_HEADER erzeugt werden soll.
 * \return 		NONE
 */
void DHCP_buildheader( unsigned char * DHCPbuffer )
{
		unsigned int i;

		struct DHCP_HEADER * dhcpc;
		dhcpc = ( struct DHCP_HEADER *) DHCPbuffer;
			
		dhcpc->Messages_Type = BOOT_REQUEST;
		dhcpc->Hardware_Type = HARDWARE_TYPE_ETHERNET;
		dhcpc->Hardware_Adress_Lenght = 6;
		dhcpc->Hops = 0;
		dhcpc->Transaction_ID = 0x12345678;
		dhcpc->Second_elapsed = 0;
		dhcpc->Bootp_Flags = 0;
		dhcpc->Client_IP.LONG = 0;
		dhcpc->Your_IP.LONG = 0;
		dhcpc->Server_IP.LONG = 0;
		dhcpc->Relay_IP.LONG = 0;
		for ( i = 0 ; i < 6 ; i++ ) dhcpc->Your_MACADRESS[i] = mymac[i];
		for ( i = 0 ; i < SERVERNAME_LENGHT ; i++ ) dhcpc->Servername[i] = 0 ;
		for ( i = 0 ; i < BOOT_FILENAME_LENGHT ; i++ ) dhcpc->Boot_Filename[i] = 0;
		dhcpc->Magic_Cookie = MAGIC_COOKIE;
		for ( i = 0 ; i < OPTION_LENGHT ; i++ ) dhcpc->Options[i] = 0;	
		dhcpc->Options[0] = 0xff;
}
//@}

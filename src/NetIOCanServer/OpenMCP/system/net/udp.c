/*!\file udp.c \brief Stellt die UDP Funktionalitaet bereit*/
//***************************************************************************
//*            udp.c
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
///	\ingroup network
///	\defgroup UDP Der UDP Stack fuer Mikrocontroller (udp.c)
///	\code #include "arp.h" \endcode
///	\code #include "ethernet.h" \endcode
///	\code #include "ip.h" \endcode
///	\code #include "udp.h" \endcode
///	\par Uebersicht
///		Der UDP-Stack für Mikrocontroller. Behandelt komplett den UDP-Stack
/// mit Verbindungsaufbau und Abbau.
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
#include <stdio.h>
#include <avr/pgmspace.h>
#include "system/math/math.h"
#include "arp.h"
#include "ethernet.h"
#include "ip.h"
#include "udp.h"
#include "apps/can_relay/can_relay.h"

struct UDP_SOCKET UDP_sockettable[ MAX_UDP_CONNECTIONS ];
struct UDP_SOCKET * UDP_socket;
extern char CAN_SendBuffer[CAN_RELAY_BUFFER_LEN] ;
extern int CAN_Socket ;
extern unsigned long myBroadcast ;


/* -----------------------------------------------------------------------------------------------------------*/
/*! Hier findet die Initialisierung des UDP-Stack statt um einen definiertenausgangzustand zu haben.
 * \retval  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void UDP_init( void )
{
	unsigned int i;
	
	for( i = 0 ; i < MAX_UDP_CONNECTIONS ; i++ )
		UDP_CloseSocket( i );
}
	
/* -----------------------------------------------------------------------------------------------------------*/
/*! Hier findet die Bearbeitung des Packetes statt welches ein UDP-Packet enthaelt. Es wird versucht, die 
 * Verbindung zuzuordnen, wenn dies nicht moeglich ist, wird hier abgebrochen.
 * Danach wird der Inhalt dem Socket zugeordnet und Daten in den Puffer des Benutzers kopiert.
 * \warning Zu lange UDP-Packete werden abgeschnitten.
 * \param 	packet_lenght	Gibt die Packetgroesse in Byte an die das Packet lang ist.
 * \param	ethernetbuffer	Zeiger auf das Packet.
 * \return  NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void udp( unsigned int packet_lenght, unsigned char * ethernetbuffer )
{
	
		int i, SOCKET , Offset;
	
		struct ETH_header * ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header * IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct UDP_header * UDP_packet;		// TCP_struct anlegen
		UDP_packet = ( struct UDP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];
			
		#ifdef _DEBUG_
			printf_P( PSTR("UDP-Packet empfangen") , packet_lenght );
		#endif

		SOCKET = UDP_GetSocket( ethernetbuffer );
	
		// Socket zugeordnet ?
		if ( SOCKET == UDP_SOCKET_ERROR ) return;
			
		// Socket schon mit daten belegt ?
		if ( UDP_sockettable[ SOCKET ].Socketstate == UDP_SOCKET_BUSY ) return ;
		// Hat Socket Daten ?
		if ( ( ChangeEndian16bit( UDP_packet->UDP_Datalenght ) - UDP_HEADER_LENGHT ) == 0 ) return ;
		// Größe der Daten limitieren auf Puffergröße
		i = MIN( ChangeEndian16bit( UDP_packet->UDP_Datalenght) - UDP_HEADER_LENGHT , UDP_sockettable[ SOCKET ].Bufferlenght );
		
		// Größe der Daten eintragen
		UDP_sockettable[ SOCKET ].Bufferfill = i;
		// Socket als belegt melden
		UDP_sockettable[ SOCKET ].Socketstate = UDP_SOCKET_BUSY;
		// ttl wieder richtig setzen
		UDP_sockettable[SOCKET].ttl = UDP_Default_ttl;
		
		// Offset für UDP-Daten im Ethernetfrane berechnen
		Offset = ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 ) + UDP_HEADER_LENGHT;
		
		// Daten kopieren
		while ( i-- )
			{
				UDP_sockettable[ SOCKET ].Recivebuffer[i] = ethernetbuffer[ Offset + i ];
			}
		return;
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert ein Socket in den die Daten fuer ein Verbindung gehalten werden um die ausgehenden und einghenden UDP-Packet zuzuordnen.
 * \param 	IP					Die IP-Adresse des Zielhost.
 * \param	DestinationPort		Der Zielpot des Zielhost mit den verbunden werden soll. Der Sourcport wird automatisch eingestellt. Zu beachten ist das bei einer Verbindn zu Port 67 der Sourceport auf 68 eingestellt wird.
 * \param	Bufferlenght		Groesse des Datenpuffer der vom Benutzer bereitgestellt wird. Hier werden die eingegenden UDP-Daten kopiert. Dieser Puffer sollte entsprechend der Verwendung dimensioniert sein.
 * \param	UDP_Recivebuffer	Zieger auf den Puffer der vom Benutzer bereitgestellt wird.
 * \return  Beim erfolgreichen anlegen eines Socket wird die Socketnummer zurueck gegeben. Im Fehlerfall 0xffff.
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_RegisterSocket( unsigned long IP, unsigned int DestinationPort, unsigned int Bufferlenght, unsigned char * UDP_Recivebuffer)
{
	int SOCKET;
	
	SOCKET = UDP_Getfreesocket();
	
	if ( SOCKET == UDP_SOCKET_ERROR ) return( UDP_SOCKET_ERROR );
	
	UDP_sockettable[SOCKET].Socketstate = UDP_SOCKET_READY;
	UDP_sockettable[SOCKET].DestinationPort = ChangeEndian16bit( DestinationPort );
	// wenn Zielport Bootps(67) dann Sourceport auf Bootpc(68) setzen um kommunikation mit DHCP-Server zu ermöglichen
	if ( DestinationPort == 67 ) {
		UDP_sockettable[SOCKET].SourcePort = ChangeEndian16bit( 68 );
	} else if (DestinationPort  == CAN_RELAY_PORT) {
		// CAN UDP werden auf dem gleichen Socket empfangen wie gesendet.
		UDP_sockettable[SOCKET].SourcePort = ChangeEndian16bit( CAN_RELAY_PORT );
	} else {
		UDP_sockettable[SOCKET].SourcePort =~ DestinationPort;
	} ;
	
	UDP_sockettable[SOCKET].DestinationIP = IP;
	UDP_sockettable[SOCKET].Bufferfill = 0;
	UDP_sockettable[SOCKET].Bufferlenght = Bufferlenght;
	UDP_sockettable[SOCKET].Recivebuffer = UDP_Recivebuffer;
	UDP_sockettable[SOCKET].ttl = UDP_Default_ttl;
		
	if ( IP == 0xffffffff ) 
	{
		for( unsigned char i = 0 ; i < 6 ; i++ ) UDP_sockettable[SOCKET].MACadress[i] = 0xff;
		return( SOCKET );
	}
	if ( IS_ADDR_IN_MY_SUBNET( IP, Netmask ) )
		if ( IS_BROADCAST_ADDR( IP, Netmask ) ) for( unsigned char i = 0 ; i < 6 ; i++ ) UDP_sockettable[SOCKET].MACadress[i] = 0xff;
		else GetIP2MAC( IP, UDP_sockettable[SOCKET].MACadress );
	else GetIP2MAC( Gateway, UDP_sockettable[SOCKET].MACadress );

	return( SOCKET );
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert ein ListenSocket der auf einen bestimmten Port lauschen tut.
 * \param	Port				Der Port auf den gelauscht werden soll.
 * \param	Bufferlenght		Groesse des Datenpuffer der vom Benutzer bereitgestellt wird. Hier werden die eingegenden UDP-Daten kopiert. Dieser Puffer sollte entsprechend der Verwendung dimensioniert sein.
 * \param	UDP_Recivebuffer	Zieger auf den Puffer der vom Benutzer bereitgestellt wird.
 * \return  Beim erfolgreichen anlegen eines ListenSocket wird die Socketnummer zurueck gegeben. Im Fehlerfall 0xffff.
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_ListenOnPort( unsigned int Port, unsigned int Bufferlenght, unsigned char * UDP_Recivebuffer)
{
	unsigned int SOCKET;
	
	SOCKET = UDP_Getfreesocket();
	
	if ( SOCKET == UDP_SOCKET_ERROR ) return( UDP_SOCKET_ERROR );
	
	UDP_sockettable[SOCKET].Socketstate = UDP_SOCKET_READY;
	UDP_sockettable[SOCKET].DestinationPort = 0 ;
	UDP_sockettable[SOCKET].SourcePort = ChangeEndian16bit( Port );
	UDP_sockettable[SOCKET].DestinationIP = 0 ;
	UDP_sockettable[SOCKET].Bufferfill = 0;
	UDP_sockettable[SOCKET].Bufferlenght = Bufferlenght;
	UDP_sockettable[SOCKET].Recivebuffer = UDP_Recivebuffer;
	
	return(SOCKET);
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet ein UDP-Packet an einen Host.
 * \code
 * // ein Simples Beispiel für das Senden einen UDP-Packetes
 * #define Bufferlen 100
 * #define DestinationPort 53
 * #define IP 0x12345678
 *
 * unsigned char buffer[ Bufferlen ];
 * unsigned int Socket;
 *
 * // Socket Registrieren
 * Socket = UDP_RegisterSocket( IP, DestinationPort, Bufferlen, buffer );
 * // Socket erfolgreich geoeffnet ?
 * if ( Socket == 0xffff ) 
 *           return( error );
 * // UDP-Packet senden
 * UDP_SendPacket( Socket , Bufferlen, buffer );
 * // Socket schliessen
 * UDP_CloseSocket( Socket );
 * \endcode
 * \param 	SOCKET			Die Socketnummer ueber die das Packet gesendet wird.
 * \param	Datalenght		Gibt die Datenlaenge der Daten in Byte an die gesendet werden sollen.
 * \param	UDP_Databuffer  Zeifer auf den Datenpuffer der gesendet werden soll.
 * \return  Bei einem Fehler beim versenden wird ungleich 0 zurueckgegeben, sonst 0.
 * \sa UDP_RegisterSocket , UDP_GetSocketState
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_SendPacket( int SOCKET, unsigned int Datalenght, unsigned char * UDP_Databuffer )
{
	if ( SOCKET >= MAX_UDP_CONNECTIONS ) return( UDP_SOCKET_ERROR );
		
	LockEthernet();
	
	unsigned char * ethernetbuffer;
	ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + UDP_HEADER_LENGHT + Datalenght );

	struct ETH_header * ETH_packet; 		// ETH_struct anlegen
	ETH_packet = (struct ETH_header *) ethernetbuffer;
	struct IP_header * IP_packet;		// IP_struct anlegen
	IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
	IP_packet->IP_Version_Headerlen = 0x45; // Standart IPv4 und Headerlenght 20byte
	struct UDP_header * UDP_packet;		// TCP_struct anlegen
	UDP_packet = ( struct UDP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];
		
	unsigned int i;
	unsigned int offset = ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 ) + UDP_HEADER_LENGHT;

	for ( i = 0 ; i < Datalenght ; i++ ) ethernetbuffer[ offset + i ] = UDP_Databuffer[ i ];
	
	MakeIPheader( UDP_sockettable[SOCKET].DestinationIP, PROTO_UDP, UDP_HEADER_LENGHT + Datalenght , ethernetbuffer );
	MakeUDPheader( SOCKET, Datalenght, ethernetbuffer );
	MakeETHheader( UDP_sockettable[SOCKET].MACadress, ethernetbuffer );
	sendEthernetframe( ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + UDP_HEADER_LENGHT + Datalenght, ethernetbuffer);

	FreeEthernet();
	
	return(0);
}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt den Socketstatus aus.
 * \param	SOCKET	Die Socketnummer vom abzufragen Socket.
 * \return  Den Socketstatus.
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_GetSocketState( int SOCKET )
	{
		if ( SOCKET >= MAX_UDP_CONNECTIONS ) return( UDP_SOCKET_ERROR );
		return( UDP_sockettable[ SOCKET ].Socketstate );
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt die Anzahl der Byte aus die sich im Puffer befinden. Diese Abfrage macht nur sinn in Verbindung mit
 * UDP_GetSocketState nachdem ein UDP-Packet empfangen worden ist und der Status fuer das auf UDP_SOCKET_BUSY steht.
 * Danach werden bis zur Freigabe durch UDP_FreeBuffer keine Daten auf den Socket mehr angenommen
 * \param	SOCKET		Die Socketnummer vom abzufragen Socket.
 * \return  Anzahl der Byte im Puffer.
 *\sa UDP_GetSocketState, UDP_FreeBuffer
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_GetByteInBuffer( int SOCKET )
	{
		if ( SOCKET >= MAX_UDP_CONNECTIONS ) return( UDP_SOCKET_ERROR );
		return ( UDP_sockettable[ SOCKET ].Bufferfill );
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt den UDP-Puffer wieder zum empfang frei. Danach werden wieder UDP-Daten angenommen und in den Puffer kopiert.
 * \param	SOCKET		Die Socketnummer die freigegeben werden soll.
 * \return	NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_FreeBuffer( int SOCKET )
	{
		if ( SOCKET >= MAX_UDP_CONNECTIONS ) return( UDP_SOCKET_ERROR );

		UDP_sockettable[ SOCKET ].Bufferfill = 0;

		UDP_sockettable[ SOCKET ].Socketstate = UDP_SOCKET_READY;
		return( 0 );
	}
	
/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt das Socket wieder freu und beendet die Verbindung. Alle UDP-Packet die dann von diesen Socket empfangen werden, werden verworfen.
 * \param	SOCKET		Die Socketnummer die geschlossen werden soll.
 * \return	Es wird beim erfolgreichen schliessen der Socket 0 zurueck gegeben, sonst 0xffff.
 */
/* -----------------------------------------------------------------------------------------------------------*/
int UDP_CloseSocket( int SOCKET )
	{
		if ( SOCKET >= MAX_UDP_CONNECTIONS ) return( UDP_SOCKET_ERROR );
			
		if ( UDP_sockettable[SOCKET].Socketstate == UDP_SOCKET_NOT_USE ) return( UDP_SOCKET_ERROR );
			
		UDP_sockettable[SOCKET].Socketstate = UDP_SOCKET_NOT_USE;
		
		return( 0 );
	}

/* -----------------------------------------------------------------------------------------------------------
wählt den richtigen Socketeintrag setzt TCP_socket auf diesen eintrag
------------------------------------------------------------------------------------------------------------*/
int UDP_GetSocket( unsigned char * ethernetbuffer )
	{
		struct ETH_header * ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header * IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct UDP_header * UDP_packet;		// TCP_struct anlegen
		UDP_packet = ( struct UDP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		unsigned int Socket,i;
		
		for ( Socket = 0 ; Socket < MAX_UDP_CONNECTIONS ; Socket++ ) 
			{
/*				CAN_SendBuffer[0] = Socket&0xff ;
				CAN_SendBuffer[1] = Socket>>8 ;
				CAN_SendBuffer[2] = UDP_packet->UDP_SourcePort ;
				CAN_SendBuffer[3] = UDP_sockettable[Socket].DestinationPort ;
				CAN_SendBuffer[4] = UDP_packet->UDP_DestinationPort ;
				CAN_SendBuffer[5] = UDP_sockettable[Socket].SourcePort ;
		
				UDP_SendPacket (CAN_Socket,10,CAN_SendBuffer) ; */

				//Normal zuordnung
				if ( UDP_sockettable[ Socket ].DestinationPort == UDP_packet->UDP_SourcePort
						&& UDP_sockettable[ Socket ].SourcePort == UDP_packet->UDP_DestinationPort
						&& UDP_sockettable[ Socket ].DestinationIP == IP_packet->IP_SourceIP ) return( Socket );
				// BootP sonderstellung einräumen
				if ( UDP_sockettable[ Socket ].DestinationPort == UDP_packet->UDP_SourcePort
					 	&& UDP_sockettable[ Socket ].SourcePort == UDP_packet->UDP_DestinationPort ) return( Socket );
				// Sonderstellung auf Broadcast
				if ( IP_packet->IP_DestinationIP == myBroadcast
					 	&& UDP_sockettable[ Socket ].SourcePort == UDP_packet->UDP_DestinationPort ) return( Socket );		
				// Ist Packet auf einen Listen-socket eingetroffen ?	
				if ( UDP_sockettable[ Socket ].SourcePort == UDP_packet->UDP_DestinationPort
						&& UDP_sockettable[ Socket ].DestinationPort == 0
						&& UDP_sockettable[ Socket ].DestinationIP == 0 )
							{
								// Socket komplettieren
								UDP_sockettable[ Socket ].DestinationIP = IP_packet->IP_SourceIP;
								UDP_sockettable[ Socket ].DestinationPort = UDP_packet->UDP_SourcePort;
								for( i = 0 ; i < 6 ; i++ ) UDP_sockettable[ Socket ].MACadress[ i ] = ETH_packet->ETH_sourceMac[ i ] ;
								return( Socket );
							}
			}

		return( UDP_SOCKET_ERROR );
	}
	
/* -----------------------------------------------------------------------------------------------------------
Bastelt den UDP-header
------------------------------------------------------------------------------------------------------------*/
int MakeUDPheader( int SOCKET, unsigned int Datalenght, unsigned char * ethernetbuffer )
{
	if ( SOCKET >= MAX_UDP_CONNECTIONS ) return( UDP_SOCKET_ERROR );
	
	struct ETH_header * ETH_packet; 		// ETH_struct anlegen
	ETH_packet = (struct ETH_header *) ethernetbuffer;
	struct IP_header * IP_packet;		// IP_struct anlegen
	IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
	struct UDP_header * UDP_packet;		// TCP_struct anlegen
	UDP_packet = ( struct UDP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];		
		
	UDP_packet->UDP_DestinationPort = UDP_sockettable[SOCKET].DestinationPort;
	UDP_packet->UDP_SourcePort = UDP_sockettable[SOCKET].SourcePort;
	UDP_packet->UDP_Checksum = 0;
	UDP_packet->UDP_Datalenght = ChangeEndian16bit( 8 + Datalenght );
	
	UDP_sockettable[SOCKET].ttl = UDP_Default_ttl;
	
	return(0);
}

/* -----------------------------------------------------------------------------------------------------------
Holt den nächsten freien Socket
------------------------------------------------------------------------------------------------------------*/
int UDP_Getfreesocket( void )
	{
		unsigned int Socket;
		for ( Socket = 0 ; Socket < MAX_UDP_CONNECTIONS ; Socket++ ) 
			{
				if ( UDP_sockettable[ Socket ].Socketstate == UDP_SOCKET_NOT_USE ) return( Socket );
			}
		return( UDP_SOCKET_ERROR );
	}
//@}

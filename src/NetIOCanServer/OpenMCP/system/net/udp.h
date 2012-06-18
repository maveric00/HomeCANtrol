/*!\file udp.h \brief Definitionen fuer UDP */
//***************************************************************************
//*            udp.h
//*
//*  Mon Jul 31 21:47:03 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//*
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
#ifndef _UDP_H
	#define _UDP_H
	
	/* -----------------------------------------------------------------------------------------------------------*/
	/*! Hier findet die Initialisierung des UDP-Stack statt um einen definiertenausgangzustand zu haben.
	 * \param 	NONE
	 * \param	NONE
	 * \return  NONE
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	void UDP_init( void );
	/* -----------------------------------------------------------------------------------------------------------*/
	/*! Hier findet die Bearbeitung des Packetes statt welches ein UDP-Packet enthaelt. Es wir versucht die 
	 * Verbindung zuzuordnen, wenn dies nicht moeglich ist wird hir abgebrochen.
	 * Danach wird der Inhalt dem Socket zugeordnet und Daten in den Puffer des Benutzer kopiert.
	 * \warning Zu lange UDP-Packete werden abgeschnitten.
	 * \param 	packet_lenght	Gibt die Packetgroesse in Byte an die das Packet lang ist.
	 * \param	ethernetbuffer	Zeiger auf das Packet.
	 * \return  NONE
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	void udp( unsigned int packet_lenght, unsigned char * ethernetbuffer );
	
	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Sendet ein UDP-Packet an einen Host.
	 * \param 	SOCKET			Die Socketnummer ueber die das Packet gesendet wird.
	 * param	Datalenght		Gibt die Datenlaenge der Daten in Byte an die gesendet werden sollen.
	 * \param	UDP_Databuffer  Zeifer auf den Datenpuffer der gesendet werden soll.
	 * \return  Bei einem Fehler beim versenden wird ungleich 0 zurueckgegeben, sonst 0.
	 * \sa UDP_RegisterSocket , UDP_GetSocketState
	 */	
	/* -----------------------------------------------------------------------------------------------------------*/
	int UDP_SendPacket( int SOCKET, unsigned int Datalenght, unsigned char * UDP_Databuffer );

	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Reistriert ein Socket in den die Daten fuer ein Verbindung gehalten werden um die ausgehenden und einghenden UDP-Packet zuzuordnen.
	 * \param 	IP					Die IP-Adresse des Zielhost.
	 * \param	DestinationPort		Der Zielpot des Zielhost mit den verbunden werden soll. Der Sourcport wird automatisch eingestellt. Zu beachten ist das bei einer Verbindn zu Port 67 der Sourceport auf 68 eingestellt wird.
	 * \param	Bufferlenght		Groesse des Datenpuffer der vom Benutzer bereitgestellt wird. Hier werden die eingegenden UDP-Daten kopiert. Dieser Puffer sollte entsprechend der Verwendung dimensioniert sein.
	 * \param	UDP_Recivebuffer	Zieger auf den Puffer der vom Benutzer bereitgestellt wird.
	 * \return  Beim erfolgreichen anlegen eines Socket wird die Socketnummer zurueck gegeben. Im Fehlerfall 0xffff.
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	int UDP_RegisterSocket( unsigned long IP, unsigned int DestinationPort, unsigned int Bufferlenght, unsigned char * UDP_Recivebuffer);

	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Reistriert ein Socket in den die Daten fuer ein Verbindung gehalten werden um die ausgehenden und einghenden UDP-Packet zuzuordnen.
	 * \param 	IP					Die IP-Adresse des Zielhost.
	 * \param	DestinationPort		Der Zielpot des Zielhost mit den verbunden werden soll. Der Sourcport wird automatisch eingestellt. Zu beachten ist das bei einer Verbindn zu Port 67 der Sourceport auf 68 eingestellt wird.
	 * \param	Bufferlenght		Groesse des Datenpuffer der vom Benutzer bereitgestellt wird. Hier werden die eingegenden UDP-Daten kopiert. Dieser Puffer sollte entsprechend der Verwendung dimensioniert sein.
	 * \param	UDP_Recivebuffer	Zieger auf den Puffer der vom Benutzer bereitgestellt wird.
	 * \return  Beim erfolgreichen anlegen eines Socket wird die Socketnummer zurueck gegeben. Im Fehlerfall 0xffff.
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	int UDP_ListenOnPort( unsigned int Port, unsigned int Bufferlenght, unsigned char * UDP_Recivebuffer);
	
	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Gibt den Socketstatus aus.
	 * \param	SOCKET	Die Socketnummer vom abzufragen Socket.
	 * \return  Den Socketstatus.
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	int UDP_GetSocketState( int SOCKET );
	
	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Gibt die Anzahl der Byte aus die sich im Puffer befinden. Diese Abfrage macht nur sinn in Verbindung mit
	 * UDP_GetSocketState nachdem ein UDP-Packet empfangen worden ist und der Status fuer das auf SOCKET_BUSY steht.
	 * Danach werden bis zur Freigabe durch UDP_FreeBuffer keine Daten auf den Socket mehr angenommen
	 * \param	SOCKET		Die Socketnummer vom abzufragen Socket.
	 * \return  Anzahl der Byte im Puffer.
	 *\sa UDP_GetSocketState, UDP_FreeBuffer
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	int UDP_GetByteInBuffer( int SOCKET );

	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Gibt den UDP-Puffer wieder zum empfang frei. Danach werden wieder UDP-Daten angenommen und in den Puffer kopiert.
	 * \param	SOCKET		Die Socketnummer die freigegeben werden soll.
	 * \return	NONE
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	 int UDP_FreeBuffer( int SOCKET );
	
	/* -----------------------------------------------------------------------------------------------------------*/
	/*!\brief Gibt das Socket wieder freu und beendet die Verbindung. Alle UDP-Packet die dann von diesen Socket empfangen werden, werden verworfen.
	 * \param	SOCKET		Die Socketnummer die geschlossen werden soll.
	 * \return	Es wird beim erfolgreichen schliessen der Socket 0 zurueck gegeben, sonst 0xffff.
	 */
	/* -----------------------------------------------------------------------------------------------------------*/
	int UDP_CloseSocket( int SOCKET );
	
	int UDP_Getfreesocket( void );
	int UDP_GetSocket( unsigned char * ethernetbuffer );
	int MakeUDPheader( int SOCKET, unsigned int Datalenght, unsigned char * ethernetbuffer );

	#ifdef __AVR_ATmega2561__
		#define MAX_UDP_CONNECTIONS 2
	#endif
	#ifdef __AVR_ATmega644__
		#define MAX_UDP_CONNECTIONS 2
	#endif
	#ifdef __AVR_ATmega644P__
		#define MAX_UDP_CONNECTIONS 2
	#endif

	#define UDP_HEADER_LENGHT 8
	
	/*! \struct UDP_SOCKET
	 * \brief Definiert den UDP_SOCKET Aufbau. Hier sind alle wichtigen Infomationen enthalten um die Verbindung
	 * zum halten und zuzuordnen
	 */
	struct UDP_SOCKET {
		unsigned char   Socketstate;		
		unsigned long   DestinationIP;
		unsigned int	SourcePort;
		unsigned int	DestinationPort;
		unsigned char   MACadress[6];
		unsigned int	Bufferlenght;
		unsigned int	Bufferfill;
		unsigned char * Recivebuffer;
		unsigned char   ttl;
	};
	
	#define	UDP_Default_ttl		30

	#define UDP_SOCKET_NOT_USE		0x00			// SOCKET ist Frei
	#define UDP_SOCKET_READY		0x10			// Socket ist Bereit zur Benutzung
	#define UDP_SOCKET_BUSY			0x20			// SOCKET belegt ...
	#define UDP_SOCKET_ERROR		-1
	
	/*!\struct UDP_header
	 * \brief Definiert den UDP_header.
	 */
	struct UDP_header {
		unsigned int UDP_SourcePort;
		unsigned int UDP_DestinationPort;
		unsigned int UDP_Datalenght;
		unsigned int UDP_Checksum;
	};

#endif /* _UDP_H */

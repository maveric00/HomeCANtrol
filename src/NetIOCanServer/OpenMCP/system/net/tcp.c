/*! \file tcp.c \brief Stellt die TCP/IP Funkionalitaet bereit */
//***************************************************************************
//*            tcp.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup network
///	\defgroup TCP Der TCP Stack fuer Mikrocontroller (tcp.c)
///	\code #include "tcp.h" \endcode
///	\par Uebersicht
///		Der TCP-Stack fuer Mikrocontroller. Behandelt komplett den TCP-Stack
/// mit Verbindungsaufbau, Abbau und halten. Es werden Ereignisse wie bei Timeouts
/// oder Retransmisions behandelt.	
/// \date	04-15-2008: Umstellung auf FIFO-Puffer erfolgt.
/// \date	05-14-2008: Man findet ja immer noch was zum basteln. Die CopyTCPdata2socketbuffer() verbessert,
///			kopieren des Buffer hat im Interrupt zu lange gedauert, so das die Uhr nach ging. Sollte eigentlich
///			nicht vorkommen, aber wenn man jedes Byte einzeln in den FIFO kopiert dauerts halt. Jetzt wird die optimierte
///			BlockToFIFO benutzt und für den Vorgang der Interrupt von Ethernet gesperrt und im
///			Interrupt alle anderen Interrupts Freigegeben damit nicht alles kurz hängen bleibt.
/// \date	05-15-2008: TCP so erweitert das jetzt auch TCP-Packete in der Falschen Reihenfolge
///			eintreffen können, dazu wird ein Puffer (TCP_UNSORT) verwendet der das Packet zwischenspeichert
///			mit Socketnummer und Seq.nummer und beim nächsten empfang eines TCP-Packetes
///			auf gültigkeit überprüft wird ob es dem gerade empfangendem folgen sollte.
/// \date	05-25-2008: Irgend wie kommt es ab und zu zum deathlock des TCP-Stack, mal fehler finden.
/// \date   02-03-3009: So, Fehler mit den Deathlock anscheind beseitigt. Jetzt funktioniert auch der Ordnungsgemäße abbau der
///			TCP-Verbingung, so das die Sockets nicht ins leere laufen oder blockieren.
/// \date   05-23-2009: DELAYED_ACK hinzugefügt, damit Windows damit klar kommt. Immer diese extra Wurst von Windows.
/// \date   05-28-2009: So, ein bisschen Namenschaos beseitigt in den defines.
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

#include "system/math/math.h"
#include "system/math/checksum.h"
#include "system/clock/clock.h"
#include "system/buffer/fifo.h"

#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "tcp.h"

#include "config.h"

// #define _DEBUG_

#ifdef _DEBUG_
	#include "hardware/uart/uart.h"
#endif

unsigned int TXErrorCounter = 0 ;
unsigned int RXErrorCounter = 0 ;

struct TCP_SOCKET TCP_sockettable[MAX_TCP_CONNECTIONS];

struct TCP_PORT TCP_porttable[MAX_LISTEN_PORTS];

unsigned int RXErrorOldSeq = 0;

#ifdef TCP_with_unsortseq
	struct TCP_UNSORT TCP_Unsort;
	unsigned int RXErrorUnsort = 0;
#endif

char TCP_LOCK = Unlock;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Hier wird der TCP Initialisiert.
 * Hier wird der TCP-Timeouthandler Registriert beim der Clock. Danach wird die Funktion alle 1000ms aufgerufen
 * und alle Offenen kontrolliert.
 * \return		void
 */
/*------------------------------------------------------------------------------------------------------------*/
void tcp_init( void )
{
	unsigned int i;
	
	// CAllback für Timeout registrieren
	CLOCK_RegisterCallbackFunction( TCPTimeOutHandler, SECOUND );
	
	#ifdef TCP_with_unsortseq
		TCP_Unsort.lenght = 0 ;
	#endif
	
	// FIFOs für den Recivebuffer anlegen
	// und den state festlegen
	for ( i = 0 ; i < MAX_TCP_CONNECTIONS ; i++ )
	{
		TCP_sockettable[ i ].fifo = Get_FIFO( TCP_sockettable[ i ].Recivebuffer, MAX_RECIVEBUFFER_LENGHT );
		TCP_sockettable[ i ].Timeoutcounter = 0 ;
		TCP_sockettable[ i ].ConnectionState = SOCKET_NOT_USE;
	}
}
	
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die TCP-Funktion die aufgerufen wird wenn ein Packet eintrifft.
 * Hier findet die Bearbeitung der eintreffenden Packete statt. Das Packet wird einer Verbindung zugeordnet 
 * oder einen offenen Port wenn die in die TCP_PORT Liste eingetragen ist.
 * Danach wird es je nach Flag bearbeitet.
 * \param 		packet_lenght	Groesse des Packetes.
 * \param 		ethernetbuffer  Zeiger auf den Ethernetbuffer, dieser enthaelt noch die kompletten Header aller Layer.
 * \retval		void
 */
/*------------------------------------------------------------------------------------------------------------*/
void tcp( unsigned int packet_lenght, unsigned char * ethernetbuffer)
	{
		unsigned long i;
		unsigned int Socket;
		char sreg_tmp;
		
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];
		
		Socket = GetSocket ( ethernetbuffer );

		TCP_packet->TCP_ControllFlags &= ( TCP_SYN_FLAG | TCP_ACK_FLAG | TCP_FIN_FLAG );

		if ( Socket == SOCKET_ERROR )
		{
			if ( ( CheckPortInList( ChangeEndian16bit ( TCP_packet->TCP_DestinationPort ) ) == SOCKET_ERROR ) ) 
			{
				return;
			}
		}
		
		if ( TCP_packet->TCP_ControllFlags == TCP_SYN_FLAG ) // Packet mit SYN ?
			{
				Socket = RegisterSocket( ethernetbuffer ) ;
				if ( Socket != SOCKET_ERROR ) // Verbinung einem neuem Socket zuordnen
				{
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_SYNINIT ; // SYNINIT setzen so lange wie das packet noch nicht beantwortet wurde, wird in MakeTCP gebraucht um de MSS zu senden
					TCP_sockettable[ Socket ].Windowsize = TCP_packet->TCP_Window; // Windowsize setzen, wird gebraucht um zu wissen wiviel gesendet werden kann ohne ACK
					TCP_sockettable[ Socket ].AcknowledgeNumber++; // SequenceNumber um 1 erhöhen, das gehört zur SYN-sequence dazu
					MakeTCPheader( Socket, TCP_SYN_FLAG | TCP_ACK_FLAG, 0 , MAX_RECIVEBUFFER_LENGHT , ethernetbuffer ); // Baue mal den TCP-Header mit Berechnung des Pseudoheader
					TCP_sockettable[ Socket ].SequenceNumber++; // SequenceNumber um 1 erhöhen, das gehört zur SYN-sequence dazu
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2SYNACK ; // State für den Socket auf WAIT2SYNACK und den den SYN abschließen zu können
					TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND ; // bereit zum senden
					TCP_sockettable[ Socket ].Timeoutcounter = 2;
				}
				return;
			}
		
		// Wenn immer noch keine Verbindung zugeordnet beenden
		if ( Socket == SOCKET_ERROR ) return;
		// den Timeoutcounter wieder zuruecksetzen für die Verbindung auf den zugeordneten Socket
		TCP_sockettable[ Socket ].Timeoutcounter = TimeOutCounter;
				
		if ( TCP_packet->TCP_ControllFlags == ( TCP_SYN_FLAG + TCP_ACK_FLAG ) ) // Packet mit SYN + ACK ?
			{
				// SequenceNumber um 1 erhöhen, das gehört zur SYN-sequence dazu
				TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit( TCP_packet->TCP_AcknowledgeNumber );
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit( TCP_packet->TCP_SequenceNumber );
				TCP_sockettable[ Socket ].AcknowledgeNumber++;
				// Windowsize setzen, wird gebraucht um zu wissen wiviel gesendet werden kann ohne ACK
				TCP_sockettable[ Socket ].Windowsize = TCP_packet->TCP_Window; 
				// Baue mal den TCP-Header mit Berechnung des Pseudoheader
				// MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , MAX_RECIVEBUFFER_LENGHT , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_READY ; // State für den Socket auf WAIT2SYNACK und den den SYN abschließen zu können
				TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND ; // bereit zum senden
				return;
			}

		// anzahl der Daten im TCP-Datagram berechnen
		i =	( ChangeEndian16bit ( IP_packet->IP_Totallenght )
					- ( ( IP_packet->IP_Version_Headerlen & 0x0f ) * 4 + ( ( TCP_packet->TCP_DataOffset & 0xf0 ) >> 2 ) ) ) ;
		
		if ( Get_FIFOrestsize( TCP_sockettable[ Socket ].fifo ) < i )
		{
			return;
		}
		
		// Wenn Daten vorhanden einffach mal kopieren in den Socket-Puffer und bestätigung senden
		if ( i != 0 )
		{
			// Richtige Reihnfolger der Daten ?
			// Hier kann erzwungen werden das die in der richtigen reihnfolge ankommen, 
			// wenn nicht wird ein ACK-Packet mit einer alten Seqnummer gesendet
			if ( ( TCP_sockettable[ Socket ].AcknowledgeNumber ) == ( ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) ) )
			{
				// Wenn kopieren in Puffer ok war, Acknummer richtig setzen, sonst nix machen, dann wird automatisch die alte Seqnummer gesendet
				// mit der richtigen Windowsize
				if ( CopyTCPdata2socketbuffer( Socket, i , ethernetbuffer ) != SOCKET_ERROR )
					TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + i ;					

				// ACK senden
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );

				#ifdef TCP_with_unsortseq
				// schau mal nach ob vorher ein TCP-Packet kam was in der falschen Reinfolge an kam
				if ( TCP_Unsort.socket == Socket && TCP_Unsort.lenght != 0 )
				{
					if ( TCP_sockettable[ Socket ].AcknowledgeNumber + TCP_Unsort.lenght == TCP_Unsort.Sequencenumber + TCP_Unsort.lenght )
					{
						LockEthernet();
						// SREG sichern um state zu behalten und interrupts freigeben
						sreg_tmp = SREG;
						sei();
						Put_Block_in_FIFO ( TCP_sockettable[ Socket ].fifo , TCP_Unsort.lenght, TCP_Unsort.Recivebuffer );
						// SREG wiederherstellen
						SREG = sreg_tmp;
						FreeEthernet();
						
						TCP_sockettable[ Socket ].AcknowledgeNumber = TCP_Unsort.Sequencenumber + i ;
						MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
					}
					else
					{
						RXErrorCounter++;
					}
				}
				TCP_Unsort.lenght = 0;
				#endif
			}
			else
			{
				
				// Naja, falsche reihnfolge kann man nur sagen :-) aber kein Problem
				// alte sequencenummer ? wenn ja nochmal bestätigen
				if ( ( TCP_sockettable[ Socket ].AcknowledgeNumber + i ) > ( ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + i ) )
				{
					unsigned long ACK;
					
					ACK = TCP_sockettable[ Socket ].AcknowledgeNumber;
					TCP_sockettable[ Socket ].AcknowledgeNumber = TCP_packet->TCP_SequenceNumber;
					MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
					TCP_sockettable[ Socket ].AcknowledgeNumber = ACK;
					
					RXErrorOldSeq++;
				}
				#ifdef TCP_with_unsortseq
				else
				{
					// speicher mal die Daten zwischen, damit sie nicht verloren gehen, aber nur wenn der Puffer noch nicht belegt ist, sonst RX-Error
					if ( TCP_Unsort.lenght == 0 )
					{
						unsigned int Offset = ETHERNET_HEADER_LENGTH + ( IP_packet->IP_Version_Headerlen & 0x0f ) * 4 + ( ( TCP_packet->TCP_DataOffset & 0xf0 ) >> 2 ) ;				
						TCP_Unsort.Sequencenumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber );
						TCP_Unsort.socket = Socket ;
						
						LockEthernet();
						// SREG sichern um state zu behalten und interrupts freigeben
						sreg_tmp = SREG;
						sei();
						memcpy( &TCP_Unsort.Recivebuffer , &ethernetbuffer[ Offset ] , i );
						// SREG wiederherstellen
						SREG = sreg_tmp;
						FreeEthernet();
						
						TCP_Unsort.lenght = i;					
						RXErrorUnsort++;
						}
					else
					{
						RXErrorCounter++;
					}
				}
				#endif
			}
		}

		// Anforderung für das schliessen des Socket/Verbindung
		if ( TCP_packet->TCP_ControllFlags == ( TCP_FIN_FLAG ) )
		{
			// haben wir die Schliessung angefordert ?
			if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FIN )
			{
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				#ifdef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = CloseTimeout ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				#ifndef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = 0 ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
			}
			// haben wir die Schliessung angefordert ?
			if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FINACK )
			{
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				#ifdef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = CloseTimeout ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				#ifndef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = 0 ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
			}
			// Client hat Schliessung angefordert
			else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_READY ) 
			{
				TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG | TCP_FIN_FLAG , 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
//				MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FIN ;
			}				

			// Laut RFC ist Seqnummer um 1 zu erhöhen beim FIN
			TCP_sockettable[ Socket ].SequenceNumber++;
			return;
		}

		if ( TCP_packet->TCP_ControllFlags == ( TCP_FIN_FLAG | TCP_ACK_FLAG) )
		{
			if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FINACK )
			{
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
				TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber );
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
			}
			else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_READY ) 
			{
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
//				TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG | TCP_FIN_FLAG , 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FIN ;
			}
			return;
		}

		// normale bestätigung der gesendeten Daten
		if ( TCP_packet->TCP_ControllFlags == TCP_ACK_FLAG ) 						// mach mal wenn ACK empfangen
			{
				// War das ein ACK von einen Verbindungsaufbau ? Wenn ja steht die verbindung jetzt
				if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2SYNACK )  			// Wartet socket auf SYN + ACK ?
				{
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_READY2USE;					// Socket auf Ready2Use setzen
					TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND;						// Socket auf Bereit um senden setzen
				}				
				else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FIN ) 
				{
				#ifdef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = CloseTimeout ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				#ifndef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = 0 ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				}
				else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FINACK )
				{
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FIN ;
				}
				else if ( TCP_sockettable[ Socket ].SequenceNumber == ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber ) )
				{
					TCP_sockettable[ Socket ].SendetBytes = 0;
				}
			}
		return;
	}
		
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Der TCP-Timeouthandler
 * Dieser Handler schaut zyklisch in alle Sockets und verringert den Timeoutwert. Wenn ein Timeoutwert 0 erreicht
 * hat, wird das Socket geschlossen und der Connectionstats richtig gesetz.
 * \warning Die Funktion wir in der tcp_init() beim timerinterrupt registiert und fortan zyklisch aufgerufen.
 * \retval	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void TCPTimeOutHandler( void )
	{
		int Socket;
		// durchläufe MAX_TCP_CONNECTIONS

		if ( TCP_LOCK == Lock )
			return;

		// Die einzelnen Socket nach einander durchgehen
		for ( Socket = 0 ; Socket < MAX_TCP_CONNECTIONS ; Socket ++ )
			{
				// Wenn TimeOutcounter unsgleich 0 dann verringern
				if ( TCP_sockettable[ Socket ].Timeoutcounter != 0 ) TCP_sockettable[ Socket ].Timeoutcounter--;

				// Puffer für reservieren
				unsigned char ethernetbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ];
				LockEthernet();

				if ( ( TCP_sockettable[ Socket ].Timeoutcounter == ( TimeOutCounter / 3 ) ) && TCP_sockettable[ Socket ].ConnectionState != SOCKET_NOT_USE )
				{
					MakeTCPheader( Socket, TCP_FIN_FLAG | TCP_ACK_FLAG , 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FINACK ;						
				}
				else if ( TCP_sockettable[ Socket ].Timeoutcounter == 0 && TCP_sockettable[ Socket ].ConnectionState != SOCKET_NOT_USE )
				{
					MakeTCPheader( Socket, TCP_RST_FLAG , 0 , 0 , ethernetbuffer );
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;						
				}

				FreeEthernet();
			}
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sperrt den TCP-Timeouthandler
/*------------------------------------------------------------------------------------------------------------*/
void LockTCP()
{
	TCP_LOCK = Lock;
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt den TCP-Timeouthandler frei
/*------------------------------------------------------------------------------------------------------------*/
void FreeTCP()
{
	TCP_LOCK = Unlock;
}
		
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt das naechste freie Socket
 * \warning Die Funktion wir in der tcp_init() beim timerinterrupt registiert und fortan zyklisch aufgerufen.
 * \retval	SOCKET	Gibt die Nummer des naechsten Freien SOcket.
 */
/*------------------------------------------------------------------------------------------------------------*/
int Getfreesocket( void )
	{
		int Socket;
		for ( Socket = 0 ; Socket < MAX_TCP_CONNECTIONS ; Socket++ )
			{
				if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_NOT_USE && TCP_sockettable[ Socket ].Timeoutcounter == 0 ) return( Socket );
			}
		return( SOCKET_ERROR );
	}		

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Ordnet das Packet einen Socket zu.
 * \param	ethernetbuffer	Zeiger auf den Ethernetpuffer.
 * \retval	Socket	Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
int GetSocket( unsigned char * ethernetbuffer )
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		int Socket;
		
		for ( Socket = 0 ; Socket < MAX_TCP_CONNECTIONS ; Socket++ ) 
			{
				if ( 	TCP_sockettable[ Socket ].ConnectionState != SOCKET_NOT_USE 
						&& TCP_sockettable[ Socket ].SourcePort == TCP_packet->TCP_SourcePort 
						&& TCP_sockettable[ Socket ].DestinationPort == TCP_packet->TCP_DestinationPort 
						&& TCP_sockettable[ Socket ].SourceIP == IP_packet->IP_SourceIP ) return( Socket );
			}
		return( SOCKET_ERROR );
	}		
	
/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Reistriert einen neuen Socket wenn noch einer frei ist und sichert alle nötigen daten in ihn.
 * \param	ethernetbuffer	Zeiger auf den Ethernetpuffer.
 * \retval	Socket			Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
 int RegisterSocket( unsigned char *ethernetbuffer)
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		int Socket;
		
		Socket = Getfreesocket();
		if ( Socket == SOCKET_ERROR ) return( SOCKET_ERROR );
		
		// Registert das SOCKET
		TCP_sockettable[ Socket ].SourcePort = TCP_packet->TCP_SourcePort;
		TCP_sockettable[ Socket ].DestinationPort = TCP_packet->TCP_DestinationPort;
		TCP_sockettable[ Socket ].SourceIP = IP_packet->IP_SourceIP;
		TCP_sockettable[ Socket ].SequenceNumber =~ ChangeEndian32bit( TCP_packet->TCP_SequenceNumber );
		TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit( TCP_packet->TCP_SequenceNumber );
		TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND;
		TCP_sockettable[ Socket ].Timeoutcounter = 10;
		Flush_FIFO ( TCP_sockettable[ Socket ].fifo );
		TCP_sockettable[ Socket ].Windowsize = TCP_packet->TCP_Window ;
		TCP_sockettable[ Socket ].SendetBytes = 0;
		
		for ( int i = 0 ; i < 6 ; i++ ) TCP_sockettable[ Socket ].MACadress[i] = ETH_packet->ETH_sourceMac[i];
		
		return( Socket );
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Baut einen TCP-header und berechnet den Pseudoheader und die Checksumme 
 *	Übergeben werden müssen die TCP_FLAGS, Datenlänge den Datensegments, die Windowsize, und der Pointer auf den Buffer
 * \param	Socket			Socket für den der TCP-header gebaut werden soll.
 * \param	TCP_flags		Die TCP-flags die gesetzt werden sollen.
 * \param	Datalenght		Anzahl der Daten in Bytes die gesendet werden sollen.
 * \param	Windowsize		Größe der Windowsize in Bytes (Anzahl der freien Bytes im Puffer).
 * \param	ethernetbuffer	Zeiger auf den Ethernetpuffer.
 * \retval	void
 */
/*------------------------------------------------------------------------------------------------------------*/
void MakeTCPheader( int Socket, unsigned char TCP_flags, unsigned int Datalenght, unsigned int Windowsize, unsigned char * ethernetbuffer )
	{
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];

		struct IP_Pseudoheader *IP_pseudopacket;
		IP_pseudopacket = ( struct IP_Pseudoheader *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT - IP_PSEUDOHEADER_LENGHT ];

		// Bastel mal den Pseudoheader
		IP_pseudopacket->IP_SourceIP = TCP_sockettable[ Socket ].SourceIP;
		IP_pseudopacket->IP_DestinationIP = myIP;
		IP_pseudopacket->IP_ZERO = 0x0;
		IP_pseudopacket->IP_Protokoll = 0x6;
		IP_pseudopacket->IP_TCP_lenght = ChangeEndian16bit( TCP_HEADER_LENGHT + Datalenght );										
			
		// TCP_header basteln
		TCP_packet->TCP_SourcePort = TCP_sockettable[ Socket ].DestinationPort;
		TCP_packet->TCP_DestinationPort = TCP_sockettable[ Socket ].SourcePort;
		TCP_packet->TCP_SequenceNumber = ChangeEndian32bit( TCP_sockettable[ Socket ].SequenceNumber );
		TCP_packet->TCP_AcknowledgeNumber = ChangeEndian32bit( TCP_sockettable[ Socket ].AcknowledgeNumber );
		TCP_packet->TCP_DataOffset = ( ( TCP_HEADER_LENGHT << 2 ) & 0xf0 ) ;
		TCP_packet->TCP_ControllFlags = TCP_flags ;
		TCP_packet->TCP_Window = ChangeEndian16bit( Windowsize );
		TCP_packet->TCP_Checksum = 0x0;
		TCP_packet->TCP_UrgentPointer = 0x0;
		
		if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_SYNINIT )
		{
			// beschreibe das Optionfeld mit MSS = MAX_TCP_Datalenght wenn der SYN ausgehandelt wird
			TCP_packet->TCP_Options[0] = 0x02;
			TCP_packet->TCP_Options[1] = 0x04;
			TCP_packet->TCP_Options[2] = ( MAX_TCP_Datalenght >> 8 ) & 0x00ff ;
			TCP_packet->TCP_Options[3] = MAX_TCP_Datalenght & 0x00ff ;
			TCP_packet->TCP_Options[4] = 0;
			TCP_packet->TCP_Options[5] = 0;
			TCP_packet->TCP_Options[6] = 0;
			TCP_packet->TCP_Options[7] = 0;
			TCP_packet->TCP_Options[8] = 0;
		}
		else
		{
			// lasse Optionfeld leer wenn kein SYN
			TCP_packet->TCP_Options[0] = 0;
			TCP_packet->TCP_Options[1] = 'H';
			TCP_packet->TCP_Options[2] = 'u';
			TCP_packet->TCP_Options[3] = 'r';
			TCP_packet->TCP_Options[4] = 'r';
			TCP_packet->TCP_Options[5] = 'i';
			TCP_packet->TCP_Options[6] = 'c';
			TCP_packet->TCP_Options[7] = 'a';
			TCP_packet->TCP_Options[8] = 'n';
		}
			
		TCP_packet->TCP_Checksum = ChangeEndian16bit( Checksum_16( 	&ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT - IP_PSEUDOHEADER_LENGHT ],
																	TCP_HEADER_LENGHT + IP_PSEUDOHEADER_LENGHT  + Datalenght ) ) ;
		MakeIPheader( TCP_sockettable[ Socket ].SourceIP, PROTO_TCP, TCP_HEADER_LENGHT + Datalenght , ethernetbuffer );
		MakeETHheader( TCP_sockettable[ Socket ].MACadress, ethernetbuffer );
		sendEthernetframe( ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT + Datalenght, ethernetbuffer );
		
		return;
	}
		
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Kopiert die Daten aus dem TCP-packet in den Socketpuffer
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die größe der Daten in Bytes.
 * \param	ethernetbuffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	0
 */
/*------------------------------------------------------------------------------------------------------------*/	
int CopyTCPdata2socketbuffer( int Socket, unsigned int Datalenght , unsigned char *ethernetbuffer )
	{
		char sreg_tmp;
		int Offset;
		
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];
			
		// Abbrechen wenn die Datem nicht mehr in den buffer passen
		if ( ( Get_FIFOrestsize ( TCP_sockettable[ Socket ].fifo ) ) < Datalenght ) return( SOCKET_ERROR );
		
		Offset = ETHERNET_HEADER_LENGTH + ( IP_packet->IP_Version_Headerlen & 0x0f ) * 4 + ( ( TCP_packet->TCP_DataOffset & 0xf0 ) >> 2 ) ;

		// gibt mal interrupts im interrupt frei außer den netzwerkinterrupt, da das kopieren der daten manchmal zu lange dauert und sonst
		// die Clock hinterher hinkt :-(
		LockEthernet();
   		sreg_tmp = SREG;    /* Sichern */
		sei();
		Put_Block_in_FIFO ( TCP_sockettable[ Socket ].fifo, Datalenght, &ethernetbuffer[ Offset ] );
		SREG = sreg_tmp;
		FreeEthernet();

		return( Datalenght );
	}		
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert einen Port auf den gelauscht wird für einegehende Verbindungen
 * \param	Port	Der Port auf den Gelauscht werden soll
 * \retval	Im Erfolgsfall 0, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
int RegisterTCPPort( unsigned int Port )
	{
		int i;
		
		for ( i = 0 ; i < MAX_LISTEN_PORTS ; i ++ ) 
			{
				if ( TCP_porttable[i].TCP_Port == TCP_Port_not_use ) 
				{
					TCP_porttable[i].TCP_Port = Port;
					return(0);
				}
			}
		return( SOCKET_ERROR );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Traegt einen Port aus der Liste der Port aus auf denen gelauscht werden soll
 * \param	Port	Der Port auf den Gelauscht werden soll
 * \return	Im Erfolgsfall 0, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
void UnRegisterTCPPort( unsigned int Port )
	{
		int i;
		
		for ( i = 0 ; i < MAX_LISTEN_PORTS ; i ++ ) 
			{
				if ( TCP_porttable[i].TCP_Port == Port ) TCP_porttable[i].TCP_Port = TCP_Port_not_use;
			}
		return;
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Checkt ob ein TCP-Port schon auf lauschen steht.
 * \param	Port	Der Port der gecheckt werden soll.
 * \retval	0x0 wenn der Port in der Liste ist und 0xffff wenn er nicht in der Liste ist.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int CheckPortInList( unsigned int Port )
	{
		int i;
		
		for ( i = 0 ; i < MAX_LISTEN_PORTS ; i ++ ) 
			{
				if ( TCP_porttable[i].TCP_Port == Port ) return(0);
			}
		return( SOCKET_ERROR );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Schaut ob auf einen Port ein Verbindung eingegangen ist.
 * \param	Port	Der Port der abgefragt wird.
 * \return	Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/	
int CheckPortRequest( unsigned int Port )
	{
		int i;
		
		for ( i = 0 ; i < MAX_TCP_CONNECTIONS ; i ++ ) 
			{
				if ( TCP_sockettable[i].DestinationPort == ChangeEndian16bit( Port ) && TCP_sockettable[i].ConnectionState == SOCKET_READY2USE ) 
				{
					TCP_sockettable[i].ConnectionState = SOCKET_READY;
					return(i);
				}
			}
		return( SOCKET_ERROR );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt den Status des Socket aus.
 * \param	Socket		Die Socketnummer vom dem der Status zurueckgegeben werden soll.
 * \return	Der Socketstate
 */
/*------------------------------------------------------------------------------------------------------------*/	
char CheckSocketState( int Socket )
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS ) return( SOCKET_ERROR );
		return( TCP_sockettable[ Socket ].ConnectionState );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Schliesst ein Socket und Beendet die TCP-Verbindung.
 * \param	Socket		Die Socketnummer die geschlossen werden soll.
 */
/*------------------------------------------------------------------------------------------------------------*/		
void CloseTCPSocket( int Socket)
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS  || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return;
					
		int timer;
		
		timer = CLOCK_RegisterCoundowntimer();
		if( timer == CLOCK_FAILED ) return;
		
		CLOCK_SetCountdownTimer( timer , CLOSETIMEOUT, MSECOUND );

		while ( 1 )
		{
			if ( ( TCP_sockettable[ Socket ].SendState == SOCKET_READY2SEND ) && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				break;
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				#ifdef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = CloseTimeout ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				#ifndef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = 0 ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				CLOCK_ReleaseCountdownTimer( timer );
				return;
			}
		}
		
		LockEthernet();
		
		unsigned char ethernetbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ];
		
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];

		MakeTCPheader( Socket, TCP_FIN_FLAG | TCP_ACK_FLAG , 0, ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
		
		TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FINACK;
	
		FreeEthernet();		

		timer = CLOCK_RegisterCoundowntimer();
		if( timer == CLOCK_FAILED ) return;
		CLOCK_SetCountdownTimer( timer , CLOSETIMEOUT, MSECOUND );
		
		#ifdef TCP_delayed_ack
			LockEthernet();

			MakeTCPheader( Socket, TCP_FIN_FLAG | TCP_ACK_FLAG , 0, ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );

			FreeEthernet();		
		#endif

		while ( 1 )
		{
			LockEthernet();
			
			if ( ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_NOT_USE ) && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				FreeEthernet();
				break;
			}
			
			FreeEthernet();

			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				LockEthernet();
//				MakeTCPheader( Socket, TCP_RST_FLAG , 0, 0, ethernetbuffer );
				#ifdef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = CloseTimeout ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				#ifndef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = 0 ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				FreeEthernet();

				CLOCK_ReleaseCountdownTimer( timer );
				break;
			}
		}
		
		// TCP_sockettable[ Socket ].fifo = TCP_sockettable[ Socket ].old_fifo;

		return;
	}	

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet Daten ueber ein Socket aus dem RAM/FLASH/EEPROM.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die Datenlaenge in Bytes die sersendet werden soll.
 * \param	Sendbuffer	Zeiger auf die Dten im RAM der versendet werden soll.
 * \param	Mode		Art des Zeigers (RAM/FLASH/EEPROM).
 * \param	RetransmissionCounter		Anzahl der bereits erfolgten Retransmissions.
 * \return	Datalenght	Anzahl der gesendet Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int SendData_RPE( int Socket, unsigned int Datalenght, unsigned char * Sendbuffer, unsigned char Mode, int RetransmissionCounter )	
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( SOCKET_ERROR );
		
		int timer;

		unsigned char ethernetbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT + MAX_TCP_Datalenght ];

		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];	

		unsigned int Offset = ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ;
		unsigned int i;

		switch ( Mode )
		{
			case RAM:	for ( i = 0 ; i < Datalenght ; i++ )
								ethernetbuffer[ Offset + i ] = Sendbuffer[ i ]; // kopiere die daten in den Sendebiffer
						break;
			case FLASH:	for ( i = 0 ; i < Datalenght ; i++ )
								ethernetbuffer[ Offset + i ] = pgm_read_byte ( Sendbuffer + i ) ;
						break;
			default:	return( SOCKET_ERROR );
		}

		LockEthernet();
		
		MakeTCPheader( Socket, TCP_PSH_FLAG | TCP_ACK_FLAG , Datalenght, MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ethernetbuffer );
		TCP_sockettable[ Socket ].SequenceNumber = TCP_sockettable[ Socket ].SequenceNumber + i;
		TCP_sockettable[ Socket ].SendetBytes = Datalenght ;

		FreeEthernet();
		
		timer = CLOCK_RegisterCoundowntimer();
		if( timer == CLOCK_FAILED ) return( SOCKET_ERROR );

		
		if ( RetransmissionCounter < 1 )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT/16 , MSECOUND );
		else if ( RetransmissionCounter < MAX_TCP_RETRANSMISSIONS/4 )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT/4 , MSECOUND );
		else if ( RetransmissionCounter < MAX_TCP_RETRANSMISSIONS/2 )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT , MSECOUND );
		else if ( RetransmissionCounter < MAX_TCP_RETRANSMISSIONS )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT , MSECOUND );

		#ifdef TCP_delayed_ack
			LockEthernet();
		
			TCP_sockettable[ Socket ].SequenceNumber = TCP_sockettable[ Socket ].SequenceNumber - i;
			TCP_sockettable[ Socket ].SendetBytes = Datalenght ;
			MakeTCPheader( Socket, TCP_ACK_FLAG , 0, MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ethernetbuffer );
			TCP_sockettable[ Socket ].SequenceNumber = TCP_sockettable[ Socket ].SequenceNumber + i;
			TCP_sockettable[ Socket ].SendetBytes = Datalenght ;

			FreeEthernet();
		#endif

		while(1)
		{
			LockEthernet();
			if( CLOCK_GetCountdownTimer( timer ) == 0 )
			{
				CLOCK_ReleaseCountdownTimer ( timer );
				TCP_sockettable[ Socket ].SequenceNumber = TCP_sockettable[ Socket ].SequenceNumber - i ;
				FreeEthernet();
				TXErrorCounter++;
				return( SOCKET_ERROR );
			}
			else
			{
				if ( TCP_sockettable[ Socket ].SendetBytes == 0 )
				{
					CLOCK_ReleaseCountdownTimer ( timer );
					FreeEthernet();
					return( Datalenght );
				}
			}
			FreeEthernet();
		}
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet Daten ueber ein Socket aus dem RAM/FLASH/EEPROM.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die Datenlaenge in Bytes die sersendet werden soll.
 * \param	Sendbuffer	Zeiger auf die Daten im FLASH der versendet werden soll.
 * \param	Mode		Wie der Zeiger interpretiert werden soll, als RAM/FLASH/EEPROM.
 * \return	Datelenght	Anzahl der gesendeten Bytes oder -1 bei Fehler.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int PutSocketData_RPE( int Socket, unsigned int Datalenght, unsigned char * Sendbuffer, unsigned char Mode )
	{
		// schau mal ob socket gültig ist
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( SOCKET_ERROR );
			
		int Transmission=0,i,packet,TransmissionCounter=0;		

		// Anzahl der Packet berechnen die die maximallänge haben
		packet = ( Datalenght / MAX_TCP_Datalenght );
		
		// Packet mit maximallänge senden
		for ( i = 0 ; i < packet ; i++ )
		{
			Transmission = 1;
			TransmissionCounter = 0;
			while( Transmission != 0 && TransmissionCounter < MAX_TCP_RETRANSMISSIONS )
			{
				switch ( Mode )
				{
					case RAM:	if ( SendData_RPE( Socket, MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], RAM, TransmissionCounter ) != SOCKET_ERROR )
								{
									Transmission = 0;
								}
								else
								{
									Transmission = 1;
									TransmissionCounter++;
								}
								break;
					case FLASH:	if ( SendData_RPE( Socket, MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], FLASH, TransmissionCounter ) != SOCKET_ERROR )
								{
									Transmission = 0;
								}
								else
								{
									Transmission = 1;
									TransmissionCounter++;
								}
								break;
					default:	return( -1 );
				}
			}
		}

		if ( TransmissionCounter == MAX_TCP_RETRANSMISSIONS )
		{
			CloseTCPSocket( Socket );
			return( SOCKET_ERROR );
		}
				
		// Wenn noch Byte übrig sind, senden
		if ( ( Datalenght % MAX_TCP_Datalenght ) != 0 )
		{
			Transmission = 0;
			TransmissionCounter = 0;
			while( Transmission == 0 && TransmissionCounter < MAX_TCP_RETRANSMISSIONS )
			{
				switch ( Mode )
				{
					case RAM:	if ( SendData_RPE( Socket, Datalenght % MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], RAM, TransmissionCounter ) != SOCKET_ERROR )
								{
									Transmission = 1;
								}
								else
								{
									Transmission = 0;
									TransmissionCounter++;
								}
								break;
					case FLASH:	if ( SendData_RPE( Socket, Datalenght % MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], FLASH, TransmissionCounter ) != SOCKET_ERROR )
								{
									Transmission = 1;
								}
								else
								{
									Transmission = 0;
									TransmissionCounter++;
								}
								break;
					default:	return( -1 );
				}
			}
		}

		if ( TransmissionCounter == MAX_TCP_RETRANSMISSIONS )
		{
			CloseTCPSocket( Socket );
			return( SOCKET_ERROR );
		}
		// anzahl der gesendet daten zurück geben
		return( Datalenght );		
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer und schreibt ihn in einen FIFO-Puffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	fifo		Fifo in den die Daten kopiert werden sollen.
 * \param	bufferlen	Anzahl der Bytes die kopiert werden soll.
 * \retval	Datalenght	Anzahl der kopierten Bytes oder -1 bei Fehler.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int GetSocketDataToFIFO( int Socket , unsigned int fifo, unsigned int bufferlen )
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( SOCKET_ERROR );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return ( 0 );
		
		LockEthernet();
		
		unsigned int i = 0;
		
		if ( ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) >= bufferlen ) && ( Get_FIFOrestsize ( fifo ) >= bufferlen ) )
		{
			i = Get_FIFO_to_FIFO( TCP_sockettable[ Socket ].fifo , bufferlen, fifo );
		}
		else
		{
			FreeEthernet();
			return( 0 );
		}
				
		// sende ein Windowupdate-Paket wenn buffer zu 7/8 frei ist
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) < ( MAX_RECIVEBUFFER_LENGHT / 2 ) )
		{
			unsigned char ackbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ];

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );
		}

		FreeEthernet();
		return( bufferlen );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Datalenght	Anzahl der gesendeten Bytes oder -1 bei Fehler.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int GetSocketData( int Socket , unsigned int bufferlen, unsigned char *buffer)
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( SOCKET_ERROR );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return ( 0 );
		
		LockEthernet();
		
		unsigned int i = 0, Byte_in_fifo=0;
		
		// Wenn der Buffer kleiner als der Empfangsbuffer Buffer voll machen, sonst alles kopieren
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) > bufferlen )
		{
			Get_Block_from_FIFO ( TCP_sockettable[ Socket ].fifo, bufferlen, buffer );	
		}
		else
		{
			Byte_in_fifo = Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo );
			for ( i = 0 ; i < Byte_in_fifo ; i++ )
				buffer[ i ] = Get_Byte_from_FIFO ( TCP_sockettable[ Socket ].fifo );
			Flush_FIFO ( TCP_sockettable[ Socket ].fifo );
		}
		
		// sende ein Windowupdate-Paket wenn buffer zu 3/4 frei ist
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) < ( MAX_RECIVEBUFFER_LENGHT / 2 ) )
		{
			unsigned char ackbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ];

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );
		}

		FreeEthernet();
		return( i );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Löscht den Empfangspuffer
 * \param	Socket		Socketnummer von welchen der Puffer gelöscht werden soll.
 * \retval	Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int FlushSocketData( int Socket )
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( SOCKET_ERROR );
		
		return( Flush_FIFO( TCP_sockettable[ Socket ].fifo ) );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Anzahl der Daten im Puffer des Sockets.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \retval	Datalenght	Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int GetBytesInSocketData( int Socket )
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( SOCKET_ERROR );
		
		return( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
	}
	
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \retval	char		Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
char GetByteFromSocketData( int Socket )
	{
		if ( Socket < 0 || Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE) return( 0 );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return ( 0 );
		
		char Data;
		
		LockEthernet();
		
		Data = Get_Byte_from_FIFO ( TCP_sockettable[ Socket ].fifo );

		// Sendet ein Update wenn Buffer leer
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 )
		{
			unsigned char ackbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ];

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );
		}
		
		FreeEthernet();

		return( Data );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Baut eine TCP-Verbindung zu einer IP-Adresse auf.
 * \param	IP		Die IP-Adresse des Zielhost.
 * \param	Port	Ziel-Port des Zielhost.
 * \retval	Socket	Die Socketnummer der aufgebauten verbindung oder 0xffff im Fehlerfall.
 */
/*------------------------------------------------------------------------------------------------------------*/	
int Connect2IP( unsigned long IP, unsigned int Port )
	{
		int Socket, i;
		struct TIME time;
		// hole einen freien SOCKET

		Socket = Getfreesocket();
		// Wenn kein SOCKET mehr frei, EXIT!
		if ( Socket == SOCKET_ERROR ) 
		{
			FreeEthernet();
			return ( SOCKET_ERROR );
		}
		// Arp-request senden und schaun ob ip vorhanden
		if ( IS_ADDR_IN_MY_SUBNET( IP, Netmask ) )
		{
			if ( IS_BROADCAST_ADDR( IP, Netmask ) ) 
			{
				for( i = 0 ; i < 6 ; i++ ) TCP_sockettable[ Socket ].MACadress[i] = 0xff;
			}
			else if ( GetIP2MAC( IP, TCP_sockettable[ Socket ].MACadress ) == NO_ARP_ANSWER ) return ( SOCKET_ERROR );
		}
		else if ( GetIP2MAC( Gateway , TCP_sockettable[ Socket ].MACadress ) == NO_ARP_ANSWER ) return ( SOCKET_ERROR );
		
		LockEthernet();
		
		CLOCK_GetTime( &time );
		
		// Socket reservieren
		TCP_sockettable[ Socket ].ConnectionState = SOCKET_SYNINIT;
		TCP_sockettable[ Socket ].DestinationPort =~ time.ss + time.ms;
		
		unsigned char ethernetbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ];
	
		struct TCP_header * TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];
		
		// Register den SOCKET
		TCP_sockettable[ Socket ].SourcePort = ChangeEndian16bit( Port ) ;
		TCP_sockettable[ Socket ].SourceIP = IP;
		TCP_sockettable[ Socket ].SequenceNumber =~ 0x12345678;
		TCP_sockettable[ Socket ].AcknowledgeNumber = 0x0 ;
		TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND;
		TCP_sockettable[ Socket ].Timeoutcounter = 10;
		Flush_FIFO ( TCP_sockettable[ Socket ].fifo );
		TCP_sockettable[ Socket ].Windowsize = 0;
		TCP_sockettable[ Socket ].SendetBytes = 0;		

		MakeTCPheader( Socket, TCP_SYN_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ethernetbuffer );
	
		FreeEthernet();

		int timer;
		
		timer = CLOCK_RegisterCoundowntimer();
		if ( timer == CLOCK_FAILED ) return ( SOCKET_ERROR );
		
		CLOCK_SetCountdownTimer( timer , CONNECTTIMEOUT, MSECOUND );

		while ( 1 )
		{
			LockEthernet();
			if ( (  TCP_sockettable[ Socket ].ConnectionState == SOCKET_READY ) && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , MAX_RECIVEBUFFER_LENGHT , ethernetbuffer );
				FreeEthernet();
				return( Socket );
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				#ifdef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = CloseTimeout ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				#ifndef CLOSE_WAIT
					TCP_sockettable[ Socket ].Timeoutcounter = 0 ;
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				#endif
				CLOCK_ReleaseCountdownTimer( timer );
				FreeEthernet();
				return( SOCKET_ERROR );
			}
			FreeEthernet();
		}
}
//@}

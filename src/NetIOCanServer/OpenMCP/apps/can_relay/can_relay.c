/////*! \file telnet.c \brief Ein sehr einfacher Telnetclient */
//***************************************************************************
//*            telnet.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup telnet Ein sehr einfacher Telnetclient (telnet.c)
///	\code #include "telnet.h" \endcode
///	\par Uebersicht
/// 	Ein sehr einfacher Telnet-client der auf dem Controller läuft. Ermöglicht
/// das Abfragen vom Status des Controllers und diverse andere Dinge.
/// \date	04-18-2008: -> Der Blödsinn mit Windows ist beseitigt, geht jetzt. Es wird jetzt pro Durchlauf versucht, den Puffer auszulesen und
///			erst wenn eine Eingabe mit 0x0a,0x0d (LF, CR) abgeschlossen wird, gehts ab an die Verarbeitung des Strings. Was für ein Akt :-) und alles wegen
///			Windows.
/// \date   05-25-2009: Umstellung der Telnetstruktur und Ausführung der Kommandos.
/// \date	01-01-2010:	Umstellung der Telnetstruktur für das ausführen und registrieren von Telnetkommandos. Diese werden jetzt von extern Registriert, so das ein
///			saubere Trennung besteht.
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
#include <avr/version.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "system/net/udp.h"
#include "system/stdout/stdout.h"
#include "system/thread/thread.h"

#include "can_relay.h"
#include "config.h"
#include "hardware/CAN_Lib/utils.h"
#include "system/net/ip.h"

#define CAN_PORT0 2
#define CAN_PORT1 3

char CAN_ReceiveBuffer[CAN_RELAY_BUFFER_LEN] ;
char CAN_SendBuffer[CAN_RELAY_BUFFER_LEN] ;
int CAN_Socket ;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert den can_relay-clinet und registriert den Port auf welchen dieser lauschen soll.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/

extern unsigned long myBroadcast ;

void can_relay_init()
{
	int i;

	CAN_Socket = UDP_RegisterSocket(myBroadcast,CAN_RELAY_PORT,CAN_RELAY_BUFFER_LEN,CAN_ReceiveBuffer );
	printf_P( PSTR("CAN-Relay-Server gestartet auf Port %d.\r\n") , CAN_RELAY_PORT );

	if (CAN_Socket!=UDP_SOCKET_ERROR) {	
		THREAD_RegisterThread( can_relay_thread, PSTR("can_relay"));
	} ;
}


uint8_t CANRouting (uint32_t CAN_ID)
{
	uint8_t Linie ;
	
	Linie = (CAN_ID>>10)&0xf ;
  
	if (Linie==0x1) {
		return (0) ;
	} else if (Linie==0x2) {
		return (1) ;
	} else if (Linie==0x3) {
		return (2) ;
	} else if (Linie==0x4) {
		return (3) ;
	} 
	
	return (-1) ;
}

void CANtoUDP (can_t *InMessage, char* Buffer)
{
	char *CANIDP;
	int i ;
	
	CANIDP = (char*)&(InMessage->id) ;
	
	for (i=0;i<4;i++) Buffer[i] = CANIDP[i] ;
	Buffer[4] = InMessage->length ;
	for (i=0;i<InMessage->length;i++) Buffer[i+5] = InMessage->data[i] ;
	for (;i<8;i++) Buffer[i+5]=0 ;
}

void UDPtoCAN (char *Buffer, can_t *OutMessage)
{
	char *CANIDP ;
	int i ;
	
	CANIDP = (char*)&(OutMessage->id) ;
	for (i=0;i<4;i++) CANIDP[i] = Buffer[i] ;
	OutMessage->length=Buffer[4];
	for (i=0;i<OutMessage->length;i++) OutMessage->data[i]=Buffer[i+5];
	for (;i<8;i++) OutMessage->data[i]=0 ;
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Der can-relay-client an sich. Er wird zyklisch aufgerufen und schaut nach ob eine Verbindung auf den
 * registrierten Port eingegangen ist. Wenn ja holt er sich die Daten und sendet sie entsprechend der Routing-
 * Tabelle an die CAN-Ports. Umgekehrt werden CAN-Nachrichten von einem Port an den anderen und an das Netz 
 * gesendet.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	

can_t SendMessage ;

void can_relay_thread()
{
	static int a=0 ;
	char Data; 
	int i;		
	
	
	if ( UDP_GetSocketState( CAN_Socket ) == UDP_SOCKET_BUSY ) { // UDP recheived
		UDPtoCAN(CAN_ReceiveBuffer,&SendMessage) ;
		UDP_FreeBuffer(CAN_Socket) ;
		if (CANRouting(SendMessage.id)==CAN_PORT0) {
			can_send_message(0,&SendMessage) ;
		} ;
		if (CANRouting(SendMessage.id)==CAN_PORT1) {
			can_send_message(1,&SendMessage) ;
		} ;
	}


	SendMessage.id = 0 ;
		
	if (CanRX0!=0) { // CAN 0 Received 
			// Copy complete CAN Message
		cli() ; // Sichern des Interrupts nicht notwendig, da wir sonst nicht in den Threads währen.
		SendMessage = RXMessage0 ;
		sei() ;
		if (CANRouting (SendMessage.id)==CAN_PORT1) { // Send it also to the other CAN
			can_send_message(1,&SendMessage) ; // und an das andere Interface
		} ;
		CanRX0 = 0 ;
	} ;
	if (CanRX1!=0) { // CAN 1 Received 
		cli() ; // Sichern des Interrupts nicht notwendig, da wir sonst nicht in den Threads währen.
		SendMessage = RXMessage1 ;
		sei() ;
		if (CANRouting (SendMessage.id)==CAN_PORT0) { // Send it also to the other CAN
			can_send_message(0,&SendMessage) ; // und an das andrere Interface
		} ;							
		CanRX1 = 0 ;
	} ;
		
	if (SendMessage.id !=0) { // Es wurde etwas empfangen, also auf UDP herausgeben.
		CANtoUDP(&SendMessage,CAN_SendBuffer);
		UDP_SendPacket (CAN_Socket,13,CAN_SendBuffer) ;		
	}
}


//@}

/*! \file "arp.c" \brief Die Arp-Funktionlitaet */
//***************************************************************************
//*            arp.c
//*
//*  Mon Aug 28 22:31:14 2006
//*  Copyright  2006  sharandac
//*  Email sharandac(at)snafu.de
//****************************************************************************/
///	\ingroup network
///	\defgroup ARP ARP-Funktionen (arp.c)
///	\code #include "arp.h" \endcode
///	\code #include "ip.h" \endcode
///	\code #include "ethernet.h" \endcode
///	\par Uebersicht
/// \todo	Arp sicherer machen, bekommt manchmal die Antworten nicht mit und bis jetzt nur
///			ein Eintrag in der Arp-Table möglich
/// \date	06-04-2008 Arp umgebaut, Arp-Table geht jetzt und es kommen keine fails mehr vor
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
#include <avr/pgmspace.h>
#include <stdio.h>

#include "arp.h"
#include "ethernet.h"
#include "ip.h"
#include "system/clock/clock.h"
#include "system/string/string.h"

struct ARP_TABLE ARPtable[ MAX_ARPTABLE_ENTRYS ];

/*!\brief Der ARP-Table Timeouthandler für ARP-Einträge
 */
void ARP_Timeouthandler( void )
{
	unsigned char i;
	
	for ( i = 0 ; i < MAX_ARPTABLE_ENTRYS ; i++ )
		if ( ARPtable[ i ].ttl != 0 )
			ARPtable[ i ].ttl--;
}

/*!\brief Initialisiert die ARP
 */
void ARP_INIT( void )
{
	unsigned int i;
	
	for( i = 0; i < MAX_ARPTABLE_ENTRYS ; i++ )
		ARPtable[ i ].ttl = 0;

	CLOCK_RegisterCallbackFunction( ARP_Timeouthandler, SECOUND );
}

/*!\brief Die ARP-Funktion wenn ein ARP-Packet eintrifft
 * \param packet_lenght			Anzahl der Byte im Ethernetbuffer
 * \param ethernetbuffer		Zeiger auf den Ethernetbuffer an sich
 */
void arp( unsigned int packet_lenght, unsigned char *ethernetbuffer)
	{
		unsigned char i,a;
		
		struct ETH_header *ETH_packet;
		ETH_packet = (struct ETH_header *) ethernetbuffer; 
		struct ARP_header *ARP_packet;
		ARP_packet = (struct ARP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		switch ( ARP_packet->ARP_Opcode ) {
			
			case 0x0100:		// check mal ob das für mich ist !
								if ( ARP_packet->ARP_destIP != myIP ) return ;

/*								// Wenn IP bekannt aktuallisieren
								for( i = 0 ; i < MAX_ARPTABLE_ENTRYS ; i++ )
								{
									if( ARPtable[ i ].IP == ARP_packet->ARP_sourceIP )
									{
										ARPtable[i].ttl = Default_ARP_ttl;
										for ( a = 0 ; a < 6 ; a++ ) ARPtable[i].MAC[a] = ARP_packet->ARP_sourceMac[a];
									}
								}
*/
								// wenn ja fang mal an die antwort zusammen zu basteln
								ARP_packet->ARP_Opcode = 0x0200;
								// mac und ip des senders in ziel kopieren
								for ( i = 0; i < 10; i++ )ARP_packet->ARP_destMac[i] = ARP_packet->ARP_sourceMac[i]; // MAC und IP umkopieren
								// meine mac und ip als absender einsetzen
								for ( i = 0; i < 6 ; i++ )ARP_packet->ARP_sourceMac[i] = mymac[i]; // MAC einsetzen
								ARP_packet->ARP_sourceIP = myIP ; // IP einsetzen
								// sourceMAC in destMAC eintragen und meine MAC in sourceMAC kopieren
								for( i = 0 ; i < 6 ; i++){	
										ETH_packet->ETH_destMac[i] = ETH_packet->ETH_sourceMac[i];	
										ETH_packet->ETH_sourceMac[i] = mymac[i]; }
								sendEthernetframe( packet_lenght, ethernetbuffer);
								break;
			case 0x0200:		// check mal ob das für mich ist !
								for( i = 0 ; i < MAX_ARPTABLE_ENTRYS ; i++ )
								{
									if( ARPtable[ i ].IP == ARP_packet->ARP_sourceIP )
									{
										ARPtable[i].IP = ARP_packet->ARP_sourceIP;
										ARPtable[i].ttl = Default_ARP_ttl;
										for ( a = 0 ; a < 6 ; a++ ) ARPtable[i].MAC[a] = ARP_packet->ARP_sourceMac[a];
									}
								}
								break;
		}
	}

/*!\brief Ermittelt für eine IP-Adresse die MAC-Adresse.
 * \param IP			Die IP-Adresse von der die MAC-Adresse ermittelt werden soll.
 * \param MACbuffer		Zeiger auf de Speicherbereich von 6Byte wo die MAC-Adresse abgelegt wird.
 * \return				Die MAC-Adresse im Puffer.
 * \retval ARP_ANSWER	Die MAC anfrage war erfolgrich.
 * \retval NO_ARP_ANSWER Die MAC-anfrage ist Fehlgeschlagen.
 */
unsigned int GetIP2MAC( unsigned long IP, unsigned char * MACbuffer )
	{
		unsigned int i,a,ARPentry=0;

		// Ist die MAC schon in der Liste und gültig ?
		for( i = 0 ; i < MAX_ARPTABLE_ENTRYS ; i++ )
		{		
			if ( ARPtable[ i ].ttl > 0 )
			{
				if ( ARPtable[ i ].IP == IP )
				{
					for( a = 0 ; a < 6 ; a++ ) MACbuffer[a] = ARPtable[ i ].MAC[a];
					return( ARP_ANSWER );
				}
			}
		}

		// Suche nach freien Eintrag
		for( i = 0 ; i < MAX_ARPTABLE_ENTRYS ; i++ )
		{
			if ( ARPtable[ i ].ttl == 0 )
			{
				ARPtable[ i ].IP = IP;
				ARPentry = i;
				break;
			}
		}
		
		unsigned char * ethernetbuffer;
		ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + ARP_HEADER_LENGHT );
		
		struct ETH_header *ETH_packet;
		ETH_packet = (struct ETH_header *) ethernetbuffer; 
		struct ARP_header *ARP_packet;
		ARP_packet = (struct ARP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];

		// Hardware Type Ethernet
		ARP_packet->HWtype = 0x0100;
		// Protocoltype = IP
		ARP_packet->Protocoltype = 0x0008;
		// Hardware adresse size 6
		ARP_packet->HWsize = 0x06;
		// Protocolsize 4
		ARP_packet->Protocolsize = 0x4;
		// Opcode für request
		ARP_packet->ARP_Opcode = 0x0100;
		// mac auf 0 setzen
		for ( i = 0; i < 6; i++ )ARP_packet->ARP_destMac[i] = 0x00;
		// IP die abgefragt werden soll
		ARP_packet->ARP_destIP = IP;
		// meine mac und ip als absender einsetzen
		for ( i = 0; i < 6 ; i++ )ARP_packet->ARP_sourceMac[i] = mymac[i]; // MAC einsetzen
		ARP_packet->ARP_sourceIP = myIP ; // IP einsetzen
		// sourceMAC in destMAC in Ethernetframe eintragen

		for( i = 0 ; i < 6 ; i++){	
				ETH_packet->ETH_destMac[i] = 0xff;	
				ETH_packet->ETH_sourceMac[i] = mymac[i]; }
		ETH_packet->ETH_typefield = 0x0608;
				
		sendEthernetframe( ETHERNET_HEADER_LENGTH + ARP_HEADER_LENGHT , ethernetbuffer);
		
		unsigned char timer;
		
		timer = CLOCK_RegisterCoundowntimer();
		CLOCK_SetCountdownTimer( timer, 10 , MSECOUND );

		while ( 1 )
		{
			if ( ( ARPtable[ ARPentry ].ttl > 0 ) )
			{
				for( i = 0 ; i < 6 ; i++ ) MACbuffer[i] = ARPtable[ ARPentry ].MAC[i];
				CLOCK_ReleaseCountdownTimer( timer );
				return( ARP_ANSWER );
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				CLOCK_ReleaseCountdownTimer( timer );
				return( NO_ARP_ANSWER );
			}
		}
	}

/*!\brief Zeigt einen eintrag aus der ARP-table.
 * \param Entry			Nummer des Eintrages der gezeigt werden soll.
 * \param IP			Zeiger auf den Speicher in der die IP gespeichert werden soll.
 * \param MAC			Zeiger auf den Speicher in der die MAC gespeichert werden soll.
 * \param ttl			ttl = TimeToLive.
 * \return				Rückgabewerd der Funktion
 * \retval 1			Die MAC-Anfrage war erfolgrich.
 * \retval 0			Die MAC-Anfrage ist Fehlgeschlagen.
 */
unsigned char GetARPtableEntry( unsigned int Entry, unsigned long * IP, unsigned char * MAC, unsigned char * ttl)
{
	unsigned int i;
	
	if ( Entry >= MAX_ARPTABLE_ENTRYS ) return 0;
	
	*IP = ARPtable[ Entry ].IP;
	for( i = 0 ; i < 6 ; i++ ) MAC[ i ] = ARPtable[ Entry ].MAC[ i ];
	*ttl = ARPtable[ Entry ].ttl;

	return(1);
}

/*!\brief Wandelt eine MAC-Adresse in einen String
 * \param MAC		Pointer auf die MAC-Adresse.
 * \param strMAC	Pointer wohin der String solll.
 * \return			Rüchgabewert der Funktion.
 * \retval 0		Wandlung war erfolgreich. 
 * \retval -1		Wandlung war nicht erfolgreich. 
 */
char * mactostr( unsigned char * MAC, char * strMAC )
{
	sprintf_P( strMAC, PSTR("%02x:%02x:%02x:%02x:%02x:%02x") , MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5] );
	return( strMAC );
}
//@}

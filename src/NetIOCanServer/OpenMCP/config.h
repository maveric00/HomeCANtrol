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
 
#ifndef _CONFIG_H
	#define CONFIG_H

	// aktiviert 1-Wire
//	#define ONEWIRE 
	// aktiviert ADC
//	#define ANALOG
	// aktiviert GPIO
//	#define GPIO
	// aktiviert EXTINT
	#define EXTINT
	// aktiviert PCint
	#define PC_INT
	// aktivier das LED-Modul
//	#define LED
	// aktiviert MMC
	#define MMC
	// aktiviert TWI
//	#define TWI
	// LCD-Display aktivieren, nur auf dem myEthernet !!!
//	#define LCD
	#ifdef LCD
		// LCD-Display aktivieren, nur auf dem myEthernet !!!
		#define myAVR_LCD
	#endif
	// Impulsezähler aktivierenm nur auf dem myEthernet !!!
//	#define IMPULSCOUNTER
	// aktiviert den Treiber für den VS10XX
//	#define VS10XX

	// Die LED-Anzeigetafel im CCC-Berlin, braucht kein anderer :-D
//	#define LEDTAFEL

	// Hier kann für den ENC28j60 der Link eingestellt werden, es kann nur einer gewählt werden
//	#define ETH_LINK_CONFIG
	#define ETH_LINK_FULL
//	#define ETH_LINK_HALF

	// aktiviert TCP
	#define TCP
	#ifdef TCP
		// Erlaubt den Stack ein Packet zwischen zu speichern wenn sie in der Falschen reihenfolge kommen
		#define TCP_with_unsortseq
		// Beschleunigt TCP in Verbindung mit Windows
		#define TCP_delayed_ack
		// wartet auf das schließen einer Verbindung
//		#define CLOSE_WAIT
	#endif

	// aktiviert UDP
	#define UDP
	
	#ifdef UDP
		// aktiviert DHCP, dazu muss UDP aktiv sein
		// Wenn DHCP nicht benutzt wird kann die statische IP in
		// system/net/ip.c geändert werden
//		#define DHCP
		// Versucht bei einem Fehlversuch von DHCP die IP aus der Config zu lesen, sonst die in ip.c
		#define READ_CONFIG
		// aktiviert DNS, dazu muss UDP aktiv sein
		#define DNS
		// aktiviert NTP, dazu muss UDP aktiv sein
		#define NTP
	#endif

	#ifdef TCP

		// aktiviert DynDNS
		#ifdef DNS
//			#define DYNDNS
		#endif

		// aktiviert Twitter
//		#define TWITTER

		// aktiviert den Telnet-Server, dazu muss TCP aktiv sein
		#define TELNETSERVER

		// aktiviert den HTTP-Server, dazu muss TCP aktiv sein
		#define HTTPSERVER
		#ifdef HTTPSERVER
			// aktiviert das Streaminginterface
//			#define HTTPSERVER_STREAM
			// aktiviert die Stats per Webinterface
			#define HTTPSERVER_STATS
			// aktiviert die Konfiguration per Webinterface
			#define HTTPSERVER_NETCONFIG

			// aktiviert das System-Menü per Webinterface
			#define HTTPSERVER_SYSTEM
			#ifdef HTTPSERVER_SYSTEM
				#ifdef DYNDNS
					#define HTTPSERVER_DYNDNS
				#endif
				// aktiviert twitter per Webinterface
				#if defined(TWITTER)
					#define HTTPSERVER_TWITTER
				#endif
				// aktiviert ntp per Webinterface
				#if defined(NTP)
					#define HTTPSERVER_NTP
				#endif
				// aktiviert eemem per Webinterface
				#define HTTPSERVER_EEMEM
				// aktiviert reset per Webinterface
				#define HTTPSERVER_CRON
				// aktiviert reset per Webinterface
				#define HTTPSERVER_RESET
			#endif

			// aktiviert IO auf dem Webinterface
			#define HTTPSERVER_IO
			#ifdef HTTPSERVER_IO
				// aktiviert ADC auf dem Webinterface
				#ifdef ANALOG
					#define HTTPSERVER_ANALOG
				#endif
				// aktiviert GPIO auf dem Webinterface
				#ifdef GPIO
					#define HTTPSERVER_GPIO
				#endif
				// aktiviert 1-Wire auf dem Webinterface
				#ifdef ONEWIRE
					#define HTTPSERVER_ONEWIRE
				#endif
				// aktiviert TWI auf dem Webinterface
				#ifdef TWI
					#define HTTPSERVER_TWI
				#endif
				// aktivert Impulsecounter per Webinterface
				#ifdef IMPULSCOUNTER
					#define HTTPSERVER_IMPULSECOUNTER
				#endif
			#endif
		#endif
	#endif

#endif /* CONFIG_H */


/*! \mainpage Willkommen zum Mikrowebserver Projekt
 *
 * \section intro_sec Übersicht
 *
 * Zu Beginn der Aufstellung von Anforderungen an das uns gedanklich vorschwebende Projekt 
 * war eine Präzisierung der Funktionen des Microwebservers notwendig. Hier ging es vor allem 
 * um die vielseitige Nutzung des Gerätes, sowohl für Schüler, Studenten und Anfänger auf dem 
 * Gebiet der Programmierung als auch für fortgeschrittene Programmierer, Hobbyelektroniker 
 * und Dozenten in entsprechenden Unterrichtsfächern oder Kursen. Es sollte eine einfach zu 
 * bedienende und recht anschlussfreudige Entwicklungsumgebung für Schulungszwecke geschaffen werden, 
 * die aber auch im Hausgebrauch oder in der Industrie sinnvoll eingesetzt werden kann. 
 * Uns war aber auch klar, dass es schon eine Vielzahl von Mikrocontrollersystemen gab. Deshalb 
 * konzipierten wir ein System, welches den Ansprüchen an ein vollwertige Embbeded System gerecht wird, 
 * aber den Laien nicht mit Komplexität erschlägt. Folgende Funktionen sollte unser System vorweisen:
 *<br>
 *<br>
 *   * leicht zu programmierender Microcontroller <br>
 *   * kein Ausbau des Controllers aus der Hardware zum Programmieren (ISP)<br>
 *   * Programmieren in Assembler und C/C++ möglich<br>
 *   * vielfältige Schnittstellenverfügbarkeit wie RS232, SPI, USART, I2C(TWI), Netzwerk und Analogeingänge<br>
 *   * Debuggmodus (Industristandart JTAG) für Schulungs- und Entwicklungszwecke muss möglich sein<br>
 *   * hohe Taktrate des Systems (>10MHz)<br>
 *   * ausreichend Speicher (256kB Flash, 128kB RAM)<br>
 *   * kompakte Abmessungen der Platine<br>
 *   * wenige Bauelemente, geringe Beschaltung der IC`s<br>
 *   * großer Spannungsversorgungsbereich<br>
 *   * kostengünstige und leicht verfügbare Programmiersoftware und Programmiergeräte <br>
 *<br>
 * Dieses Projekt hat das Ziel, eine Entwicklungsumgebung für einen ATmega2561 mit einem
 * Netzwerkinterface zu erstellen, das mit Hilfe des ENC28j60 von Microchip realisiert wird. Ziel ist
 * es, eine Plattform für Entwickler zu bieten, der die wichtigsten Funktionen zur Verfügung 
 * stellt, so dass der Entwickler sich auf die eigentliche Applikation konzentrieren kann.
 */

/****************************************************************************
 *            main.c
 *
 *  Mon May 29 20:26:59 2007
 *  Copyright  2007  Dirk Broßwick
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
#include "system/init.h"
#include "apps/apps_init.h"
#include "apps/modul_init.h"

int main( void ) 
{
	// System initialisieren
	init();

	// Applikationen initialisieren (http, telnet, cron, .... )
	apps_init();

	// Module initialisieren ( cmd, cgi .... )
	modul_init();
	
	// die mainloop, hier wird alles abgearbeitet
	while(1)
	{
		THREAD_mainloop();
	}
}
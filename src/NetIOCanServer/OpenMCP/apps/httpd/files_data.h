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
 
#ifndef _FILES_DATA_H
	#define FILES_DATA_H

	#define MAX_FILES_ENTRYS 	32

	typedef struct {
		const prog_char	*filesname;
		const prog_char	*files;
		const prog_char	filestype;
		const int	len;
	} const FILES ;

	#define	TEXT	0
	#define	JPEG	1
	#define PNG		2

	#include "config.h"

const char files1[] PROGMEM = "index.html";
const char data1[] PROGMEM = {
	"<HTML>"
	"<HEAD>"
	"<TITLE>OpenMCP - Welcome on Microcontroller Board</TITLE>"
	"</HEAD>"
	"<frameset rows=\"60,35,*"
	#ifdef HTTPSERVER_STATS
		",40"
	#endif
	"\" scrolling=\"no\" frameborder=\"2\" border=\"2\" framespacing=\"2\" bordercolor=\"#000000\">"
  	"<frame src=\"headline.html\" name=\"Navigation1\" scrolling=\"no\">"
  	"<frame src=\"mainmenu.html\" name=\"Navigation2\" scrolling=\"no\">"
  	"<frame src=\"info.html\" name=\"main\" scrolling=\"no\">"
	#ifdef HTTPSERVER_STATS
  	"<frame src=\"stats.cgi\" name=\"update\" scrolling=\"no\">"
	#endif
  	"<noframes>"
    "<body>"
    "<p>Ihr Browser unterstützt keine Frames!</p>"
    "</body>"
  	"</noframes>"
	"</frameset>"
	"</HTML>"
	"\r\n\r\n"	};

const char files2[] PROGMEM = "headline.html";
const char data2[] PROGMEM = {
	"<HTML>"
	"<HEAD>"
	"<TITLE>OpenMCP</TITLE>"
	"</HEAD>"
	"<BODY bgcolor=\"#006400\" text=\"#FFFAF0\">"// colors 6666ff__FFFFFF
	"<h1>OpenMCP - Welcome on Microcontroller Board</h1>"
	"</BODY>"
	"</HTML>"
	"\r\n\r\n"	};


#ifdef HTTPSERVER_IO
	const char files3[] PROGMEM = "io.html";
	const char data3[] PROGMEM = {
		"<HTML>"
		"<HEAD>"
		"<TITLE>OpenMCP</TITLE>"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
		"</HEAD>"
		"<BODY bgcolor=\"#228B22\" text=\"#FFFFFF\">"//8888FF und FFFFFF
		"<a href=\"mainmenu.html\">zurueck</a>"
		#ifdef HTTPSERVER_GPIO
			" / <a href=\"dio_out.cgi\" target=\"main\">Digital Out</a> / <a href=\"dio_in.cgi\" target=\"main\">Digital In</a>"
		#endif
		#ifdef HTTPSERVER_ANALOG
			" / <a href=\"aio.cgi\" target=\"main\">Analog In</a>"
		#endif
		#ifdef HTTPSERVER_ONEWIRE
			" / <a href=\"onewire.cgi\" target=\"main\">1-Wire</a>"
		#endif
		#ifdef HTTPSERVER_TWI
			" / <a href=\"twi.cgi\" target=\"main\">TWI</a>"
		#endif
		#if defined(LEDTAFEL)
			" / <a href=\"tafel.cgi\" target=\"main\">LED-Tafel</a>"
		#endif
		#if defined( IMPULSCOUNTER )
			" / <a href=\"impuls.cgi\" target=\"main\">Impulsz&auml;hler</a>"
		#endif
		"</BODY>"
		"</HTML>"
		"\r\n\r\n"	};
#endif

#if defined(HTTPSERVER_STREAM)
const char files9[] PROGMEM = "stream.html";
const char data9[] PROGMEM = {
	"<HTML>"
	"<HEAD>"
	"<TITLE>OpenMCP</TITLE>"
	"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
	"</HEAD>"
	"<BODY bgcolor=\"#228B22\" text=\"#FFFFFF\">"//8888FF und FFFFFF
	"<a href=\"mainmenu.html\">zurueck</a> / <a href=\"stream.cgi\" target=\"main\">Stream</a> / <a href=\"stream.cgi?info\" target=\"main\">Infos</a> / <a href=\"stream.cgi?config\" target=\"main\">Konfiguration</a>"
	"</BODY>"
	"</HTML>"
	"\r\n\r\n"	};
#endif

const char files4[] PROGMEM = "mainmenu.html";
const char data4[] PROGMEM = {
	"<HTML>"
	"<HEAD>"
	"<TITLE>OpenMCP</TITLE>"
	"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
	"</HEAD>"
	"<BODY bgcolor=\"#228B22\" text=\"#FFFFFF\">"
	"<a href=\"info.html\"target=\"main\">Informationen</a>"
	#if defined(HTTPSERVER_STREAM)
		" / <a href=\"stream.html\">Stream</a>"
	#endif
	#ifdef HTTPSERVER_IO
		" / <a href=\"io.html\">IO-Ports</a>"
	#endif
	#ifdef HTTPSERVER_NETCONFIG
		" / <a href=\"network.html\">Netzwerk</a>"
	#endif
	#ifdef HTTPSERVER_SYSTEM
		" / <a href=\"system.html\">System</a>"
	#endif
	"</BODY>"
	"</HTML>"
	"\r\n\r\n"	};

#ifdef HTTPSERVER_NETCONFIG
const char files5[] PROGMEM = "network.html";
const char data5[] PROGMEM = {
	"<HTML>"
	"<HEAD>"
	"<TITLE>OpenMCP</TITLE>"
	"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
	"</HEAD>"
	"<BODY bgcolor=\"#228B22\" text=\"#FFFFFF\">"
	"<a href=\"mainmenu.html\">zurueck</a> / <a href=\"network.cgi\" target=\"main\">Infos</a> / <a href=\"network.cgi?config\" target=\"main\">Konfiguration</a>"
	"</BODY>"
	"</HTML>"
	"\r\n\r\n"	};
#endif

#ifdef HTTPSERVER_SYSTEM
const char files6[] PROGMEM = "system.html";
const char data6[] PROGMEM = {
	"<HTML>"
	"<HEAD>"
	"<TITLE>OpenMCP</TITLE>"
	"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">"
	"</HEAD>"
	"<BODY bgcolor=\"#228B22\" text=\"#FFFFFF\">"
	"<a href=\"mainmenu.html\">zurueck</a> "
#if defined(HTTPSERVER_RESET)
	"/ <a href=\"reset.cgi\" target=\"main\">Reset</a>"
#endif
#if defined(HTTPSERVER_NTP)
	" / <a href=\"ntp.cgi\" target=\"main\">NTP</a>"
#endif
#if defined(HTTPSERVER_DYNDNS)
	" / <a href=\"dyndns.cgi\" target=\"main\">DynDNS</a>"
#endif
#if defined(HTTPSERVER_TWITTER)
	" / <a href=\"twitter.cgi\" target=\"main\">Twitter</a>"
#endif
#if defined(HTTPSERVER_EEMEM)
	" / <a href=\"eemem.cgi\" target=\"main\">EEmem</a>"
#endif
#if defined(HTTPSERVER_CRON)
	" / <a href=\"cron.cgi\" target=\"main\">cron</a>"
#endif
	"</BODY>"
	"</HTML>"
	"\r\n\r\n"	};
#endif

const char files7[] PROGMEM = "info.html";
const char data7[] PROGMEM = {
	"<HTML>"
	"<BODY>"
	"<pre><p>"
	"  Welcome on\r\n"
	"       ____            ______________\r\n"
	"      /   /           /     / __/   /\r\n"
	"     / / /___________/ / / / / / / /\r\n"
	"    / / /   / __/   / / / / / / __/\r\n"
	"   / / / / / __/ / / / / / /_/ /\r\n"
	"  /___/ __/___/_/_/_/_/_/___/_/     Open MicroControllerProjekt\r\n"
	"     /_/\r\n"
	"  \r\n"
	"For more information visit the <a href=\"http://wiki.neo-guerillaz.de\" target=\"_blank\">OpenMCP</a> projectpage.\r\n"
//    "Or visit the <a href=\"http://www.twitter.com/openmcp\" target=\"_blank\">OpenMCP Twitter</a>.\r\n"
	"Microwebserver build on AVR-libc version: " __AVR_LIBC_VERSION_STRING__ "/" __AVR_LIBC_DATE_STRING__ " with avr-gcc "__VERSION__", Date: " __DATE__ " "__TIME__ " \r\n"
	"\r\n"
	"(c)2006-2009   Software: Dirk Brosswick (sharandac@snafu.de)\r\n"
#ifdef __AVR_ATmega2561__
	#if defined(OpenMCP)
	"               Hardware: Peter Wiedorn (peter.wiedorn@gmx.de)\r\n"
	#endif
#endif
	"</pre></p>"
	"</BODY>"
	"</HTML>"
	"\r\n"	};

const char files8[] PROGMEM = "style.css";
const char data8[] PROGMEM = {
	"@charset \"ISO-8859-1\"\r\n"
	"   a:link { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:visited { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:hover { text-decoration:none; font-weight:bold; background-color:#4169E1; }\r\n"
	"   a:active { text-decoration:none; font-weight:bold; background-color:#FFFFFF; }\r\n"
	"   a:focus { text-decoration:none; font-weight:bold; background-color:#DDA0DD; }\r\n" };

FILES files[] = {
	{ files1, data1, TEXT, sizeof( data1 ) - 1 },
	{ files2, data2, TEXT, sizeof( data2 ) - 1 },
#ifdef HTTPSERVER_IO
	{ files3, data3, TEXT, sizeof( data3 ) - 1 },
#endif
#ifdef HTTPSERVER_NETCONFIG
	{ files5, data5, TEXT, sizeof( data5 ) - 1 },
#endif
#ifdef HTTPSERVER_SYSTEM
	{ files6, data6, TEXT, sizeof( data6 ) - 1 },
#endif
	{ files7, data7, TEXT, sizeof( data7 ) - 1 },
	{ files8, data8, TEXT, sizeof( data8 ) - 1 },
	{ files4, data4, TEXT, sizeof( data4 ) - 1 },
#if defined(HTTPSERVER_STREAM)
	{ files9, data9, TEXT, sizeof( data4 ) - 1 },
#endif
	{ 0,0,0,0 }
};

#endif /* FILES_DATA_H */

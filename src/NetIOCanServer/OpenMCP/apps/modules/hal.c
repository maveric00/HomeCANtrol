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
 
#include <avr/pgmspace.h>
#include <avr/version.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"


#include "hardware/hal.h"

#include "apps/telnet/telnet.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"
#include "hal.h"

void modul_hal_init( void )
{
#if defined(HTTPSERVER_HAL)
	cgi_RegisterCGI( modul_cgi_hal, PSTR("hal.cgi") );
#endif
}

void modul_cgi_hal( void * pStruct )
{
	printf_P( PSTR(	"<HTML>\r\n"
					"<HEAD>\r\n"
					"<TITLE>ADC</TITLE>\r\n"
					"</HEAD>\r\n"
					"<BODY>\r\n"
					" <pre>\r\n" ));

	HAL_printconfig();
	
	printf_P( PSTR(	" </pre>\r\n"
					"</BODY>\r\n"
					"</HTML>\r\n"
					"\r\n"));
	
}


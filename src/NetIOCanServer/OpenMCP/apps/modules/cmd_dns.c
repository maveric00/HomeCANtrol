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

#if defined(DNS)

#include "system/net/arp.h"
#include "system/net/ip.h"
#include "system/net/dns.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"

#include "apps/telnet/telnet.h"
#include "cmd_dns.h"

void init_cmd_dns( void )
{
#if defined(TELNETSERVER)
	telnet_RegisterCMD( cmd_dns, PSTR("dns"));
#endif
}

int cmd_dns( int argc, char ** argv )
{
	long ip;
	char ipstr[ 20 ];
	
	if ( argc == 2 )
	{
		ip = DNS_ResolveName( argv[ 1 ] );
		printf_P( PSTR("IP zu %s = %s\r\n"), argv[ 1 ], iptostr( ip, ipstr ) );
	}
	else
		printf_P( PSTR("dns <hostname>\r\n"));
}
#endif

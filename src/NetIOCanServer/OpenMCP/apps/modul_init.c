/***************************************************************************
 *            modul_init.c
 *
 *  Mon Dec 14 16:54:30 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "apps/telnet/telnet.h"
#include "config.h"

// hier kommen die includes der Befehle rein die Eingebunden werden
#include "apps/modules/cmd_arp.h"
#include "apps/modules/cmd_reset.h"
#include "apps/modules/cmd_stats.h"
#include "apps/modules/cmd_ifconfig.h"
#include "apps/modules/cmd_eemem.h"
#include "apps/modules/cmd_cron.h"
#if defined(ANALOG)
	#include "apps/modules/cmd_adc.h"
#endif
#if defined(GPIO)
	#include "apps/modules/cmd_gpio.h"
#endif
#ifdef DNS
	#include "apps/modules/cmd_dns.h"
	#ifdef DYNDNS
		#include "apps/modules/cmd_dyndns.h"
	#endif
#endif
#if defined(NTP)
	#include "apps/modules/cmd_ntp.h"
#endif
#if defined(TWITTER)
	#include "apps/modules/cmd_twitter.h"
#endif
#if defined(TWI)
	#include "apps/modules/cmd_twi.h"
#endif
#if defined(ONEWIRE)
	#include "apps/modules/cmd_onewire.h"
#endif
#if defined(LEDTAFEL)
	#include "apps/modules/cmd_tafel.h"
#endif
#if defined(HTTPSERVER_STREAM)
	#include "apps/modules/cmd_stream.h"
#endif
#include "apps/modules/hal.h"
#if defined(myAVR) && defined( IMPULSCOUNTER )
	#include "apps/modules/impulse.h"
#endif

#include "apps/modules/temp_json.h"
#include "apps/modules/cmd_temp.h"

void modul_init( void )
{
	// Befehle registrieren
	init_cmd_arp();
	init_cmd_dns();
	init_cmd_stats();
	init_cmd_ifconfig(),
	init_cmd_eemem();
	init_cmd_reset();
	init_cmd_cron();
#if defined(NTP)
	init_cmd_ntp();
#endif
#if defined(DNS)
	init_cmd_dns();
	#if defined(DYNDNS)
		init_cmd_dyndns();
	#endif
#endif
#if defined(HTTPSERVER_TWITTER)
	init_cmd_twitter();
#endif
#if defined(ANALOG)
	init_cmd_adc();
#endif
#if defined(GPIO)
	init_cmd_gpio();
#endif
#if defined(TWI)
	init_cmd_twi();
	temp_json_init();
#endif
#if defined(ONEWIRE)
	init_cmd_onewire();
#endif
#if defined(LEDTAFEL)
	init_cmd_tafel();
#endif
#if defined(HTTPSERVER_STREAM)
	init_cmd_stream();
#endif
#if defined(myAVR) && defined( IMPULSCOUNTER )
	IMPULS_init();
#endif
	init_cmd_temp();
	
	//	modul_hal_init();
}

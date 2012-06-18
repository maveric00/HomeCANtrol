/***************************************************************************
 *            temp_json.c
 *
 *  Sun Dec 20 00:51:51 2009
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "config.h"

#if defined(TWI)

#include "system/thread/thread.h"
#include "system/clock/clock.h"
#include "apps/httpd/cgibin/cgi-bin.h"
#include "apps/httpd/httpd2.h"
#include "apps/httpd/httpd2_pharse.h"

#include "hardware/twi/twi.h"

#include "temp_json.h"

struct TIME lasttime;

struct TEMP_JSON temp_json;

void temp_json_init( void )
{
	int i;
	
	struct TIME nowtime;

	CLOCK_GetTime ( &nowtime );

	lasttime.hh = -1;
	lasttime.DD = -1;

	THREAD_RegisterThread( temp_json_thread, PSTR("Temp Logger"));
	cgi_RegisterCGI( temp_json_cgi, PSTR("temp.json"));
	cgi_RegisterCGI( temp_logger_cgi, PSTR("templogger.cgi"));

	temp_json_thread();
}

void temp_json_thread( void )
{
	int Temp;
		
	struct TIME nowtime;

	CLOCK_GetTime ( &nowtime );

	// Log löschen wenn neuer Tag
	if ( lasttime.DD != nowtime.DD )
	{
		lasttime.DD = nowtime.DD;
		temp_logger_clean();
	}
	
	// log schreiben wenn neue Stünde
	if ( lasttime.hh != nowtime.hh )
	{
		lasttime.hh = nowtime.hh;

		TWI_Write( 0 );
		TWI_SendStop();

		TWI_SendAddress( 0x4f, TWI_READ );
		Temp = ( TWI_ReadAck() << 8 );
		Temp |= ( TWI_ReadNack() );
		TWI_SendStop();

		temp_json.temp[ lasttime.hh ] = Temp;	
	}
}

void temp_json_cgi( void * pStruct )
{
	int i;
	int Temp;
	char VZ;
	
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;

//	printf_P( PSTR("name: 'Sensor1',"));
	printf_P( PSTR("[ "));

	for ( i = 0 ; i < TEMP_MAX ; i++ )
	{
		Temp = temp_json.temp[ i ];

		if ( i > 0 )
			printf_P( PSTR(","));

		if ( Temp < 0 )
			VZ = '-';
		else
			VZ = '+';
		
		if ( Temp != 0x8000 )
			printf_P( PSTR("%c%d.%01d "), VZ, abs(Temp / 256) , abs((Temp << 8 ) / 6250 )); // 0.1øC
		else
			printf_P( PSTR("null ") );
	}

	printf_P( PSTR(" ]"));

}

void temp_logger_cgi( void * pStruct )
{
	
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;

	struct TIME nowtime;

	CLOCK_GetTime ( &nowtime );

	printf_P(	PSTR(	"<html>"
						"<head>"
						"<script type=\"text/javascript\" src=\"jquery.min.js\"></script>"
						"<script type=\"text/javascript\" src=\"highcharts.js\"></script>"
						"<script type=\"text/javascript\" src=\"excanvas-compressed.js\"></script>"
						"<script type=\"text/javascript\">"
						"$(document).ready(function(){"
						"var chart=new Highcharts.Chart({"
   						"chart:{renderTo:'container',defaultSeriesType:'spline',margin:[50, 180, 60, 80]},"
   						"title:{text:'Temperaturverlauf',style:{margin:'10px 100px 0 0'}},"
  	 					"xAxis:{type:'datetime'},"
   						"plotOptions:{spline:{lineWidth:2,marker:{enabled:false},pointInterval:3600000,pointStart:Date.UTC(%d,%d,%d,0,0,0),states:{hover:{marker:{enabled:true,symbol:'circle',radius:3,lineWidth:1}}}}},"
						"yAxis:{title:{text:'Temperature (&deg;C)'}},"
						"tooltip:{formatter:function(){return'<b>'+ this.series.name +'</b><br>'+Highcharts.dateFormat('%%e. %%b %%Y, %%H:00',this.x)+': '+this.y+'&deg;C';2}},"
						"legend:{layout:'vertical',style:{left:'auto',bottom:'auto',right:'10px',top: '100px'}},"
						"series:[{name:'Sensor1',dataURL:'/temp.json'}]"
//						"series:[{nameURL:'/temp.json',dataURL:'/temp.json'}]"
						"});});"
						"</script>"
						"</head>"
						"<body>"
						"<div id=\"container\" style=\"width:800px;height:400px\"></div>"
						"</body>"
						"</html>"
						"\r\n\r\n" ), nowtime.YY, nowtime.MM-1, nowtime.DD );
}

void temp_logger_clean( void )
{
	int i;

	for ( i = 0 ; i < TEMP_MAX ; i++ )
		temp_json.temp[ i ] = 0x8000 ;

}
#endif

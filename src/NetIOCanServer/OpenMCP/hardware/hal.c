/***************************************************************************
 *            hal.c
 *
 *  Sun Dec 13 16:45:12 2009
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

#include "system/net/tcp.h"
#include "system/stdout/stdout.h"

#include "config.h"

#include "hal.h"

#if defined(__AVR_ATmega644P__) && defined(myAVR)
HAL hal_table[] = {
	// Port A
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC | __CONFIG_LCD ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC | __CONFIG_LCD ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC | __CONFIG_LCD ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC | __CONFIG_LCD ) } ,

	// Port B
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_LCD ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_LCD ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_LCD ) } ,
	{ __LCD, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_LCD ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __PCINT,( __CONFIG_SYSTEM | __CONFIG_PCINT ) } ,
	{ __ONEWIRE, ( __CONFIG_SYSTEM | __CONFIG_ONEWIRE ) } ,
	
	// Port C
	{ __TWI, ( __CONFIG_SYSTEM | __CONFIG_TWI ) } ,
	{ __TWI, ( __CONFIG_SYSTEM | __CONFIG_TWI ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP ) } ,
	{ __LED, ( __CONFIG_SYSTEM | __CONFIG_LED ) } ,
	{ __LED, ( __CONFIG_SYSTEM | __CONFIG_LED ) } ,
	{ __MMC, ( __CONFIG_SYSTEM | __CONFIG_MMC ) } ,
	{ __SYSTEM, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,

	// Port D
	{ __UART, ( __CONFIG_SYSTEM | __CONFIG_UART ) } ,
	{ __UART, ( __CONFIG_SYSTEM | __CONFIG_UART ) } ,
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI ) } ,
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI ) } ,
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI ) } ,
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI ) } ,
	{ __PCINT, ( __CONFIG_SYSTEM | __CONFIG_PCINT ) } ,
	{ __PCINT, ( __CONFIG_SYSTEM | __CONFIG_PCINT ) } } ;

#elif defined(__AVR_ATmega644__) && defined(AVRNETIO)
HAL hal_table[] = {
	// Port A
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,

	// Port B
	{ __ONEWIRE, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __EXTINT, ( __CONFIG_SYSTEM | __CONFIG_EXTINT  ) } , // hier liegt der Interrupt vom ENC28j60
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI  ) } , // hier liegt der SPI-Bus vom ENC28j60
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI  ) } , // hier liegt der SPI-Bus vom ENC28j60
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI  ) } , // hier liegt der SPI-Bus vom ENC28j60
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI ) } , // hier liegt der SPI-Bus vom ENC28j60
	
	// Port C
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT  | __CONFIG_TWI ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT  | __CONFIG_TWI ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,

	// Port D
	{ __UART, ( __CONFIG_SYSTEM | __CONFIG_UART ) } ,
	{ __UART, ( __CONFIG_SYSTEM | __CONFIG_UART ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN |  __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_EXTINT ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_EXTINT ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } };

#elif defined(__AVR_ATmega2561__) && defined(OpenMCP)
HAL hal_table[] = {
	// Port A
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,
	{ __ADC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_ADC ) } ,

	// Port B
	{ __ONEWIRE, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_IN, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __EXTINT, ( __CONFIG_SYSTEM | __CONFIG_EXTINT  ) } , // hier liegt der Interrupt vom ENC28j60
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI  ) } , // hier liegt der SPI-Bus vom ENC28j60
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI  ) } , // hier liegt der SPI-Bus vom ENC28j60
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI  ) } , // hier liegt der SPI-Bus vom ENC28j60
	{ __SPI, ( __CONFIG_SYSTEM | __CONFIG_SPI ) } , // hier liegt der SPI-Bus vom ENC28j60
	
	// Port C
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT  | __CONFIG_TWI ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT  | __CONFIG_TWI ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,
	{ __GPIO_OUT, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT ) } ,

	// Port D
	{ __UART, ( __CONFIG_SYSTEM | __CONFIG_UART ) } ,
	{ __UART, ( __CONFIG_SYSTEM | __CONFIG_UART ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN |  __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_EXTINT ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_EXTINT ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } ,
	{ __MMC, ( __CONFIG_GPIO_IN | __CONFIG_GPIO_OUT | __CONFIG_GPIO_IN | __CONFIG_GPIO_IN_PULLUP | __CONFIG_ONEWIRE | __CONFIG_PCINT | __CONFIG_MMC ) } };
#else
	#error "HAL wird für diese Plattform nicht unterstützt."
#endif

void HAL_printconfig( void )
{
	
	int i;

	for ( i = 0 ; i < HAL_PINCOUNT ; i ++ )
	{
		printf_P( PSTR( "Pin%c%d: "),65 + (i/8), i%8);
		
		switch( hal_table[i].HAL_Config )
		{
			case __SYSTEM:			printf_P( PSTR("System"));
											break;
			case __GPIO_OUT: 		printf_P( PSTR("GPIO output"));
											break;
			case __GPIO_IN:  		printf_P( PSTR("GPIO input"));
											break;
			case __GPIO_IN_PULLUP:	printf_P( PSTR("GPIO input with Pullup"));
											break;
			case __ADC:				printf_P( PSTR("ADC"));
											break;
			case __TWI:				printf_P( PSTR("TWI"));
											break;
			case __ONEWIRE:			printf_P( PSTR("1-Wire"));
											break;
			case __PCINT:			printf_P( PSTR("Pin-Change Interrupt"));
											break;
			case __EXTINT:			printf_P( PSTR("Externe Interrupt"));
											break;
			case __UART:				printf_P( PSTR("RS232"));
											break;	
			case __SPI:				printf_P( PSTR("SPI-Bus"));
											break;
			case __LCD:				printf_P( PSTR("LCD-Display"));
											break;
			case __MMC:				printf_P( PSTR("MMC-Interface"));
											break;
			case __XRAM:			printf_P( PSTR("XRAM-Interface"));
											break;
			case __LED:				printf_P( PSTR("LED-Anzeige"));
											break;
			default:						printf_P( PSTR("unknown"));
											break;
		}
		printf_P( PSTR("\r\n"));
	}
}

		
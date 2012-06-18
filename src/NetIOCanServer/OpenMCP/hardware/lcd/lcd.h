/*!\file lcd.c \brief Stellt Funktionen für ein LCD bereit */
/***************************************************************************
 *            lcd.h
 *
 *  Thu Nov  5 17:02:56 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup hardware
///	\defgroup lcd Stellt Funktionen für ein LCD bereit (lcd.h)
//****************************************************************************/
//@{

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
 
 #ifndef _LCD_H
	#define LCD_H

	#include "config.h"

	#define LCD_2x16
//	#define LCD_4x16
	
	void LCD_init( void );
	void LCD_sendCMD( unsigned char CMD );
	void LCD_sendDATA( unsigned char DATA );
	void LCD_clrscreen( void );
	void LCD_setXY( char Xpos, char Ypos );
	void LCD_sendCHAR( char DATA );

#if defined(__AVR_ATmega644P__)
	#if defined(myAVR_LCD)
		#define 	E_PORT		PORTA
		#define		E_PIN		PA5
		#define 	E_DDR		DDRA
		#define 	RS_PORT		PORTA
		#define		RS_PIN		PA4
		#define 	RS_DDR		DDRA
	
		#define		DATAPORT_1	PORTA
		#define		DATADDR_1	DDRA
		#define		DATAMASK_1	0xC0
	
		#define		DATAPORT_2	PORTB
		#define		DATADDR_2	DDRB
		#define		DATAMASK_2	0x03
	#endif
#else
	#error "LCD wird für diese Hardwareplatform nicht unterstützt, bitte in der config.h abwählen!"
#endif

	#if defined(LCD_2x16)
		#define	LCD_Rows	2
	#endif
	#if defined(LCD_4x16)
		#define	LCD_Rows	4
	#endif
	#define LCD_Columns	16

	#define	LCD_ROW_1		0x00
	#define	LCD_ROW_2		0x40
	#define	LCD_ROW_3		0x10
	#define	LCD_ROW_4		0x50

	#define	LCD_CMD_CLR		0x01
	#define	LCD_CMD_HOME	0x02
	#define	LCD_CMD_ONOFF	0x0c
	#define	LCD_CMD_CURSOR	0x0a
	#define	LCD_CMD_CPOS	0x09
	#define	LCD_CMD_DDRAM	0x80
	#define	LCD_CMD_4BIT	0x20
	#define	LCD_CMD_2LINES	0x08
	#define	LCD_CMD_5x8		0x04
	#define LCD_CMD_DDRAM	0x80

#endif /* LCD_H */
//@}


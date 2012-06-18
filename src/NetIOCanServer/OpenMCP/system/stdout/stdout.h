/*! \file stdout.h \brief STDOUT-Funktion um die Ausgaben umzulenken */
//***************************************************************************
//*            stdout.h
//*
//*  Sat July  13 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup stdout Stdout Funktionen (stdout.h)
///	\par Uebersicht
//****************************************************************************/
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
//@{
#ifndef _STDOUT_H
	#define STDOUT_H

	typedef void ( * STDOUT_CALLBACK_FUNC ) ( void );

	void STDOUT_INIT( void );
	int STDOUT_Send_Byte ( char Byte, FILE * stream );
	int STDOUT_Get_Byte ( FILE * stream );

	int STDOUT_set( char TYPE, int DEVICE );
	int STDOUT_save( void * pStruct );
    int STDOUT_restore( void * pStruct );
	void STDOUT_Flush( void );
	void STDOUT_SetXY( char x, char y );


	/*! \struct STDOUT
	 * \brief Definiert die Struktur für den StandartOut. Hier werden alle wichtigen Daten zur Standartout gesichert, welche Verbindungsart u.s.w.
	 * und es wird der Puffer für die Daten verwaltet, das ist z.b. bei TCP wichtig, damit nicht wür jedes gesendete Byte ein komplettes Packet gesendet wird,
	 * das würde eine menge Traffic und Banbdbreite verschwenden.
	 */
	struct STDOUT {
		volatile char					TYPE;				/*!< Speichert den Verbindungtyp. */
		volatile unsigned int			DEVICE;				/*!< Speichert das Subdevice. */
		volatile unsigned char *		BUFFER;				/*!< Pointer auf den Puffer für Daten wenn benötig. */
		volatile int					BUFFER_POS;			/*!< Position im Puffer. */
		volatile int					XPOS;				/*!< Falls benötig X-Position. */
		volatile int					YPOS;				/*!< Falls benötig Y-Position. */
		volatile STDOUT_CALLBACK_FUNC 	SEND;
		volatile STDOUT_CALLBACK_FUNC 	GET;
	};

	#define UNKNOWN		-1
	#define NONE		0
	#define	RS232		1
	#define _TCP		2
	#define TAFEL		3
	#define _LCD			4
	#define	USER		5

#endif /* STDOUT_H */

 //@}

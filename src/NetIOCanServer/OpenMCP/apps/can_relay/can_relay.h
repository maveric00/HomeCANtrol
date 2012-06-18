/*! \file can_relay.h \brief Koppelt UDP-CAN-Messages mit dem CAN-Bus und umgekehrt */
/***************************************************************************
 *            can_relay.h
 *
 *  Mon Jun 19 21:56:05 2006
 *  Copyright  2006 Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
///	\ingroup software
///	\defgroup Koppelt UDP-CAN-Messages mit dem CAN-Bus und umgekehrt
///	\code #include "can_relay.h" \endcode
//****************************************************************************/
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
//@{
#ifndef _CAN_RELAY_H
	#define _CAN_RELAY_H

	#include <avr/pgmspace.h> 
	#include "hardware/CAN_Lib/can.h"

	#define CAN_RELAY_PORT 13247
	#define CAN_RELAY_BUFFER_LEN 20


	void can_relay_init( void );
	void can_relay_thread( void );


#endif /* _CAN_RELAY_H */
//@}

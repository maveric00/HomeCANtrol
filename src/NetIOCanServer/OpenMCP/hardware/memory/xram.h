/*! \file xram.h \brief Aktiviert das externe RAM-Interface */
//***************************************************************************
// *            tcp.h
// *
// *  Sat Jun  3 23:01:49 2006
// *  Copyright  2006  User
// *  Email
//****************************************************************************
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
#ifndef _XRAM_H
	#define XRAM_H

	void init_xram (void) __attribute__ ((naked)) __attribute__ ((section (".init2")));

#endif /* XRAM_H */

 

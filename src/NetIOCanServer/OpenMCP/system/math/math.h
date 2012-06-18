/*!\file math.h \brief Einige Extra Sachen */
//***************************************************************************
//*            math.h
//*
//*  Sun Jun 11 20:30:57 2006
//*  Copyright  2006  User
//*  Email
//***************************************************************************
///	\ingroup math
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
#ifndef __MATH_H__
	
	#define __MATH_H__

	unsigned long ChangeEndian32bit( unsigned long Wert);
	unsigned int ChangeEndian16bit( unsigned int Wert);
	
	// MIN/MAX/ABS macros
	#define MIN(a,b)			((a<b)?(a):(b))
	#define MAX(a,b)			((a>b)?(a):(b))
	#define ABS(x)				((x>0)?(x):(-x))

	// constants
	#define PI		3.14159265359

#endif
//@}

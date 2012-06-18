/*! \file config.h \brief Stellt Config-tools bereit */
//***************************************************************************
//*            config.h
//*
//*  Son Aug 10 16:25:49 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup hardware
///	\defgroup config Die Config (config.h)
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

#ifndef _CONFIG_H
	#define CONFIG_H
	
	void Config_Init( void );
	int findConfig( void );
	void makeConfig( void );
	int getConfigsizeUsed( void );
	void PrintConfig( void );

	int readConfig( char * ConfigName, char * ConfigValue );
	int writeConfig( char * ConfigName, char * ConfigValue );
	int changeConfig( char * ConfigName, char * ConfigValue );
	int deleteConfig( char * ConfigName);
	int checkConfigName( char * ConfigName );

	int readConfig_P( const char * ConfigName, char * ConfigValue );
	int writeConfig_P( const char * ConfigName, char * ConfigValue );
	int changeConfig_P( const char * ConfigName, char * ConfigValue );
	int deleteConfig_P( const char * ConfigName);
	int checkConfigName_P( const char * ConfigName );
	void setprotectConfig( int Protect );

	#define PROTECT		1
	#define UNPROTECT	0

	#define	configID	"MCPconfig"

    struct Config {
		char	TAG[ sizeof( configID ) ];
		int		Configlen;
	};

#endif /* CONFIG_H */

//@}

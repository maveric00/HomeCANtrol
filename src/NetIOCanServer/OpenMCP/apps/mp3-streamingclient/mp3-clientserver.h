/*! \file mp3-clientserver.h \brief Das Tsunami-Steuerinferface */
//***************************************************************************
//*            mp3-clientserver.h
//*
//*  Mon May 18:46:47 2008
//*  Copyright  2008  sharandac
//*  Email: sharandac@snafu.de
///	\ingroup hardware
///	\defgroup VS1001 Funktionen für den VS10xx bereit um zu streamen (mp3-clientserver.h)
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
 
#ifndef _MP3_CLIENTSERVER_H
	#define MP3_CLIENTSERVER_H

	#define HTML 	0
	#define TELNET	1

	void mp3clientcommand( unsigned char * buffer , unsigned int socket );
	void PlayURL( char * URL, unsigned int socket );
	void RePlayURL( unsigned int socket );
	void StopPlay( void );
	void PharseURL( unsigned char * URLBuffer );
	void mp3clientupdate( unsigned long bandwidth, unsigned int tcpbuffer, unsigned int streamingbuffer,  unsigned int decodetime, unsigned char verboselevel );
	void mp3clientupdateMETATITLE( char * streamingTITLE );
	void mp3clientupdateNAME( char * streamingNAME );
	void mp3clientupdateMETAURL( char * streamingURL );
	void mp3clientupdateGENRE( char * streamingGENRE );
	void mp3clientupdateBITRATE( int streamingBITRATE );
	void mp3clientPrintInfo( unsigned int SOCKET, char type );
	void mp3client_PrintHeaderinfo( void );

	struct MP3_STATS {
//		char 	trash;
		unsigned long 	streamingIP;
		char 			streamingPROTO[8];
		char 			streamingURL[128];
		unsigned int 	streamingPORT;
		char 			streamingFILE[128];
		char 			streamingNAME[256];
		char 			streamingMETATITLE[256];
		char			streamingMETAURL[128];
		char			streamingGENRE[128];
		int				streamingBITRATE;
		unsigned long	streamingrate;
		unsigned int	streamingtcpbuffer;
		unsigned int	streamingbuffer;
		unsigned long	streamingtime;
		unsigned char	streamingverboselevel;
	};

#endif /* MP3_CLIENTSERVER_H */

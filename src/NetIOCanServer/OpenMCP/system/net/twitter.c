/*! \file twitter.c \brief Stellt Funktionen für das twittern bereit. */
//***************************************************************************
//*            twitter.c
//*
//*  Son Aug 15 19:19:23 2009
//*  Copyright  2009  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup TWITTER Stellt Funktionen für das twittern bereit. (twitter.c)
///	\code #include "twitter.h" \endcode
///	\par Uebersicht
///		Stellt Funktionen für das twittern bereit.
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

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/stdout/stdout.h"
#include "system/base64/base64.h"
#include "system/net/dns.h"
#include "system/net/tcp.h"

#include "twitter.h"

const char 	TWITTERFILE[] PROGMEM = "/statuses/update.xml";
const char 	TWITTERURL[] PROGMEM = "twitter.com";
#define 	TWITTERPORT 	80

const char 	TWITTERHEADER_1[] PROGMEM =  "POST ";
const char 	TWITTERHEADER_2[] PROGMEM =  " HTTP/1.1\r\n"
										"Host: ";
const char 	TWITTERHEADER_3[] PROGMEM =  "\r\nAuthorization: Basic %s\r\n"
										"Content-Length: %d\r\n\r\n"
										"status=%s";

//****************************************************************************/
/*!\brief Sendet einen Twittereintrag.
 * \param tweet			Zeiger auf den String der gesendet werden soll.
 * \param userpw		Zeiger auf den String der Username und Password enthält.
 * \return				Erfolgreich oder nicht.
 * \retval 0			Erfolgreich gesendet.
 * \retval -1			Fehler beim senden.
 *
 * \code
 *   // Beispiel wie man einen Tweet sendet.
 *
 *  char TWEET[] = "foobar";
 *  char TWEETUSERASSWD[] = "user:pass";
 *
 *  if( TWITTER_sendtweet( TWEET, TWEETUSERPASSWD ) != 0 )
 *		printf_P( PSTR("Error"));
 *
 * \endcode
 */
//****************************************************************************/
int TWITTER_sendtweet( char * tweet, char * userpw )
{
	struct STDOUT oldstream;

	char userpw_b64[64];
	int ret=0, SOCKET;;
	long IP;	
	
	if ( strlen( tweet ) > 141 )
		return( TWITTER_FAILD );
	
	// TWITTERURL auflösen
	IP = DNS_ResolveName_P( TWITTERURL );
	if ( IP == -1 )
		return( TWITTER_FAILD );
		
	// zu twitter verbinden
	SOCKET = Connect2IP( IP, TWITTERPORT );
	if ( SOCKET == -1 )
		return( TWITTER_FAILD );
	
	// STDOUT umbiegen auf die neue Verbingung und alt STDOUT sichern
	STDOUT_save( &oldstream );
	STDOUT_set( _TCP, SOCKET );

	// Username und Passwort encoden
	base64_encode( userpw_b64, 64, userpw, strlen( userpw ) ); 
	
	// Request senden
	printf_P( TWITTERHEADER_1 );
    printf_P( TWITTERFILE );
    printf_P( TWITTERHEADER_2 );
	printf_P( TWITTERURL );
    printf_P( TWITTERHEADER_3, userpw_b64, 9+strlen(tweet) , tweet );
	printf_P( PSTR("\r\n") );
	
	// Gesicherte STDOUT wieder herstellen
	STDOUT_restore( &oldstream );

	CloseTCPSocket( SOCKET );

	// und zurück
	return( TWITTER_OK );
}

//****************************************************************************/
/*!\brief Sendet einen Twittereintrag.
 * \param tweet			Zeiger auf den String der im Flash liegt der gesendet werden soll.
 * \param userpw		Zeiger auf den String der im Flash liegt der Username und Password enthält.
 * \return				Erfolgreich oder nicht.
 * \retval 0			Erfolgreich gesendet.
 * \retval -1			Fehler beim senden.
 *
 * \code
 *   // Beispiel wie man einen Tweet sendet.
 *
 *  const char TWEET[] PROGMEM = "foobar";
 *  const char TWEETUSERASSWD[] PROGMEM = "user:pass";
 *
 *  if( TWITTER_sendtweet_P( TWEET, TWEETUSERPASSWD ) != 0 )
 *		printf_P( PSTR("Error"));
 *
 * \endcode
 *
 *  Oder
 *
 * \code
 *   // Beispiel wie man einen Tweet sendet.
 *
 *  if( TWITTER_sendtweet_P( PSTR("foobar") , PSTR("user:passwd") ) != 0 )
 *		printf_P( PSTR("Error"));
 *
 * \endcode
 */
//****************************************************************************/
int TWITTER_sendtweet_P( const char * tweet, const char * userpw )
{
	struct STDOUT oldstream;

	char userpw_[64];
	char userpw_b64[64];
	int ret = 0, SOCKET;
	long IP;	
	
	if ( strlen_P( tweet ) > 141 )
		return( TWITTER_FAILD );

	// TWITTERURL auflösen
	IP = DNS_ResolveName_P( TWITTERURL );
	if ( IP == -1 )
		return( TWITTER_FAILD );
		
	// zu twitter verbinden
	SOCKET = Connect2IP( IP, TWITTERPORT );
	if ( SOCKET == -1 )
		return( TWITTER_FAILD );
	
	// STDOUT umbiegen auf die neue Verbingung und alt STDOUT sichern
	STDOUT_save( &oldstream );
	STDOUT_set( _TCP, SOCKET );

	// Username und Passwort umkopieren für Base64
	strcpy_P( userpw_ , userpw );
	
	// Username und Passwort encoden
	base64_encode( userpw_b64, 64, userpw_ , strlen( userpw_ ) ); 
	
	// Request senden
	printf_P( TWITTERHEADER_1 );
    printf_P( TWITTERFILE );
    printf_P( TWITTERHEADER_2 );
	printf_P( TWITTERURL );
    printf_P( TWITTERHEADER_3, userpw_b64, 9+strlen_P(tweet) );
	printf_P( tweet );
	printf_P( PSTR("\r\n") );

	// Gesicherte STDOUT wieder herstellen
	STDOUT_restore( &oldstream );

	CloseTCPSocket( SOCKET );

	// und zurück
	return( TWITTER_OK );
}
//}@

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
 
#ifndef _HTTPD2_H
	#define HTTPD2_H

//	#define HTTP_DEBUG

	void httpd_init( void );
	void httpd_thread( void );

	#define	HTTP_PORT				80

	#define REQUEST_BUFFERLEN		192
	#define REQUEST_BUFFERLEN_FILE	( REQUEST_BUFFERLEN )
	#define REQUEST_BUFFERLEN_DATA	( REQUEST_BUFFERLEN )


	#define MAX_HTTP_PARAMS			12

	/*! \struct HTTP_REQUEST
	 *  \brief Definiert die Struktur in der Informationen zu einem HTTP-Request stehen.
	 */
	struct HTTP_REQUEST {
		char			GET_FILE[ ( REQUEST_BUFFERLEN_FILE ) + 1 ];		/*!< Speichert den Filenamen der aufgerufen werden soll. */
		char			GET_DATA[ ( REQUEST_BUFFERLEN_DATA ) + 1 ];		/*!< Speichert Variablennamen und deren Inhalt bei einen GET-Request wenn sie vorhanden sind. */
		char			HTTP_LINEBUFFER[ REQUEST_BUFFERLEN ];			/*!< Speichert eine komplette Zeile bei einen HTTP-Request. */
		char			STATE;											/*!< Definiert den Status der HTTP-Verbindung zu einem Client. 
																			 Mögliche Werte sind DISCONNET, CONNECTED, GETREUQEST_OK, REQUEST_END, ANSWER_SEND */
		char			REQUEST_TYPE;									/*!< Speichert den Typ des HTTP-Request. Typen sind GET_REQUEST oder POST_REQUEST. */
		int				REQUEST_LEN;
		char			argc;											/*!< Speichert die Anzahl der Variablen eines GET-Request. */
		char *			argname[ MAX_HTTP_PARAMS ];						/*!< Speichert die Pionter auf einen Variablennamen. */
		char *			argvalue[ MAX_HTTP_PARAMS ];					/*!< Speichert die Pionter auf den Inhalt der Variable analog zu argname. */
		int				HTTP_POS;										
		unsigned long	CLIENT_IP;
		unsigned int	HTTP_SOCKET;
	};

	#define GET_REQUEST			0
	#define POST_REQUEST		1

	#define DISCONNECT			0
	#define	CONNECTED			1
	#define GETREQUEST_OK		2
	#define REQUEST_END			3
	#define ANSWER_SEND			4

#endif /* HTTPD2_H */

 

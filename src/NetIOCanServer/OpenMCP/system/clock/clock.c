/*! \file clock.c \brief Stellt die CLOCK Funkionalitaet bereit */
//***************************************************************************
//*            clock.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup CLOCK Die Clockfunktion für den Microcontroller als Zeitbasis (clock.c)
///	\code #include "clock.h" \endcode
///	\par Uebersicht
///		Stellt funktionen bereit um eine genaue Zeit zu realisieren und funktionen
/// um Zeitgesteuert eigene Funktionen die man hinterlegt aufzurufen
/// \date	03-06-2008: Neuen Code hinzugefügt, geht jetzt schneller, die Callbacks und 
///			Counter werden nur einmal durchsucht pro Tick.
/// \date	04-06-2008: Lastverteilung in den Callbacks eingebaut, geht jetzt wunderbar, es wird pro Tick
///			nur eine Callback aufgerufen für Resolutions größer gleich Sekunde, die anderen werden
///			als Executionbit markiert und in den folgenden Tick ausgeführt.
/// \date	05-01-2008: Die Uhrzeit kann jetzt mit Hilfe eines Struct geholt werden, sollte in zukunft benutzt
///			werden da nicht mehr auf die Globalen Variablen zugegriffen werden muss.
/// \date	05-14-2008: Delayfunktion hinzugefügt.
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

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stddef.h>

#include "hardware/timer1/timer1.h"
#include "clock.h"
	
struct CALLBACK Clock_CallBack_Table [ MAX_CLOCK_CALLBACKS ]; 

struct COUNTER Counter_Table [ MAX_CLOCK_COUNTDOWNTIMER ];

struct TIME time;
/*-----------------------------------------------------------------------------------------------------------*/
/*! \brief Initialisiert die System-Clock und registriert ihn auf Timer1 mit 1/100s als Callbackfunktion.
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_init(void)
	{
		unsigned int i;
		
		// Alle Callbackeinträge löschen
		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ ) 
			{
				Clock_CallBack_Table[i].CallbackFunc = NULL;
				Clock_CallBack_Table[i].Resolution = NO_USE;
			}
			
		for ( i = 0 ; i < MAX_CLOCK_COUNTDOWNTIMER ; i++ ) 
			{
				Counter_Table[i].Counter = 0;
				Counter_Table[i].Resolution = NO_USE;
			}
			
		time.uptime=0;
		time.time=0;

		CLOCK_decode_time( time.time );
		
		// Clocksource init
		timer1_init( 100 , 0);
		// clock_tick registrieren als Callbackfunktion
		timer1_RegisterCallbackFunction( CLOCK_tick );
		
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*! \brief Die ISR der Clock, hier werden die Uhrzeit und Callbackeinträge abgearbeitet
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_tick( void )
	{			
		unsigned char state;
		unsigned int i;
		time.ms++;
		state = MSECOUND;
		if ( time.ms == 100 )
		{
			time.ms = 0;
			time.ss++;
			time.uptime++;
			time.time++;
			state = SECOUND;
			if ( time.ss == 60 )
			{
				time.ss = 0;
				time.mm++;
				state = MINUTE;
				if ( time.mm == 60 )
				{
					time.mm = 0;
					time.hh++;
					state = HOUR;
					CLOCK_decode_time( time.time );

					if ( time.hh == 24 )
					{
						time.hh = 0;
						state = DAY;
					}
				}
			}
		}
				
		// alle Callbacks durchgehen und alles was größer Resolution MSECOUND als Execution markieren für spätere ausführung
		for ( i = 0; i < MAX_CLOCK_CALLBACKS ; i++ )
		{
			if ( Clock_CallBack_Table[i].Resolution <= state )
			{
				if ( state >= SECOUND )
					Clock_CallBack_Table[i].Execution = Executionbit;
				else
				{
					Clock_CallBack_Table[i].Execution = NonExecutionbit;
					Clock_CallBack_Table[i].CallbackFunc();
				}
			}
		}

		// Hier werden die als Execution markierten Callbacks ausgeführt, je durchlauf einer
		for ( i = 0; i < MAX_CLOCK_CALLBACKS ; i++ )
		{
			if ( Clock_CallBack_Table[i].Execution == Executionbit )
			{
				Clock_CallBack_Table[i].Execution = NonExecutionbit;
				Clock_CallBack_Table[i].CallbackFunc();
				break;
			}
		}
		
		for ( i = 0 ; i < MAX_CLOCK_COUNTDOWNTIMER ; i++ )
		{
			if ( Counter_Table[i].Resolution <= state && Counter_Table[i].Counter != 0 )
			{
				Counter_Table[i].Counter--;
			}
		}
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert einen CountdownTimer.
 * \param	pFunc		Zeiger auf die Funktion die aufgerufen werden soll
 * \param	Resolution	Gibt die Auflösung an mit der die Callbackfunktion aufgerufen werden soll
 *						mögliche Paramter: MSECOUND,SECOUND,MINUTE,HOUR und DAY
 * \return	TRUE oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
char CLOCK_RegisterCallbackFunction( CLOCK_CALLBACK_FUNC pFunc, unsigned char Resolution )
	{
		unsigned char i;
		
		timer1_stop();

		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ ) 
		{
			if ( Clock_CallBack_Table[i].CallbackFunc == pFunc )
			{
				timer1_free();
				return CLOCK_OK;
			}
		}
		
		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
		{
			if ( Clock_CallBack_Table[i].CallbackFunc == NULL )
			{
				// zuerst Pointer zu Funktion setzen und Resolution setzen zum scharf machen
				// dies ist nötig da der zugrif auf Resolution atomar ist
				Clock_CallBack_Table[i].CallbackFunc = pFunc;
				Clock_CallBack_Table[i].Resolution = Resolution;
				timer1_free();
				return CLOCK_OK;
			}
		}
		timer1_free();
		return CLOCK_FAILED;
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Löscht einen registrierte Callbackfunktion.
 * \param	pFunc		Zeiger auf die Funktion die aufgerufen werden soll
 * \return	TRUE oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
char CLOCK_RemoveCallbackFunction( CLOCK_CALLBACK_FUNC pFunc )
	{
		unsigned char counter;
		
		timer1_stop();

		for ( counter = 0 ; counter < MAX_CLOCK_CALLBACKS ; counter++ )
		{
			if ( Clock_CallBack_Table[ counter ].CallbackFunc == pFunc )
			{
				// zuerst Resolution löschen zum sperren und dann Pointer löschen
				// dies ist nötig da der zugrif auf Resolution atomar ist
				Clock_CallBack_Table[ counter ].Resolution = NO_USE;
				Clock_CallBack_Table[ counter ].CallbackFunc = NULL;
				timer1_free();
				return CLOCK_OK;
			}
		}
		timer1_free();
		return CLOCK_FAILED;
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert einen Counterdown-zähler.
 * \code
 * unsigned char timer;
 *
 * #define TIMEOUT 1000
 *		
 * timer = CLOCK_RegisterCoundowntimer();
 * CLOCK_SetCountdownTimer( timer , TIMEOUT, MSEC );
 *
 * while ( 1 )
 * {
 *		if ( Get_Key() && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
 *		{
 *			CLOCK_ReleaseCountdownTimer( timer );
 *			return( Okay );
 *		}
 *		if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
 *		{
 *			CLOCK_ReleaseCountdownTimer( timer );
 *			return( Error );
 *		}
 * }
 * \endcode
 * \return	Die Nummer des Countdowntimer oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
int CLOCK_RegisterCoundowntimer( void )
	{
		int counter ;
		
		timer1_stop();
		
		for ( counter = 0 ; counter < MAX_CLOCK_COUNTDOWNTIMER ; counter++ )
		{
			if ( Counter_Table[ counter ].Resolution == NO_USE )
			{
				Counter_Table[ counter ].Counter = 0;
				timer1_free();
				return ( counter ) ;
			}
		}

		timer1_free();

		return( CLOCK_FAILED );		
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Set einen startwert für einen Counterdown-zähler.
 * \param	counter		Die Counternummer der benutzt werden soll, dieser sollte vorher mit CLOCK_RegisterCoundowntimer
 *						ermittelt worden sein.
 * \param	value		Der Wert ab den gegen 0 gezählt werden soll.
 * \param	Resolution	Gibt die Auflösung an mit der die Callbackfunktion aufgerufen werden soll
 *						mögliche Paramter: MSECOUND,SECOUND,MINUTE,HOUR und DAY
 * \return	Die Nummer des Countdowntimer oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_SetCountdownTimer( int counter, unsigned int value, unsigned char Resolution )
	{
		timer1_stop();

		Counter_Table[ counter ].Counter = value;
		Counter_Table[ counter ].Resolution = Resolution;

		timer1_free();
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt den aktuellen Zählerstand für einen Counterdown-zähler.
 * \param	counter		Die Counternummer der benutzt werden soll, dieser sollte vorher mit CLOCK_RegisterCoundowntimer
 *						ermittelt worden sein.
 * \return	Value		Der Zählerstand
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int CLOCK_GetCountdownTimer( int counter )
	{
		unsigned int value;

		timer1_stop();

		value = Counter_Table[ counter ].Counter;

		timer1_free();

		return ( value );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt einen Counter wieder zur Benutzung frei.
 * \param	counter		Die Counternummer die benutzt werden soll, dieser sollte vorher mit CLOCK_RegisterCoundowntimer
 *						ermittelt worden sein.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
void CLOCK_ReleaseCountdownTimer( int counter )
	{
		Counter_Table[ counter ].Resolution = NO_USE ;
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Aktuelle Uhrzeit und speichert sie in der übergebenen Struktur.
 * \param	Time		Pointer auf die Struct in der die Uhrzeit abgelegt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
int CLOCK_GetTime( void * Time )
	{
		struct TIME * Timestruct;
		Timestruct = (struct TIME *) Time;
		
		timer1_stop();
		
		Timestruct->ms = time.ms;
		Timestruct->ss = time.ss;
		Timestruct->mm = time.mm;
		Timestruct->hh = time.hh;
		Timestruct->WW = time.WW;
		Timestruct->DD = time.DD;
		Timestruct->MM = time.MM;
		Timestruct->YY = time.YY;
		Timestruct->uptime = time.uptime;
		Timestruct->time = time.time;
		
		timer1_free();
		
		return( 0 );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Aktuelle Uhrzeit und speichert sie in der übergebenen Struktur.
 * \param	Time		Pointer auf die Struct in der die Uhrzeit abgelegt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
int CLOCK_SetTime( void * Time )
	{
		struct TIME * Timestruct;
		Timestruct = (struct TIME *) Time;
		
		timer1_stop();
		
		time.ms = Timestruct->ms;
		time.ss = Timestruct->ss;
		time.mm = Timestruct->mm;
		time.hh = Timestruct->hh;
		time.WW = Timestruct->WW;
		time.DD = Timestruct->DD;
		time.MM = Timestruct->MM;
		time.YY = Timestruct->YY;
		time.uptime = Timestruct->uptime;
		time.time = Timestruct->time;
		
		timer1_free();
		
		return( 0 );
	}
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Delay was sonst.
 * \param	ms			Warte einfach eine Zeit in ms.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_delay(unsigned int ms) 
{
	unsigned char counter;
		
	counter = CLOCK_RegisterCoundowntimer();
	CLOCK_SetCountdownTimer( counter , ms/10 , MSECOUND );

	while ( 1 )
		{
			if ( CLOCK_GetCountdownTimer( counter ) == 0 ) 
			{
				CLOCK_ReleaseCountdownTimer( counter );
				return;
			}
		}	
}

PROGMEM char wday_str[] = "So\0Mo\0Di\0Mi\0Do\0Fr\0Sa";

/************************************************************************
Beschreibung:    monthlen
Inputs: isleapyear = 0-1, month=0-11
Return:  Number of days per month
*************************************************************************/
unsigned char CLOCK_monthlen(unsigned char isleapyear,unsigned char month)
{
    if (month == 1)
    {
        return (28+isleapyear);
    }

    if (month > 6)
    {
        month--;
    }

    if (month %2 == 1)
    {
        return (30);
    }

    return (31);
}

/************************************************************************
Beschreibung:    decode_time
decodes the time into the datetime_t struct
Return:        
*************************************************************************/
void CLOCK_decode_time( unsigned long ntp_time )
{
    unsigned long dayclock;
    unsigned int  dayno;
    unsigned char summertime;

	time.time = ntp_time;
	
    time.YY = EPOCH_YR; //=1970
    dayclock = ( ntp_time ) % SECS_DAY;
    dayno    = ( ntp_time ) / SECS_DAY;

    time.ss = dayclock % 60UL;
    time.mm = (dayclock % 3600UL) / 60;
    time.hh   = dayclock / 3600UL;
    time.WW   = (dayno + 4) % 7;      // day 0 was a thursday

    while (dayno >= YEARSIZE( time.YY ) )
    {
        dayno -= YEARSIZE( time.YY );
        time.YY++;
    }

    time.MM = 0;
    while ( dayno >= CLOCK_monthlen( LEAPYEAR( time.YY ) , time.MM ) )
    {
        dayno -= CLOCK_monthlen(LEAPYEAR( time.YY ) , time.MM );
        time.MM++;
    }
    time.MM++;
    time.DD  = dayno+1;

    // Summertime
    summertime = 1;
    if ( time.MM < 3 || time.MM > 10)     // month 1, 2, 11, 12
    {
        summertime = 0;                          // -> Winter
    }

    if ( ( time.DD - time.WW >= 25 ) && ( time.WW || time.hh >= 2) )
    {                              // after last Sunday 2:00
        if ( time.MM == 10 )        // October -> Winter
        {
            summertime = 0;
        }
    }
    else
    {                              // before last Sunday 2:00
        if ( time.MM == 3)        // March -> Winter
        {
            summertime = 0;
        }
    }

    if (summertime)
    {
        time.hh++;              // add one hour
        if ( time.hh == 24)
        {                        // next day
            time.hh = 0;
            time.WW++;            // next weekday
            if ( time.WW == 7 )
            {
                time.WW = 0;
            }
            if ( time.DD == CLOCK_monthlen( LEAPYEAR( time.YY ) , time.MM ) )
            {                // next month
                time.DD = 0;
                time.MM++;
            }
            time.DD++;
        }
    }
}

//@}

/*
 Utils.h: Utility-Funktionen
 */

#ifndef	UTILS_H
#define	UTILS_H

// ----------------------------------------------------------------------------

#ifndef	TRUE
	#define	TRUE	(1==1)
#endif

#ifndef FALSE
	#define	FALSE	(1!=1)
#endif

#ifndef NULL
	#define NULL ((void*)0)		
#endif


#define	LOW_BYTE(x)		((uint8_t) (x & 0xff))
#define	HIGH_BYTE(x)	((uint8_t) (x >> 8))

#define	_bit_is_set(pin, bit)	(pin & (1<<bit))
#define	_bit_is_clear(pin, bit)	(!(pin & (1<<bit)))

/*
 * Die Makros RESET(), SET(), SET_OUTPUT(), SET_INPUT() und IS_SET()
 * beziehen sich immer auf ein bestimmtes Bit eines Ports und helfen somit
 * den Code sehr portabel zu gestalten.
 *
 * Beispiel:
 * 
 * #define LED   D,5		// PORTD, Pin 5
 * SET_OUTPUT(LED);		// Pin als Ausgang schalten (wird z.B. zu DDRD |= (1<<5);)
 * SET(LED);			// LED aktivieren
 */

#define	PORT(x)			_port2(x)
#define	DDR(x)			_ddr2(x)
#define	PIN(x)			_pin2(x)
#define	REG(x)			_reg(x)
#define	PIN_NUM(x)		_pin_num(x)

#define	RESET(x)		RESET2(x)
#define	SET(x)			SET2(x)
#define	TOGGLE(x)		TOGGLE2(x)
#define	SET_OUTPUT(x)	SET_OUTPUT2(x)
#define	SET_INPUT(x)	SET_INPUT2(x)
#define	SET_PULLUP(x)	SET2(x)
#define	IS_SET(x)		IS_SET2(x)

#define	SET_INPUT_WITH_PULLUP(x)	SET_INPUT_WITH_PULLUP2(x)

#define	_port2(x)	PORT ## x
#define	_ddr2(x)	DDR ## x
#define	_pin2(x)	PIN ## x

#define	_reg(x,y)		x
#define	_pin_num(x,y)	y

#define	RESET2(x,y)		PORT(x) &= ~(1<<y)
#define	SET2(x,y)		PORT(x) |= (1<<y)
#define	TOGGLE2(x,y)	PORT(x) ^= (1<<y)

#define	SET_OUTPUT2(x,y)	DDR(x) |= (1<<y)
#define	SET_INPUT2(x,y)		DDR(x) &= ~(1<<y)
#define	SET_INPUT_WITH_PULLUP2(x,y)	SET_INPUT2(x,y);SET2(x,y)

#define	IS_SET2(x,y)	((PIN(x) & (1<<y)) != 0)
#endif	// UTILS_H


#include <avr/pgmspace.h>
#include <util/delay.h>

#include "config.h"
#include "utils.h"
#include "mcp2515.h"
#include "mcp2515_defs.h"

#ifndef	MCP2515_CLKOUT_PRESCALER
	#error	MCP2515_CLKOUT_PRESCALER not defined!
#elif MCP2515_CLKOUT_PRESCALER == 0
	#define	CLKOUT_PRESCALER_	0x0
#elif MCP2515_CLKOUT_PRESCALER == 1
	#define	CLKOUT_PRESCALER_	0x4
#elif MCP2515_CLKOUT_PRESCALER == 2
	#define	CLKOUT_PRESCALER_	0x5
#elif MCP2515_CLKOUT_PRESCALER == 4
	#define	CLKOUT_PRESCALER_	0x6
#elif MCP2515_CLKOUT_PRESCALER == 8
	#define	CLKOUT_PRESCALER_	0x7
#else
	#error	invaild value of MCP2515_CLKOUT_PRESCALER
#endif

#ifndef	MCP2515_BITRATE
	#error	MCP2515_BITRATE not defined!
#else
	#if	MCP2515_BITRATE == 125
		// 125 kbps
		#define	R_CNF3	((1<<PHSEG21))
		#define	R_CNF2	((1<<BTLMODE)|(1<<PHSEG11))
		#define	R_CNF1	((1<<BRP2)|(1<<BRP1)|(1<<BRP0))
	#elif MCP2515_BITRATE == 250
		// 250 kbps
		#define	R_CNF3	((1<<PHSEG21))
		#define	R_CNF2	((1<<BTLMODE)|(1<<PHSEG11))
		#define	R_CNF1	((1<<BRP1)|(1<<BRP0))
	#elif MCP2515_BITRATE == 500
		// 500 kbps
		#define	R_CNF3	((1<<PHSEG21))
		#define	R_CNF2	((1<<BTLMODE)|(1<<PHSEG11))
		#define	R_CNF1	((1<<BRP0))
	#elif MCP2515_BITRATE == 1000
		// 1 Mbps
		#define	R_CNF3	((1<<PHSEG21))
		#define	R_CNF2	((1<<BTLMODE)|(1<<PHSEG11))
		#define	R_CNF1	(0)
	#else
		#error invalid value for MCP2515_BITRATE
	#endif
#endif

#define MCP2515_FILTER_EXTENDED(id)	\
		(uint8_t)  ((uint32_t) (id) >> 21), \
		(uint8_t)((((uint32_t) (id) >> 13) & 0xe0) | (1<<3) | \
			(((uint32_t) (id) >> 16) & 0x3)), \
		(uint8_t)  ((uint32_t) (id) >> 8), \
		(uint8_t)  ((uint32_t) (id))

// -------------------------------------------------------------------------
uint8_t mcp2515_register_map[45] = {
	MCP2515_FILTER_EXTENDED(0x00001000),	// Filter 0
	MCP2515_FILTER_EXTENDED(0),		// Filter 1
	MCP2515_FILTER_EXTENDED(0),		// Filter 2
	0,									// BFPCTRL
	0,									// TXRTSCTRL
	0,									// CANSTAT (read-only)
	(1<<REQOP2) | CLKOUT_PRESCALER_,	// CANCTRL
	MCP2515_FILTER_EXTENDED(0),		// Filter 3
	MCP2515_FILTER_EXTENDED(0),		// Filter 4
	MCP2515_FILTER_EXTENDED(0),		// Filter 5
	0,									// TEC (read-only)
	0,									// REC (read-only)
	0,									// CANSTAT (read-only)
	(1<<REQOP2) | CLKOUT_PRESCALER_,	// CANCTRL
	MCP2515_FILTER_EXTENDED(0x3fff),// Mask 0 (for group 0)
	MCP2515_FILTER_EXTENDED(0x1fffffff),		// Mask 1 (for group 1)
	R_CNF3,
	R_CNF2,
	R_CNF1,
	MCP2515_INTERRUPTS,
	0							// clear interrupt flags
};

// ----------------------------------------------------------------------------
// abgespeckte Variante der "normalen" MCP2515 Initialisierung

// ----------------------------------------------------------------------------
uint8_t spi_putc(uint8_t data)
{

	uint8_t data_in = 0;
	uint8_t data_b ;
	
	data_b = data ;
	
	RESET(P_SCK);
	for (uint8_t i=0;i<8;i++)
	{
		data_in <<= 1;
	
		if (data & 0x80) {
			SET(P_MOSI);
		} else {
			RESET(P_MOSI);
		} ;
		SET(P_SCK);
		if (IS_SET(P_MISO)) {
			data_in |= 1;	
		} ;
		data <<= 1;
		RESET(P_SCK);
	}
	return data_in;

}

void mcp2515_write_register(uint8_t adress, uint8_t data )
{
	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE);
	spi_putc(adress);
	spi_putc(data);
	SET(MCP2515_CS);
}

uint8_t mcp2515_read_status(uint8_t type)
{
	uint8_t data;
	
	RESET(MCP2515_CS);
	
	spi_putc(type);
	data = spi_putc(0xff);
	
	SET(MCP2515_CS);
	
	return data;
}

int8_t mcp2515_read_register(uint8_t adress)
{
	uint8_t data;
	
	RESET(MCP2515_CS);
	
	spi_putc(SPI_READ);
	spi_putc(adress);
	
	data = spi_putc(0xff);	
	
	SET(MCP2515_CS);
	
	return data;
}

/* Nur extended ID wird unterstuetzt, auch beim Empfangen */

uint8_t mcp2515_read_id(uint32_t *id)
{
	uint8_t first;
	uint8_t tmp;
	uint8_t spi_temp;
	
	first = spi_putc(0xff);
	tmp   = spi_putc(0xff);
	
	if (tmp & (1 << IDE)) {
		spi_temp = spi_putc(0xff);

		*((uint16_t *) id + 1)  = (uint16_t) first << 5;
		*((uint8_t *)  id + 1)  = spi_temp;

		spi_temp = spi_putc(0xff);
		
		*((uint8_t *)  id + 2) |= (tmp >> 3) & 0x1C;
		*((uint8_t *)  id + 2) |=  tmp & 0x03;
		
		*((uint8_t *)  id)      = spi_temp;
		
		return (1);
	}
	return (0) ;
}

void mcp2515_write_id(const uint32_t *id)
{
	uint8_t tmp;
	
	spi_putc(*((uint16_t *) id + 1) >> 5);
	
	// naechsten Werte berechnen
	tmp  = (*((uint8_t *) id + 2) << 3) & 0xe0;
	tmp |= (1 << IDE);
	tmp |= (*((uint8_t *) id + 2)) & 0x03;
	
	// restliche Werte schreiben
	spi_putc(tmp);
	spi_putc(*((uint8_t *) id + 1));
	spi_putc(*((uint8_t *) id));
}


uint8_t mcp2515_send_message(const can_t *msg)
{
	uint8_t length ;
#ifdef NO_CAN
	return ;
#endif


	// Warten, bis Sendbuffer bereit ist; es wird nur der erste Sendbuffer verwendet

	while (mcp2515_read_status(SPI_READ_STATUS)&(1<<2)) ;
	
	RESET(MCP2515_CS);

	spi_putc(SPI_WRITE_TX);
	
	mcp2515_write_id(&msg->id);

	length = msg->length & 0x0f;
	
	// Nachrichten Laenge einstellen
	spi_putc(length);
		
	// Daten
	for (uint8_t i=0;i<length;i++) {
		spi_putc(msg->data[i]);
	}
	SET(MCP2515_CS);
	
	asm volatile(	"nop\n\t"
			"nop\n\t"
			::);
	
	// CAN Nachricht verschicken
	// die letzten drei Bit im RTS Kommando geben an welcher
	// Puffer gesendet werden soll.
	RESET(MCP2515_CS);
	spi_putc(SPI_RTS | 1);
	SET(MCP2515_CS);
	
	return (1);
}




void mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data)
{
	RESET(MCP2515_CS);
	
	spi_putc(SPI_BIT_MODIFY);
	spi_putc(adress);
	spi_putc(mask);
	spi_putc(data);
	
	SET(MCP2515_CS);
}


uint8_t mcp2515_get_message(can_t *msg)
{
	uint8_t addr;
	uint8_t status ;
	uint8_t tmp ;
	uint8_t length ;

#ifdef NO_CAN
	return ;
#endif
	
	// read status
	status = mcp2515_read_status(SPI_RX_STATUS);
		
	if (_bit_is_set(status,6)) {
		// message in buffer 0
		addr = SPI_READ_RX;
	}
	else if (_bit_is_set(status,7)) {
	// message in buffer 1
		addr = SPI_READ_RX | 0x04;
	}
	else {
		// Error: no message available
		return 0x3f;
	}
	
	RESET(MCP2515_CS);
	spi_putc(addr);
	
	// CAN ID auslesen und ueberpruefen
	tmp = mcp2515_read_id(&msg->id);
	msg->flags.extended = tmp & 0x01;
	
	// read DLC
	length = spi_putc(0xff);
	msg->flags.rtr = (_bit_is_set(status, 3)) ? 1 : 0;
	
	length &= 0x0f;
	msg->length = length;
	// read data
	for (uint8_t i=0;i<length;i++) {
		msg->data[i] = spi_putc(0xff);
	}
	SET(MCP2515_CS);
	
	// clear interrupt flag
	if (_bit_is_set(status, 6))
		mcp2515_bit_modify(CANINTF, (1<<RX0IF), 0);
	else
		mcp2515_bit_modify(CANINTF, (1<<RX1IF), 0);
	
	return (status & 0x07) + 1;
}


void mcp2515_init(void)
{
	// Aktivieren der Pins fuer das SPI Interface
#ifdef NO_CAN
	return ;
#endif
	PORT_SPI &=  ~((1 << PIN_NUM(P_SCK)) | (1 << PIN_NUM(P_MOSI)));
	DDR_SPI |= (1 << PIN_NUM(P_SCK)) | (1 << PIN_NUM(P_MOSI));

	RESET(P_SCK);
	RESET(P_MOSI);
	SET(MCP2515_CS);
	SET_OUTPUT(P_SCK);
	SET_OUTPUT(P_MOSI);	
	SET_OUTPUT(MCP2515_CS);
	SET_INPUT(P_MISO);
	
	delay_ms(1);
	
	// MCP2515 per Software Reset zuruecksetzten,
	// danach ist er automatisch im Konfigurations Modus
	RESET(MCP2515_CS);
	spi_putc(SPI_RESET);
	SET(MCP2515_CS);
	
	// ein bisschen warten bis der MCP2515 sich neu gestartet hat
	delay_ms(1);
	
	// Filter usw. setzen
	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE);
	spi_putc(RXF0SIDH);
	for (uint8_t i = 0; i < sizeof(mcp2515_register_map); i++)
		spi_putc(mcp2515_register_map[i]);
	SET(MCP2515_CS);
	
	// nur Extended IDs, Message Rollover nach Puffer 1
	mcp2515_write_register(RXB0CTRL, (1<<RXM1)|(0<<RXM0)|(1<<BUKT));
	mcp2515_write_register(RXB1CTRL, (1<<RXM1)|(0<<RXM0));
	
	// MCP2515 zurueck in den normalen Modus versetzten
	mcp2515_write_register(CANCTRL, CLKOUT_PRESCALER_);
	while ((mcp2515_read_register(CANSTAT) & 0xe0) != 0);

}

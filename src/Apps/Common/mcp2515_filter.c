#include <avr/io.h>
#include "mcp2515.h"
#include "utils.h"
#include "spi.h"

// Filter setzen

uint8_t mcp2515_set_filter(uint8_t number, const can_filter_t *filter)
{
	uint8_t mask_address = 0;
	
	if (number > 5)
		return false;
	
	// Konfiguration einschalten
	mcp2515_change_operation_mode( (1<<REQOP2) );
	
	// Filtermaske setzen
	if (number == 0)
	{
		mask_address = RXM0SIDH;	
	}
	else if (number == 2)
	{
		mask_address = RXM1SIDH;
	}
	
	if (mask_address)
	{
		RESET(MCP2515_CS);
		spi_putc(SPI_WRITE);
		spi_putc(mask_address);
		mcp2515_write_id(&filter->mask);

		asm volatile ("nop");
		asm volatile ("nop");
		
		SET(MCP2515_CS);
		
		asm volatile ("nop");
		asm volatile ("nop");

	}
	
	// Filter setzen
	uint8_t filter_address;
	if (number >= 3) {
		number -= 3;
		filter_address = RXF3SIDH;
	}
	else {
		filter_address = RXF0SIDH;
	}
	
	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE);
	spi_putc(filter_address | (number * 4));
	mcp2515_write_id(&filter->id);

	asm volatile ("nop");
	asm volatile ("nop");

	SET(MCP2515_CS);
	
	asm volatile ("nop");
	asm volatile ("nop");

	if (number == 0)
	{
		mcp2515_write_register(RXB0CTRL, (1<<RXM1));
	} 
	else if (number==2)
	{
		mcp2515_write_register(RXB1CTRL, (1<<RXM1));
	} ;
	
	// zurueck zum normalen modus
	mcp2515_write_register(CANCTRL, 0);
	
	return true;
}


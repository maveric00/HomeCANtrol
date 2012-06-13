#include "mcp2515.h"
#include "utils.h"

#include "spi.h"

// ----------------------------------------------------------------------------
void mcp2515_spi_init(void)
{
	// Aktivieren der Pins fuer das SPI Interface
	SET(MCP2515_CS);
	SET_OUTPUT(MCP2515_CS);
	
	// Aktivieren der Pins fuer das SPI Interface
	RESET(P_SCK);
	RESET(P_MOSI);
	RESET(P_MISO);
	SET_OUTPUT(P_SCK);
	SET_OUTPUT(P_MOSI);
	SET_INPUT(P_MISO);
}

// ----------------------------------------------------------------------------
// Schreibt/liest ein Byte ueber den Hardware SPI Bus

uint8_t spi_putc(uint8_t data)
{

	uint8_t data_in = 0;
	uint8_t data_b ;
	
	data_b = data ;
	
	RESET(P_SCK);
	for (uint8_t i=0;i<8;i++)
	{
		data_in <<= 1;
	
		if (data & 0x80)
			SET(P_MOSI);
		else
			RESET(P_MOSI);
		SET(P_SCK);
		if (IS_SET(P_MISO))
		data_in |= 1;
		data <<= 1;
		RESET(P_SCK);
	}
	return data_in;

}

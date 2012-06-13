#ifndef	SPI_H
#define	SPI_H

//Initialisiere SPI 

extern void mcp2515_spi_init(void);
// Lese bzw. schreibe ein Byte auf den SPI 


extern uint8_t spi_putc(uint8_t data);

// ----------------------------------------------------------------------------


#endif	// SPI_H

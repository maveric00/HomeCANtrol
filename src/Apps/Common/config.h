#ifndef	CONFIG_H
#define	CONFIG_H

// -----------------------------------------------------------------------------
// CAN Settings


#ifndef	MCP2515_BITRATE
	#define	MCP2515_BITRATE		250
#endif

#ifndef	MCP2515_INTERRUPTS
	#define	MCP2515_INTERRUPTS	(1<<RX1IE)|(1<<RX0IE)
#endif

#ifndef	MCP2515_CLKOUT_PRESCALER
	#define	MCP2515_CLKOUT_PRESCALER	0
#endif




#endif	// CONFIG_H


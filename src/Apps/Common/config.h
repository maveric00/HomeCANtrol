#ifndef	CONFIG_H
#define	CONFIG_H

// -----------------------------------------------------------------------------
// CAN Settings


#define	MCP2515_BITRATE		250

#ifndef	MCP2515_INTERRUPTS
	#define	MCP2515_INTERRUPTS	(1<<RX1IE)|(1<<RX0IE)
#endif




#endif	// CONFIG_H


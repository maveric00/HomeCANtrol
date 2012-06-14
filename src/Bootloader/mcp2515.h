#ifndef	SIMPLE_MCP2515_H
#define	SIMPLE_MCP2515_H
#include <inttypes.h>


typedef struct
{
	uint32_t id;				// ID der Nachricht (11 oder 29 Bit)
	struct {
		int rtr : 1;			// Remote-Transmit-Request-Frame?
		int extended : 1;		// extended ID?
	} flags;
	
	uint8_t length;				// Anzahl der Datenbytes
	uint8_t data[8];			// Die Daten der CAN Nachricht
	
} can_t;

extern can_t InMessage ;
extern can_t OutMessage ;
extern uint8_t message_number;			// Running number of the messages
extern uint8_t message_data_counter;	
extern uint8_t message_data_length;		// Length of the data-field
extern uint8_t message_data[4];

extern uint8_t mcp2515_register_map[45] ;

typedef enum {
	// every bootloader type has this commands
	
	UPDATE_REQ      = 1,
	IDENTIFY		= 2,
	SET_ADDRESS		= 3,
	DATA			= 4,
	START_APP		= 5,
		
	REQUEST					= 0x00,
	SUCCESSFULL_RESPONSE	= 0x40,
	ERROR_RESPONSE			= 0x80,
	WRONG_NUMBER_REPSONSE	= 0xC0,
	
	NO_MESSAGE		= 0x3f
} tCommand;

#define	COMMAND_MASK			0x3F
#define	START_OF_MESSAGE_MASK	0x80

// ----------------------------------------------------------------------------
extern void mcp2515_init(void);

// ----------------------------------------------------------------------------
extern void send_message( uint8_t type, uint8_t length );

// ----------------------------------------------------------------------------
extern uint8_t get_message( void );
extern uint8_t mcp2515_get_message(can_t *msg);
extern uint8_t mcp2515_send_message(const can_t *msg) ;
extern uint8_t mcp2515_read_status(uint8_t) ;
extern void delay_ms(uint16_t __ticks) ;

#endif	// SIMPLE_MCP2515_H

#ifndef __MAIN_H
#define __MAIN_H

typedef enum {
	// every bootloader type has this commands
	
        UPDATE_REQ              = 1,
	IDENTIFY		= 2,
	SET_ADDRESS		= 3,
	DATA			= 4,
	START_APP		= 5,
		
	REQUEST			= 0x00,
	SUCCESSFULL_RESPONSE	= 0x40,
	ERROR_RESPONSE	        = 0x80,
	WRONG_NUMBER_REPSONSE	= 0xC0,
	
	NO_MESSAGE		= 0x3f
} tCommand;

#define	COMMAND_MASK			0x3F
#define	START_OF_MESSAGE_MASK	0x80


#endif /* __MAIN_H */

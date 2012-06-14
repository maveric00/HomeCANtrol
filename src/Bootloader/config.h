#ifndef	CONFIG_H
#define	CONFIG_H

// -----------------------------------------------------------------------------
// Bootloader Settings
//
// set current version of the bootloader

#define	BOOTLOADER_VERSION		3

// Unterscheidung Tiny / Mega
#if defined(__AVR_ATtiny84__)
#define RESET_ADDR		    0x0000
#define BOOTLOADER_STARTADDRESS     0x1800                        // bootloader start address, e.g. 0x1800 = 6144
#define RJMP                        (0xC000U - 1)                 // opcode of RJMP minus offset 1
#define RESET_SECTION               __attribute__((section(".bootreset")))
#define BOOTLOADER_STARTADDRESS     0x1800                        // bootloader start address, e.g. 0x1800 = 6144
#define BOOTLOADER_ENDADDRESS       0x2000                        // bootloader end   address, e.g. 0x2000 = 8192
#define BOOTLOADER_FUNC_ADDRESS (BOOTLOADER_ENDADDRESS - sizeof (BOOTLOADER_FUNCTIONS))
 
typedef struct
{
    void                        (*start_appl_main) (void);
} BOOTLOADER_FUNCTIONS; 
#endif

// -----------------------------------------------------------------------------
// CAN PIN Settings

#if defined (__AVR_AT90PWM3B__)
	#define P_MOSI D,3
	#define P_MISO D,2
	#define P_SCK  D,4
	#define	MCP2515_CS  D,1
	#define	MCP2515_INT B,2
	#define PORT_SPI	PORTD
	#define DDR_SPI		DDRD

#else
	#define P_MOSI A,6
	#define P_MISO A,5
	#define P_SCK  A,4
	#define	MCP2515_CS  A,7
	#define	MCP2515_INT A,3
	#define PORT_SPI	PORTA
	#define DDR_SPI		DDRA
#endif


// ----------------------------------------------------------------------------
// create pagesize identifier

#if	SPM_PAGESIZE == 32
	#define	PAGESIZE_IDENTIFIER		(0)
#elif SPM_PAGESIZE == 64
	#define	PAGESIZE_IDENTIFIER		(1)
#elif SPM_PAGESIZE == 128
	#define	PAGESIZE_IDENTIFIER		(2)
#elif SPM_PAGESIZE == 256
	#define	PAGESIZE_IDENTIFIER		(3)
#else
	#error	Strange value for SPM_PAGESIZE. Check the define!
#endif


// -----------------------------------------------------------------------------
// CAN settings

#ifndef	SPI_PRESCALER
	#define	SPI_PRESCALER		8
#endif

#ifndef	MCP2515_BITRATE
	#define	MCP2515_BITRATE		250
#endif

#ifndef	MCP2515_INTERRUPTS
	#define	MCP2515_INTERRUPTS	(1<<RX1IE)|(1<<RX0IE)
#endif

#ifndef	MCP2515_CLKOUT_PRESCALER
	#define	MCP2515_CLKOUT_PRESCALER	0
#endif

// ----------------------------------------------------------------------------
// einige AVR spezifische Daten setzen

#if defined(__AVR_ATtiny84__)
	#define	RWW_PAGES	96
#elif defined(__AVR_AT90PWM3B__)
	#define	RWW_PAGES	96
#else
	#error	chosen AVR command is not supported yet!
#endif

#endif	// CONFIG_H


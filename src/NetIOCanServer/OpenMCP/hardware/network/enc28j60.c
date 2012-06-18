/*! \file enc28j60.c \brief Microchip ENC28J60 Ethernet Interface Driver. */
//*****************************************************************************
//
// File Name	: 'enc28j60.c'
// Title		: Microchip ENC28J60 Ethernet Interface Driver
// Author		: Pascal Stang (c)2005
// Created		: 9/22/2005
// Revised		: 9/22/2005
// Version		: 0.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// Description	: This driver provides initialization and transmit/receive
//	functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
// chip, using an SPI interface to the host processor.
//
//*****************************************************************************
#include <avr/io.h>
#include <avr/interrupt.h>
#include "enc28j60.h"

#include "hardware/spi/spi_core.h"
#include "system/clock/clock.h"
#include "system/math/math.h"

unsigned char Enc28j60Bank;
unsigned int NextPacketPtr;
unsigned char REVID;

#ifdef __AVR_ATmega2561__
	#if defined(OpenMCP)
		#define SPIBUS 1

		#define ENC28J60_CONTROL_PORT	PORTE
		#define ENC28J60_CONTROL_DDR	DDRE
		#define ENC28J60_CONTROL_CS		PE3
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

#ifdef __AVR_ATmega644__
	#if defined(AVRNETIO)
		#define SPIBUS 0

		#define ENC28J60_CONTROL_PORT	PORTB
		#define ENC28J60_CONTROL_DDR	DDRB
		#define ENC28J60_CONTROL_CS		PB4
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
		#define SPIBUS 1

		#define ENC28J60_CONTROL_PORT	PORTD
		#define ENC28J60_CONTROL_DDR	DDRD
		#define ENC28J60_CONTROL_CS		PD5

		#define ENC28J60_RESET_PORT		PORTC
		#define ENC28J60_RESET_DDR		DDRC
		#define ENC28J60_RESET_PIN		PC7
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif

//*********************************************************************************************************
//
// Setzt die MAC-Adressse im Enc28j60
//
//*********************************************************************************************************
void nicSetMacAddress( unsigned char * MAC)
{
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
  	enc28j60Write(MAADR5, MAC[ 0 ] );
	enc28j60Write(MAADR4, MAC[ 1 ] );
	enc28j60Write(MAADR3, MAC[ 2 ] );
	enc28j60Write(MAADR2, MAC[ 3 ] );
	enc28j60Write(MAADR1, MAC[ 4 ] );
	enc28j60Write(MAADR0, MAC[ 5 ] );
}

//*********************************************************************************************************
//
// Sende Rad Command
//
//*********************************************************************************************************
 unsigned char enc28j60ReadOp( unsigned char op,  unsigned char address)
{
	unsigned char data;
	// CS aktive setzen
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);
	// lesecomando schreiben
	data = SPI_ReadWrite( SPIBUS, op | (address & ADDR_MASK) );
	// dummy senden um ergebnis zu erhalten
	data = SPI_ReadWrite( SPIBUS, 0x00 );
	// dummy read machen
	if ( address & 0x80 )
		data = SPI_ReadWrite( SPIBUS, 0x00 );
	// CS wieder freigeben
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
	
	return data;
}

//*********************************************************************************************************
//
// Sende Write Command
//
//*********************************************************************************************************
void enc28j60WriteOp( unsigned char op, unsigned char address,  unsigned char data)
{
	// CS aktive setzen
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);
	// schreibcomando senden
	SPI_ReadWrite( SPIBUS, op | (address & ADDR_MASK) );
	// daten senden
	SPI_ReadWrite( SPIBUS, data );
	// CS wieder freigeben
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
}

//*********************************************************************************************************
//
// Buffer einlesen
//
//*********************************************************************************************************
void enc28j60ReadBuffer(unsigned int len, unsigned char * data)
{
	// assert CS
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);
	
	// issue read command
	SPI_ReadWrite( SPIBUS, ENC28J60_READ_BUF_MEM );

	SPI_ReadBlock( SPIBUS, data, len);
/*	while(len--)
	{
		// read data
		*data++ = SPI1_ReadWrite( 0x00 );
	} */

	// release CS

	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
}

//*********************************************************************************************************
//
// Buffer schreiben
//
//*********************************************************************************************************
void enc28j60WriteBuffer(unsigned int len, unsigned char * data)
{
	// assert CS
	ENC28J60_CONTROL_PORT &= ~(1<<ENC28J60_CONTROL_CS);

	// issue write command
	SPI_ReadWrite( SPIBUS, ENC28J60_WRITE_BUF_MEM );

//	SPI1_FastMem2Write( data, len );
	while(len--)
	{
		// write data
		SPI_ReadWrite( SPIBUS, *data++ );
	}
	// release CS
	ENC28J60_CONTROL_PORT |= (1<<ENC28J60_CONTROL_CS);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
void enc28j60SetBank(unsigned char address)
{
	// set the bank (if needed)
	if((address & BANK_MASK) != Enc28j60Bank)
	{
		// set the bank
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
unsigned char enc28j60Read(unsigned char address)
{
	// set the bank
	enc28j60SetBank(address);
	// do the read
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
void enc28j60Write(unsigned char address, unsigned char data)
{
	// set the bank
	enc28j60SetBank(address);
	// do the write
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
unsigned int enc28j60PhyRead(unsigned char address)
{
     unsigned int data;
 
     // Set the right address and start the register read operation
     enc28j60Write(MIREGADR, address);
     enc28j60Write(MICMD, MICMD_MIIRD);
 
    // wait until the PHY read completes
     while(enc28j60Read(MISTAT) & MISTAT_BUSY);
 
     // quit reading
     enc28j60Write(MICMD, 0x00);
     
     // get data value
     data  = enc28j60Read(MIRDL);
     data |= enc28j60Read(MIRDH)<<8;
     // return the data
     return data;
}

//*********************************************************************************************************
//
// 
//
//*********************************************************************************************************
void enc28j60PhyWrite(unsigned char address, unsigned int data)
{
	// set the PHY register address
	enc28j60Write(MIREGADR, address);

	// write the PHY data
	enc28j60Write(MIWRL, data);
	enc28j60Write(MIWRH, data>>8);

	// wait until the PHY write completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);
}

//*********************************************************************************************************
//
// Initialiesiert den ENC28J60
//
//*********************************************************************************************************
void enc28j60Init(void)
{
	// initialize I/O

	// schau mal ob SPI schon laufen tut, wenn starten
	// if ( SPI_GetInitState() == SPI_NOT_INIT ) 
	// delay(10);
		
	SPI_init( SPIBUS );
			
	ENC28J60_CONTROL_DDR |= 1<<ENC28J60_CONTROL_CS;
	ENC28J60_CONTROL_PORT |= 1<<ENC28J60_CONTROL_CS;

#ifdef __AVR_ATmega644P__
	#if defined(myAVR)
		ENC28J60_RESET_DDR |= 1<<ENC28J60_RESET_PIN;
		ENC28J60_RESET_PORT &= ~(1<<ENC28J60_RESET_PIN);
	
		CLOCK_delay(10);
	
		ENC28J60_RESET_DDR |= 1<<ENC28J60_RESET_PIN;
		ENC28J60_RESET_PORT |= 1<<ENC28J60_RESET_PIN;
	
		CLOCK_delay(10);
	#else
		#error "Hardwareplatform wird nicht unterstützt!"
	#endif
#endif
	
	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	// check CLKRDY bit to see if reset is complete
	
	while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));
	// read die RevID
	REVID = enc28j60Read( EREVID );
	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// set transmit buffer start
	// ETXST defaults to 0x0000 (beginnging of ram)
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);

	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// bring MAC out of reset
	enc28j60Write(MACLCON1, 0x03);
	// enable automatic padding and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);

	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);

	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
	// LED initialization
	enc28j60PhyWrite(PHLCON, 0x0476);
    
}

//*********************************************************************************************************
//
// Sendet ein Packet
//
//*********************************************************************************************************
void enc28j60PacketSend(unsigned int len, unsigned char* packet)
{
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, ( unsigned char ) TXSTART_INIT );
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len));
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	while ( (enc28j60Read(ECON1) & ECON1_TXRTS) != 0 );

}

//*********************************************************************************************************
//
// Hole ein empfangendes Packet
//
//*********************************************************************************************************
unsigned int enc28j60PacketReceiveLenght( void )
	{
	unsigned int len;

	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);

	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	
	return( len );
	}


//*********************************************************************************************************
//
// Hole ein empfangendes Packet
//
//*********************************************************************************************************
unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char* packet)
{
	unsigned int rxstat, rs,re;;
	unsigned int len;

	// check if a packet has been received and buffered
	if( !(enc28j60Read(EIR) & EIR_PKTIF) )
		if (enc28j60Read(EPKTCNT) == 0)
			return 0;

	// Make absolutely certain that any previous packet was discarded	
	//if( WasDiscarded == FALSE)
	//	MACDiscardRx();

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	
	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	
	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	
	// limit retrieve length
	// len = MIN(len, maxlen);
	// When len bigger than maxlen, ignore the packet und read next packetptr
	if ( len > maxlen ) 
	{
		enc28j60Write(ERXRDPTL, (NextPacketPtr));
		enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
		return(0);
	}
	// copy the packet from the receive buffer
	enc28j60ReadBuffer(len, packet);
	
	// an implementation of Errata B1 Section #13
    rs = enc28j60Read(ERXSTH);
    rs <<= 8;
    rs |= enc28j60Read(ERXSTL);
    re = enc28j60Read(ERXNDH);
    re <<= 8;
    re |= enc28j60Read(ERXNDL);
    if (NextPacketPtr - 1 < rs || NextPacketPtr - 1 > re)
    {
        enc28j60Write(ERXRDPTL, (re));
        enc28j60Write(ERXRDPTH, (re)>>8);
    }
    else
    {
        enc28j60Write(ERXRDPTL, (NextPacketPtr-1));
        enc28j60Write(ERXRDPTH, (NextPacketPtr-1)>>8);
    }

/*	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60Write(ERXRDPTL, (NextPacketPtr));
	enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8); */
	
	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return len;
}

unsigned char enc28j60Linkcheck(void)                                              //
{                                                                           //
    if(enc28j60PhyRead(0x01) & PHSTAT1_LLSTAT)                              // Prüfe ob Netzwerk Link vorhanden
    {                                                                       //
        return ( LINK_OK );                                                 //
    }                                                                       //
    else                                                                    // Wenn nicht dann
    {                                                                       //
        return ( LINK_DOWN );                                               //
    }                                                                       //
}

void enc28j60EnableFullDuplex( void )
{
    // setup duplex ----------------------
    // Disable receive logic and abort any packets currently being transmitted
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS|ECON1_RXEN);
     
    unsigned int temp;
    // Set the PHY to the proper duplex mode
    temp = enc28j60PhyRead(PHCON1);
    temp &= ~PHCON1_PDPXMD;
    enc28j60PhyWrite(PHCON1, temp);
    // Set the MAC to the proper duplex mode
    temp = enc28j60Read(MACON3);
    temp &= ~MACON3_FULDPX;
    enc28j60Write(MACON3, temp);
 
    // Set the back-to-back inter-packet gap time to IEEE specified 
    // requirements.  The meaning of the MABBIPG value changes with the duplex
    // state, so it must be updated in this function.
    // In full duplex, 0x15 represents 9.6us; 0x12 is 9.6us in half duplex
    // enc28j60Write(MABBIPG, DuplexState ? 0x15 : 0x12);    
    
    // Reenable receive logic
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

	return;
}

void enc28j60EnableHalfDuplex( void )
{
    // setup duplex ----------------------
    // Disable receive logic and abort any packets currently being transmitted
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS|ECON1_RXEN);
     
    unsigned int temp;
    // Set the PHY to the proper duplex mode
    temp = enc28j60PhyRead(PHCON1);
    temp |= PHCON1_PDPXMD;
    enc28j60PhyWrite(PHCON1, temp);
    // Set the MAC to the proper duplex mode
    temp = enc28j60Read(MACON3);
    temp |= MACON3_FULDPX;
    enc28j60Write(MACON3, temp);
 
    // Set the back-to-back inter-packet gap time to IEEE specified 
    // requirements.  The meaning of the MABBIPG value changes with the duplex
    // state, so it must be updated in this function.
    // In full duplex, 0x15 represents 9.6us; 0x12 is 9.6us in half duplex
    // enc28j60Write(MABBIPG, DuplexState ? 0x15 : 0x12);    
    
    // Reenable receive logic
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

	return;
}

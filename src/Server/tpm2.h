//Port for TMM2.Net communication
#define TPM2_NET_PORT					65506
//Header Bytes
#define TPM2_SER_BLOCK_START_BYTE			0xC9 // 'NEW BLOCK BYTE' for TPM2.Serial
#define TPM2_NET_BLOCK_START_BYTE			0x9C // 'NEW BLOCK BYTE' for TPM2.Net
#define TPM2_BLOCK_TYPE_DATA				0xDA // Block is a  'DATA BLOCK'
#define TPM2_BLOCK_TYPE_CMD				0xC0 // Block is a  'COMMAND BLOCK'
#define TPM2_BLOCK_TYPE_ACK				0xAC // Block is an 'ANSWER without DATA' (Acknowledge)
#define TPM2_BLOCK_TYPE_ACK_DATA			0xAD // Block is an 'ANSWER containing DATA'
#define TPM2_BLOCK_END_BYTE				0x36 // Last Byte of a TMP2 Block
//Header & footer size
#define TPM2_SER_HEADER_SIZE				0x04 // Header size of a TMP.Serial packet
#define TPM2_NET_HEADER_SIZE				0x06 // Header size of a TMP.Serial packet
#define TPM2_FOOTER_SIZE					0x01 // The packet ends with just one byte (TPM2_BLOCK_END_BYTE)
//Positions within a TPM2 Block
#define TPM2_BLOCK_START_BYTE_P			        0x00 // 'NEW BLOCK BYTE' is allways on 1st position
#define TPM2_BLOCK_TYPE_P				0x01 // Block type allways on 2nd position
#define TPM2_FRAME_SIZE_HIGH_P				0x02 // Frame size allways on 3rd and 4th position
#define TPM2_FRAME_SIZE_LOW_P				0x03
#define TPM2_NET_PACKET_NUM_P				0x04 // A TPM2.Net block has packet number on 5th position
#define TPM2_NET_PACKET_TOTAL_PACK_NUM_P                0x05 // The total nummber of packets to be transfered for one frame
#define TPM2_SER_DATA_P		 			0x04 // Data starts at 5th position
#define TPM2_NET_DATA_P		 			0x06 // Data starts at 7th position
#define TPM2_SER_CMD_CONTROL_P				0x04 // If block is a 'COMMAND BLOCK' than 5th position contains the 'COMMAND CONTROL BYTE' (described below)
#define TPM2_NET_CMD_CONTROL_P				0x06 // If block is a 'COMMAND BLOCK' than 7th position contains the 'COMMAND CONTROL BYTE' (described below)
#define TPM2_SER_CMD_TYPE_P	           		0x05 // If block is a 'COMMAND BLOCK' than the actual command type is on 6th position
#define TPM2_NET_CMD_TYPE_P	           		0x07 // If block is a 'COMMAND BLOCK' than the actual command type is on 8th position
#define TPM2_SER_SP_CMD_P	         		0x06 // If command type is 'SPECIAL COMMAND' than the actual special command is on 7th position
#define TPM2_NET_SP_CMD_P	         		0x08 // If command type is 'SPECIAL COMMAND' than the actual special command is on 9th position
#define TPM2_SER_CMD_DATA_P				0x06 // If block is a 'COMMAND BLOCK' than command data starts at 7th position
#define TPM2_NET_CMD_DATA_P				0x08 // If block is a 'COMMAND BLOCK' than command data starts at 9th position
#define TPM2_SER_SP_CMD_DATA_P			        0x07 // If block is a 'COMMAND BLOCK' and command type is 'SPECIAL COMMAND' than command data starts at 8th position
#define TPM2_NET_SP_CMD_DATA_P				0x09 // If block is a 'COMMAND BLOCK' and command type is 'SPECIAL COMMAND' than command data starts at 10th position
//Command type definitions
#define TPM2_CMD_BLOCK_CONFIG				0x00 // configuration data as one block
#define TPM2_CMD_STORE_CONFIG				0x01 // save actual configuration
#define TPM2_CMD_LOAD_CONFIG				0x02 // load a specific configuration
#define TPM2_CMD_INIT_SD_TRANSFER			0x03 // prepare SD card for receiving a file
#define	TPM2_CMD_PLAY_SD				0x04 // play data from SD card
#define TPM2_CMD_STOP_SD				0x05 // stop playing data from SD card
#define TPM2_CMD_RW_SD					0x06 // read/write one frame from/to SD card
#define TPM2_CMD_BRIGHTNESS				0x0A // master brightness
#define TPM2_CMD_GAMMA					0x0B // gamma correction
#define TPM2_CMD_SPEED					0x0C // master speed
#define TPM2_CMD_PROGRAMM				0x0D // program number
#define TPM2_CMD_START_COLOR				0x0E // start color
#define TPM2_CMD_TIMEOUT				0x0F // time out value
#define TPM2_CMD_NUM_PIXEL				0x10 // number of pixels
#define TPM2_CMD_REPETITIONS				0x11 // number of repetitions
#define TPM2_CMD_START_ADRESS				0x12 // start address
#define TPM2_CMD_PING					0x20 // 'Ping' --> Device has to replay with an ACK
#define TPM2_CMD_SPECIAL_CMD				0xFF // SPECIAL COMMAND --> In this case user can define any own command on position 'TPM2_SP_CMD_P'
//Reply definitions
#define TPM2_ACK_OK					0x00 // Command was acknowledged and data received fine
#define TPM2_ACK_CORRUPT_DATA				0x01 // Command was acknowledged but data was bad
#define TPM2_ACK_UNNOKWN_COMMAD				0x02 // Command unknown
#define TPM2_ACK_FRAME_ERROR				0x03 // Frame Error
#define TPM2_ACK_FRAME_OVER_SIZE			0x04 // Frame too big for device's buffer
//Bit positions within the 'COMMAND CONTROL BYTE'
#define TPM2_CMD_DIR_BIT				0x07 // The MSB of the 'CONTROL BYTE' determines whether the command is a READ command or a WRITE command
#define TPM2_CMD_DIR_READ				0x00 // For a READ  command it is 0
#define TPM2_CMD_DIR_WRITE			    	0x01 // For a WRITE command it is 1
#define TPM2_CMD_ACK_BIT				0x06 // The MSB-1 of the 'CONTROL BYTE' determines whether the command expects an answer or not

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;



struct TPM2Packet {
  unsigned char StartByte ;
  unsigned char Type ;
  unsigned char FrameSize[2] ;
  unsigned char PacketNum ;
  unsigned char TotalNum ;
  unsigned char Data[1500] ;
  unsigned char BlockEndByte ;
} ;

struct CANToDMX {
  USHORT Add ;
  int Port[8] ;
} ;

extern int UniverseLine[2] ;
extern int Universe[2] ;
extern int TPMVerbose ;
extern FILE *logfd ;
extern int Can0SockFD;
extern int Can1SockFD;
extern char RouteIF0[255];
extern char RouteIF1[255];
extern char *LogTime(void) ;
extern ULONG BuildCANId (char Prio, char Repeat, char FromLine, USHORT FromAdd, char ToLine, USHORT ToAdd, char Group);
extern void ReInitCAN (void) ;
extern struct TPM2Packet TPM2Data ;
extern struct CANToDMX CANBuffer[2][ARTNET_DMX_LENGTH/3] ;
extern int TPM2FD;


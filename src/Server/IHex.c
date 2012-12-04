/* IHex.h: Deklaration fuer das Lesen und Schreiben von Intel-Hex-Files

   ToDo: Globale Variable eliminieren 
*/

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "IHex.h"

typedef unsigned long ULONG ;
typedef unsigned short USHORT ;
#ifndef TRUE
#define TRUE (0==0)
#endif
#ifndef FALSE
#define FALSE (0==1)
#endif

u_char FileBuffer[BUFFERSIZE] ;

/* Einzelne Hex-Zahlen einlesen */

static int ScanHex(char **sp, int len, USHORT *result)
{
	char cifra[20];
	int j;

	if (len > 4)
		return -2;
	for (j = 0; j < len && **sp; j++)
	{
		cifra[j] = *(*sp)++;
		if ( !isxdigit((int)cifra[j]) )
			return -1;
	}
	cifra[j] = '\0';
	*result = strtoul(cifra, NULL, 16);

	return 0;
}

int New_IHexRecord(int type, uint16_t address, const uint8_t *data, int dataLen, IHexRecord *ihexRecord) {
	/* Data length size check, assertion of ihexRecord pointer */
	if (dataLen < 0 || dataLen > IHEX_MAX_DATA_LEN/2 || ihexRecord == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
	
	ihexRecord->type = type;
	ihexRecord->address = address;
	memcpy(ihexRecord->data, data, dataLen);
	ihexRecord->dataLen = dataLen;
	ihexRecord->checksum = Checksum_IHexRecord(ihexRecord);
	
	return IHEX_OK;		
}

int Write_IHexRecord(const IHexRecord *ihexRecord, FILE *out) {
	int i;
	
	/* Check our record pointer and file pointer */
	if (ihexRecord == NULL || out == NULL)
		return IHEX_ERROR_INVALID_ARGUMENTS;
		
	/* Check that the data length is in range */
	if (ihexRecord->dataLen > IHEX_MAX_DATA_LEN/2)
		return IHEX_ERROR_INVALID_RECORD;
	
	/* Write the start code, data count, address, and type fields */
	if (fprintf(out, "%c%2.2X%2.4X%2.2X", IHEX_START_CODE, ihexRecord->dataLen, ihexRecord->address, ihexRecord->type) < 0)
		return IHEX_ERROR_FILE;
		
	/* Write the data bytes */
	for (i = 0; i < ihexRecord->dataLen; i++) {
		if (fprintf(out, "%2.2X", ihexRecord->data[i]) < 0)
			return IHEX_ERROR_FILE;
	}
	
	/* Calculate and write the checksum field */
	if (fprintf(out, "%2.2X\r\n", Checksum_IHexRecord(ihexRecord)) < 0)
		return IHEX_ERROR_FILE;
		
	return IHEX_OK;
}

uint8_t Checksum_IHexRecord(const IHexRecord *ihexRecord) {
	uint8_t checksum;
	int i;

	/* Add the data count, type, address, and data bytes together */
	checksum = ihexRecord->dataLen;
	checksum += ihexRecord->type;
	checksum += (uint8_t)ihexRecord->address;
	checksum += (uint8_t)((ihexRecord->address & 0xFF00)>>8);
	for (i = 0; i < ihexRecord->dataLen; i++)
		checksum += ihexRecord->data[i];
	
	/* Two's complement on checksum */
	checksum = ~checksum + 1;

	return checksum;
}

int LoadIHexFile (char *FileName, long relocation_offset)
{
	int rval = 0;
	int okline_counter = 0;
	u_char *endp;
	u_char *dp;
	int img_size = 0;
	char riga[MAXLINE+1];
	long laddr = 0;
	FILE *fh;
	USHORT bcount;
	USHORT addr;
	USHORT data;
	USHORT rectype;

	endp = FileBuffer + BUFFERSIZE;
	dp = FileBuffer;

	//Relocation check
	if (dp + relocation_offset > endp)
		return BADPARAM;
	else
		dp += relocation_offset;

	if ( (fh = fopen(FileName, "r")) == NULL )
		return FILENOTFOUND;

	riga[MAXLINE] = '\0';
	while ( fgets(riga, MAXLINE, fh) )
	{
		char *s;
		int k;

		if ( (s = strchr(riga, ':')) == NULL )
			continue;
		else
			s++;

		//Byte Count
		if ( ScanHex(&s, 2, &bcount) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		u_char checksum = (u_char)bcount;

		//Address
		if ( ScanHex(&s, 4, &addr) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		checksum += (u_char)(addr >> 8);
		checksum += (u_char)addr;

		//affect only low 16 bits of address
		laddr &= 0xFFFF0000;
		laddr |= addr;

		//Record Type
		if ( ScanHex(&s, 2, &rectype) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		checksum += (u_char)rectype;

		//Data Byte
		if (rectype == DATA_RECORD)
		{
			//buffer overflow
			if (dp+laddr+bcount > endp)
			{
				rval = BUFFEROVERFLOW;
				break;
			}

			char ok = TRUE;
			u_char *p;
			for (k = 0, p = dp+laddr; k < bcount && ok; k++)
			{
				if ( ScanHex(&s, 2, &data) != OK )
					ok = FALSE;

				checksum += (u_char)data;
				*p++ = (u_char)data;
			}
			if (!ok)	//Irgendwas war defekt
			{
				rval = BADFILETYPE;
				break;
			}
			img_size = laddr + bcount;
		}
		else if (rectype == SEG_ADDR_RECORD)
		{
			if (bcount != 2)
			{
				rval = BADFILETYPE;
				break;
			}
			else
			{
				//Address
				USHORT addr;
				if ( ScanHex(&s, 4, &addr) != OK )
				{
					rval = BADFILETYPE;
					break;
				}
				checksum += (u_char)(addr >> 8);
				checksum += (u_char)addr;

				laddr = (long)addr << 4;
			}
		}
		else if (rectype == LIN_ADDR_RECORD)
		{
			if (bcount != 2)
			{
				rval = BADFILETYPE;
				break;
			}
			else
			{
				//Address
				USHORT addr;
				if ( ScanHex(&s, 4, &addr) != OK )
				{
					rval = BADFILETYPE;
					break;
				}
				checksum += (u_char)(addr >> 8);
				checksum += (u_char)addr;

				laddr = (long)addr << 16;
			}
		}
		else	// Unknown record type: discard data bytes (but check for validity)
		{
			char ok = TRUE;
			while (bcount-- && ok)
			{
				if ( ScanHex(&s, 2, &data) != OK )
					ok = FALSE;
				checksum += (u_char)data;
			}
			if (!ok)
			{
				rval = BADFILETYPE;
				break;
			}
		}

		if ( ScanHex(&s, 2, &data) != OK )
		{
			rval = BADFILETYPE;
			break;
		}
		if ( (u_char)data != (u_char)(~checksum + 1) )
		{
			rval = BADFILETYPE;
			break;
		}
		else
			okline_counter++;

		if (rectype == END_RECORD)
		{
			break;
		}
	}

	fclose(fh);

	if (okline_counter == 0)
		rval = BADFILETYPE;
	else
	{
		if (img_size == 0)     
			img_size++;
	}


	if (rval == OK)
	{
		rval = img_size;
	}

	return rval;
}


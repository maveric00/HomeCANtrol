/*! \file nano_db.c \brief Funktionen für das Arbeiten mit nanoDB. */
/***************************************************************************
 *            nano_db.c
 *
 *  Mon Dec 28 20:53:51 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/
///	\ingroup	system 
///	\defgroup nanoDB NanoDB Funktionen (nano_db.c)
///	\code #include "nano_db.h" \endcode
//****************************************************************************
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
//@{
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#if defined(MMC)

	#include "nano_db.h"
	#include "system/filesystem/fat.h"
	#include "system/filesystem/filesystem.h"

char root[] = "/";

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Erstellt eine Datei auf der MMC/SD-Karte.
 * \param 	FULLNAME			Der vollständige Pfad zur Datei der angelegt werden soll.
 * \returns	nanoDB_ERROR if failed, nanoDB_OK ist success
 */
/*------------------------------------------------------------------------------------------------------------*/
int nano_DB_makeDB( char * FULLNAME )
{
	struct fat_dir_entry_struct directory;
	struct fat_dir_struct* dd;
	struct fat_dir_entry_struct dir_entry;

	char * filename;
	char * directoryname;

	int returncode = nanoDB_ERROR;

	directoryname = FULLNAME ;
	filename = nano_DB_getfilename( FULLNAME );

	if ( FULLNAME == filename )
		directoryname = root ;

	fat_get_dir_entry_of_path(fs, directoryname , &directory);

	// Verzeichbnis öffnen
	dd = fat_open_dir(fs, &directory);
	if(dd)
	{
 		if ( fat_create_file( dd , filename, &dir_entry ) )
			returncode = nanoDB_OK;
	}
	fat_close_dir ( dd );

	return( returncode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Schriebt einen Datensatz.
 * \param 	FULLNAME			Der vollständige Pfad zur Datei.
 * \param	DBentryNumber		Der Eintrag der geschrieben werden soll.
 * \param	DB					Pointer auf den Datensatz der geschrieben werden soll.
 * \param	DBlenght			Größe des Datensatzes in Bytes.
 * \returns	nanoDB_ERROR if failed, nanoDB_OK ist success
 */
/*------------------------------------------------------------------------------------------------------------*/
int nano_DB_writeDBentry( char * FULLNAME, long DBentryNumber, void * DB, int DBlenght )
{
	struct fat_dir_entry_struct directory;
	struct fat_dir_struct* dd;
	struct fat_dir_entry_struct dir_entry;

	char * directoryname;
	char * filename;
	long filesize=-1;
	
	int returncode = nanoDB_ERROR;

	if ( DBentryNumber > nano_DB_getnumbersofDB( FULLNAME, DBlenght ) )
		return( returncode );

	directoryname = FULLNAME;
	filename = nano_DB_getfilename( FULLNAME );

	if ( FULLNAME == filename )
		directoryname = root ;
	
	fat_get_dir_entry_of_path(fs, directoryname , &directory);

	// Verzeichbnis öffnen
	dd = fat_open_dir(fs, &directory);
	if(dd)
	{
		// Verzeichniss inhalt lesen und Datei suchen
		while(fat_read_dir(dd, &dir_entry))
	    {
	        if(strcmp( dir_entry.long_name, filename ) == 0)
	        {
				filesize = dir_entry.file_size;
	            fat_reset_dir(dd);					
	            break;
	        }
		}

		if ( ( filesize % DBlenght ) == 0 )
		{
			struct fat_file_struct* fd = fat_open_file(fs, &dir_entry);
			if ( fd )
			{
				long offset;
				unsigned char seek = FAT_SEEK_SET;
				offset = DBentryNumber * DBlenght ;

				if ( offset > filesize )
				{
					offset = 0;
					seek = FAT_SEEK_END;
				}
				
				if(fat_seek_file(fd, &offset , seek))
				{
	                if( !fat_write_file(fd, (uint8_t*) DB, DBlenght ) != DBlenght )
					{
						returncode = nanoDB_OK;
//						printf_P( PSTR("%d Byte in Datei \"%s\" geschrieben (size %ld)!\r\n"),DBlenght, filename, dir_entry.file_size );						
					}
//					else
//						printf_P( PSTR("Kann nicht in Datei \"%s\" schreiben!\r\n"),filename);
				}
//				else
//					printf_P( PSTR("Fehler beim seeken!\r\n"));
                fat_close_file( fd );				
			}
//			else
//				printf_P( PSTR("Datei \"%s\" kann nicht geöffnet werden\r\n"), filename );
		}
//		else
//			printf_P( PSTR("Datei \"%s\" kann nicht gefunden werden oder ist korrupt!\r\n"), filename );
	}
//	else
//		printf_P( PSTR("Verzeichniss \"%s\" kann nicht geöffnet werden!\r\n"), directoryname);

	fat_close_dir ( dd );

	return( returncode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Liest einen Datensatz aus einer Datei.
 * \param 	FULLNAME			Der vollständige Pfad zur Datei aus der gelesen wird.
 * \param	DBentryNumber		Der Eintrag der gelesen werden soll.
 * \param	DB					Pointer auf den Speicher in den der Datensatz gelesen wird.
 * \param	DBlenght			Größe des Datensatzes in Bytes.
 * \returns	nanoDB_ERROR if failed, nanoDB_OK ist success
 */
/*------------------------------------------------------------------------------------------------------------*/
int nano_DB_readDBentry( char * FULLNAME, long DBentryNumber, void * DB, int DBlenght )
{
	struct fat_dir_entry_struct directory;
	struct fat_dir_struct* dd;
	struct fat_dir_entry_struct dir_entry;

	char * directoryname;
	char * filename;
	long filesize=-1;

	// returncode auf Fehler setzen
	int returncode = nanoDB_ERROR;

	if ( DBentryNumber >= nano_DB_getnumbersofDB( FULLNAME, DBlenght ) )
		return( returncode );

	// FULLNAME in Verzeichnissname und Dateiname zerlegen
	directoryname = FULLNAME;
	filename = nano_DB_getfilename( FULLNAME );

	// Wenn FULLNAME gleich filename, denn Verzeichniss setzen
	if ( FULLNAME == filename )
		directoryname = root ;

	// in Verzeichniss springen
	fat_get_dir_entry_of_path(fs, directoryname , &directory);

	// Verzeichbnis öffnen
	dd = fat_open_dir(fs, &directory);
	if(dd)
	{
		// Verzeichniss inhalt lesen und Datei suchen
		while(fat_read_dir(dd, &dir_entry))
	    {
	        if(strcmp( dir_entry.long_name, filename ) == 0)
	        {
				filesize = dir_entry.file_size;
	            fat_reset_dir(dd);					
	            break;
	        }
		}

		if ( ( filesize % DBlenght ) == 0 )
		{
			struct fat_file_struct* fd = fat_open_file(fs, &dir_entry);
			if ( fd )
			{
				long offset;
				offset = DBentryNumber * DBlenght ;

				if(fat_seek_file(fd, &offset , FAT_SEEK_SET))
				{
	                if( !fat_read_file(fd, (uint8_t*) DB, DBlenght ) != DBlenght )
					{
						returncode = nanoDB_OK;
//						printf_P( PSTR("%d Byte von Pos %ld gelesen (size %ld)!\r\n"),DBlenght, offset, filesize );						
					}
//					else
//						printf_P( PSTR("Kann Datei \"%s\" nicht lesen!\r\n"),filename);
				}
//				else
//					printf_P( PSTR("Fehler beim seeken!\r\n"));
                fat_close_file( fd );				
			}
//			else
//				printf_P( PSTR("Datei \"%s\" kann nicht geöffnet werden\r\n"), filename );
		}
//		else
//			printf_P( PSTR("Datei \"%s\" kann nicht gefunden werden oder ist korrupt!\r\n"), filename );
	}
//	else
//		printf_P( PSTR("Verzeichniss \"%s\" kann nicht geöffnet werden!\r\n"), directoryname);

	fat_close_dir ( dd );

	return( returncode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt die Anzahl der Datensätze in der Datei zurück.
 * \param 	FULLNAME			Der vollständige Pfad zur Datei.
 * \param	DBlenght			Größe des Datensatzes in Bytes.
 * \returns	nanoDB_ERROR if failed, or > 0.
 */
/*------------------------------------------------------------------------------------------------------------*/
int nano_DB_getnumbersofDB( char * FULLNAME, int DBlenght )
{
	struct fat_dir_entry_struct directory;
	struct fat_dir_struct* dd;
	struct fat_dir_entry_struct dir_entry;

	char * directoryname;
	char * filename;

	// returncode auf Fehler setzen
	int returncode = nanoDB_ERROR;
	
	// FULLNAME in Verzeichnissname und Dateiname zerlegen
	directoryname = FULLNAME;
	filename = nano_DB_getfilename( FULLNAME );

	// Wenn FULLNAME gleich filename, denn Verzeichniss setzen
	if ( FULLNAME == filename )
		directoryname = root ;

	// in Verzeichniss springen
	fat_get_dir_entry_of_path(fs, directoryname , &directory);

	// Verzeichbnis öffnen
	dd = fat_open_dir(fs, &directory);
	if(dd)
	{
		// Verzeichnissinhalt lesen und Datei suchen
		while(fat_read_dir(dd, &dir_entry))
	    {
			// Wenn Datei gefunden, filesize holen und als returncode setzen
	        if(strcmp( dir_entry.long_name, filename ) == 0)
	        {
				returncode = dir_entry.file_size / DBlenght ;
	            fat_reset_dir(dd);					
	            break;
	        }
		}
	}
	// verzeichniss schließen
	fat_close_dir ( dd );

	return( returncode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Zerlegt einen FULLNAME in Filename und Verzeichnis.
 * \param 	FULLNAME			Der vollständige Pfad.
 * \returns	Pointer auf den Filename.
 */
/*------------------------------------------------------------------------------------------------------------*/
char * nano_DB_getfilename( char * FULLNAME )
{
	char * filename;

	filename = FULLNAME + strlen( FULLNAME );

	while( filename != FULLNAME )
	{
		if ( * filename == '/' )
		{
			*filename = '\0';
			filename++;
			break;
		}
		filename--;
	}
	return( filename );
}
#else
	#error "MMC-Support muss aktiviert sein um nano_DB benutzen zu können."
#endif
//@}

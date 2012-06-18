/***************************************************************************
 *            filesystem.c
 *
 *  Sun Sep 13 21:06:43 2009
 *  Copyright  2009  Dirk Broßwick
 *  <sharandac@snafu.de>
 ****************************************************************************/

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

#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include "fat.h"
#include "fat_config.h"
#include "partition.h"
#include "hardware/sd_raw/sd_raw.h"
#include "hardware/sd_raw/sd_raw_config.h"

#include "filesystem.h"

struct partition_struct* partition;
struct fat_fs_struct* fs;

int FILE_init( void )
{
        /* open first partition */
        partition = partition_open(sd_raw_read,
                                                            sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
                                                            sd_raw_write,
                                                            sd_raw_write_interval,
#else
                                                            0,
                                                            0,
#endif
                                                            0
                                                           );

        if(!partition)
        {
            /* If the partition did not open, assume the storage device
             * is a "superfloppy", i.e. has no MBR.
             */
            partition = partition_open(sd_raw_read,
                                       sd_raw_read_interval,
#if SD_RAW_WRITE_SUPPORT
                                       sd_raw_write,
                                       sd_raw_write_interval,
#else
                                       0,
                                       0,
#endif
                                       -1
                                      );
            if(!partition)
            {
            	return( FILESYSTEM_FAILED );
			}
				
        }

        /* open file system */
        fs = fat_open(partition);
        if(!fs)
        {
            return( FILESYSTEM_FAILED );
        }

		struct fat_dir_entry_struct directory;

        fat_get_dir_entry_of_path(fs, "/", &directory);

/* 		struct fat_dir_struct* dd;

        dd = fat_open_dir(fs, &directory);
        if(!dd)
        {
            printf_P(PSTR("opening root directory failed\r\n"));
            return;
		}

                struct fat_dir_entry_struct dir_entry;
                while(fat_read_dir(dd, &dir_entry))
                {
                    uint8_t spaces = sizeof(dir_entry.long_name) - strlen(dir_entry.long_name) + 4;

                    printf_P( PSTR("%s"),dir_entry.long_name);
                    printf_P( PSTR("%c"), dir_entry.attributes & FAT_ATTRIB_DIR ? '/' : ' ' );
                    while(spaces--)
                        printf_P(PSTR(" "));
                    printf_P( PSTR("%ld\r\n"),dir_entry.file_size);
                }
*/
	return( FILESYSTEM_OK );
}

/*uint8_t find_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name, struct fat_dir_entry_struct* dir_entry)
{
    while(fat_read_dir(dd, dir_entry))
    {
        if(strcmp(dir_entry->long_name, name) == 0)
        {
            fat_reset_dir(dd);
            return 1;
        }
    }

    return 0;
}

struct fat_file_struct* open_file_in_dir(struct fat_fs_struct* fs, struct fat_dir_struct* dd, const char* name)
{
    struct fat_dir_entry_struct file_entry;
    if(!find_file_in_dir(fs, dd, name, &file_entry))
        return 0;

    return fat_open_file(fs, &file_entry);
}*/

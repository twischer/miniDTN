/*
 * Copyright (c) 2012, Institute of Operating Systems and Computer Networks (TU Brunswick).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup Device Interfaces
 * @{
 *
 * \addtogroup diskio_layer
 * @{
 */

/**
 * \file
 *      DiskIO Abstraction Layer Implementation
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */
 
#include "diskio.h"
#include "mbr.h"
#include <string.h>
#include "../../interfaces/flash-microSD.h"
#include "../../interfaces/flash-at45db.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct diskio_device_info *default_device = 0;
static struct diskio_device_info devices[DISKIO_MAX_DEVICES];

int diskio_rw_op( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer, uint8_t op );

void print_device_info( struct diskio_device_info *dev ) {
	printf("DiskIO Device Info\n");

	printf("\ttype = ");
	switch(dev->type & 0x7F) {
		case DISKIO_DEVICE_TYPE_SD_CARD:
			printf("SD_CARD");
			break;
		case DISKIO_DEVICE_TYPE_GENERIC_FLASH:
			printf("Generic_Flash");
			break;
		default:
			printf("Unknown");
			break;
	}

	if(dev->type & DISKIO_DEVICE_TYPE_PARTITION) {
		printf(" (Partition)");
	}

	printf("\n");
	printf("\tnumber = %d\n", dev->number);
	printf("\tpartition = %d\n", dev->partition);
	printf("\tnum_sectors = %ld\n", dev->num_sectors);
	printf("\tsector_size = %d\n", dev->sector_size);
	printf("\tfirst_sector = %ld\n", dev->first_sector);
}

int diskio_read_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer ) {
	return diskio_rw_op( dev, block_address, 1, buffer, DISKIO_OP_READ_BLOCK );
}

int diskio_read_blocks( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer ) {
	return diskio_rw_op( dev, block_start_address, num_blocks, buffer, DISKIO_OP_READ_BLOCKS );
}

int diskio_write_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer ) {
	return diskio_rw_op( dev, block_address, 1, buffer, DISKIO_OP_WRITE_BLOCK );
}

int diskio_write_blocks( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer ) {
	return diskio_rw_op( dev, block_start_address, num_blocks, buffer, DISKIO_OP_WRITE_BLOCKS );
}

int diskio_rw_op( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer, uint8_t op ) {
	if( dev == NULL ) {
		if( default_device == 0 ) {
			return DISKIO_ERROR_NO_DEVICE_SELECTED;
		}
		dev = default_device;
	}

	block_start_address += dev->first_sector;
	switch( dev->type & DISKIO_DEVICE_TYPE_MASK ) {
		uint8_t ret_code = 0;
		uint8_t tries = 0, reinit = 0;
		case DISKIO_DEVICE_TYPE_SD_CARD:
			switch( op ) {
				case DISKIO_OP_READ_BLOCK:
#ifndef DISKIO_OLD_STYLE
					for(tries = 0; tries < 50; tries++) {
						ret_code = microSD_read_block( block_start_address, buffer );
						if( ret_code == 0 ) {
							return DISKIO_SUCCESS;
						}
						_delay_ms(1);

						if( reinit == 0 && tries == 49 ) {
							tries = 0;
							reinit = 1;
							microSD_init();
						}
					}
					PRINTF("diskion_rw_op(): Unrecoverable Error!");
					return DISKIO_ERROR_INTERNAL_ERROR;
#else
					if( microSD_read_block( block_start_address, buffer ) == 0) {
						return DISKIO_SUCCESS;
					}
					return DISKIO_ERROR_TRY_AGAIN;
#endif
					break;
				case DISKIO_OP_READ_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_WRITE_BLOCK:
#ifndef DISKIO_OLD_STYLE
					for(tries = 0; tries < 50; tries++) {
						ret_code = microSD_write_block( block_start_address, buffer );
						if( ret_code == 0 ) {
							return DISKIO_SUCCESS;
						}
						_delay_ms(1);
						if( reinit == 0 && tries == 49 ) {
							tries = 0;
							reinit = 1;
							microSD_init();
						}
					}
					PRINTF("diskion_rw_op(): Unrecoverable Error!");
					return DISKIO_ERROR_INTERNAL_ERROR;
#else
					if( microSD_write_block( block_start_address, buffer ) == 0) {
						return DISKIO_SUCCESS;
					}
					return DISKIO_ERROR_TRY_AGAIN;
#endif
					break;
				case DISKIO_OP_WRITE_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				default:
					return DISKIO_ERROR_OPERATION_NOT_SUPPORTED;
					break;
			}
			break;
		case DISKIO_DEVICE_TYPE_GENERIC_FLASH:
			switch( op ) {
				case DISKIO_OP_READ_BLOCK:
					at45db_read_page_bypassed( block_start_address, 0, buffer, 512 );
					return DISKIO_SUCCESS;
					break;
				case DISKIO_OP_READ_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_WRITE_BLOCK:
					at45db_write_buffer( 0, buffer, 512 );
					at45db_buffer_to_page( block_start_address );
					return DISKIO_SUCCESS;
					break;
				case DISKIO_OP_WRITE_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				default:
					return DISKIO_ERROR_OPERATION_NOT_SUPPORTED;
					break;
			}			
			break;
		case DISKIO_DEVICE_TYPE_NOT_RECOGNIZED:
		default:
			return DISKIO_ERROR_DEVICE_TYPE_NOT_RECOGNIZED;
	}
	return DISKIO_SUCCESS;
}

void diskio_set_default_device( struct diskio_device_info *dev ) {
	default_device = dev;
}

struct diskio_device_info * diskio_devices() {
	return devices;
}

int diskio_detect_devices() {	
	struct mbr mbr;
	int dev_num = 0;
	int i = 0, index = 0;

	memset( devices, 0, DISKIO_MAX_DEVICES * sizeof(struct diskio_device_info) );
	if( at45db_init() == 0 ) {
		devices[index].type = DISKIO_DEVICE_TYPE_GENERIC_FLASH;
		devices[index].number = dev_num;
		/* This Flash has 4096 Pages */
		devices[index].num_sectors = 4096;
		/* A Page is 528 Bytes long, but for easier acces we use only 512 Byte*/
		devices[index].sector_size = 512;
		devices[index].first_sector = 0;
		index += 1;
	}

	if( microSD_init() == 0 ) {
		devices[index].type = DISKIO_DEVICE_TYPE_SD_CARD;
		devices[index].number = dev_num;
		devices[index].num_sectors = microSD_get_block_num();
		devices[index].sector_size = microSD_get_block_size();
		devices[index].first_sector = 0;
		if( devices[index].sector_size > DISKIO_MAX_SECTOR_SIZE ) {
			goto end_of_function;
		}

		mbr_init( &mbr );
		mbr_read( &devices[index], &mbr );
		index += 1;
		for( i = 0; i < 4; ++i ) {
			if( mbr_hasPartition( &mbr, i + 1 ) != 0 ) {
				devices[index].type = DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION;
				devices[index].number = dev_num;
				devices[index].partition = i + 1;
				devices[index].num_sectors = mbr.partition[i].lba_num_sectors;
				devices[index].sector_size = devices[dev_num].sector_size;
				devices[index].first_sector = mbr.partition[i].lba_first_sector;
				index += 1;
			}
		}

		dev_num += 1;
		index += 1;
	}

	end_of_function:

	if(index == 0) {
		return DISKIO_FAILURE;
	}

	return DISKIO_SUCCESS;
}

#include "diskio.h"
#include "mbr.h"
#ifndef DISKIO_DEBUG
#include "../../interfaces/flash_microSD.h"
#endif

static struct diskio_device_info *default_device = 0;
static struct diskio_device_info devices[DISKIO_MAX_DEVICES];

/* TODO: Länge des Buffers sollte mitübergeben werden, macht die Nutzung einfacher */

#ifdef DISKIO_DEBUG
#include <stdio.h>
FILE *handle = 0;

void diskio_read_block_file( uint32_t block_start_address, uint8_t *buffer ) {
	uint16_t i;
	fseek( handle, block_start_address * DISKIO_DEBUG_FILE_SECTOR_SIZE, SEEK_SET );
	for( i = 0; i < DISKIO_DEBUG_FILE_SECTOR_SIZE; ++i ) {
		buffer[i] = (uint8_t) getc( handle );
	}
}

void diskio_read_blocks_file( uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer ) {
	uint16_t i;
	uint8_t blocks;
	fseek( handle, block_start_address * DISKIO_DEBUG_FILE_SECTOR_SIZE, SEEK_SET );
	for( blocks = 0; blocks < num_blocks; ++blocks){
		for( i = 0; i < DISKIO_DEBUG_FILE_SECTOR_SIZE; ++i ) {
			buffer[i + DISKIO_DEBUG_FILE_SECTOR_SIZE * blocks] = (uint8_t) getc( handle );
		}
	}
}

void diskio_write_block_file( uint32_t block_start_address, uint8_t *buffer ) {
	uint16_t i;
	fseek( handle, block_start_address * DISKIO_DEBUG_FILE_SECTOR_SIZE, SEEK_SET );
	for( i = 0; i < DISKIO_DEBUG_FILE_SECTOR_SIZE; ++i ) {
		putc( buffer[i], handle );
	}
}
 /*ToDo: Rewrite with fread and fwrite*/
 /*ToDo: Test if it replaces data, not appends it*/
void diskio_write_blocks_file( uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer ) {
	uint16_t i;
	uint8_t blocks;
	fseek( handle, block_start_address * DISKIO_DEBUG_FILE_SECTOR_SIZE, SEEK_SET );
	for( blocks = 0; blocks < num_blocks; ++blocks){
		for( i = 0; i < DISKIO_DEBUG_FILE_SECTOR_SIZE; ++i ) {
			putc( buffer[i + DISKIO_DEBUG_FILE_SECTOR_SIZE * blocks], handle );
		}
	}
}
#endif

#define DISKIO_OP_WRITE_BLOCK  1
#define DISKIO_OP_READ_BLOCK   2
#define DISKIO_OP_WRITE_BLOCKS 3
#define DISKIO_OP_READ_BLOCKS  4

int diskio_rw_op( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer, uint8_t op );

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
		if( default_device == 0 )
			return DISKIO_ERROR_NO_DEVICE_SELECTED;
		dev = default_device;
	}
	block_start_address += dev->first_sector;
	switch( dev->type & DISKIO_DEVICE_TYPE_MASK ) {
		uint8_t ret_code = 0;
		case DISKIO_DEVICE_TYPE_SD_CARD:
			switch( op ) {
				case DISKIO_OP_READ_BLOCK:
					ret_code = microSD_read_block( block_start_address, buffer );
					if( ret_code != 0 )
						return DISKIO_ERROR_INTERNAL_ERROR;
					break;
				case DISKIO_OP_READ_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_WRITE_BLOCK:
					ret_code = microSD_write_block( block_start_address, buffer );
					if( ret_code != 0 )
						return DISKIO_ERROR_INTERNAL_ERROR;
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
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_READ_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_WRITE_BLOCK:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_WRITE_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				default:
					return DISKIO_ERROR_OPERATION_NOT_SUPPORTED;
					break;
			}			
			break;
		case DISKIO_DEVICE_TYPE_FILE:
			#ifdef DISKIO_DEBUG
			switch( op ) {
				case DISKIO_OP_READ_BLOCK:
					diskio_read_block_file( block_start_address, buffer );
					break;
				case DISKIO_OP_READ_BLOCKS:
					diskio_read_blocks_file( block_start_address, num_blocks, buffer );
					break;
				case DISKIO_OP_WRITE_BLOCK:
					diskio_write_block_file( block_start_address, buffer );
					break;
				case DISKIO_OP_WRITE_BLOCKS:
					diskio_write_blocks_file( block_start_address, num_blocks, buffer );
					break;
				default:
					return DISKIO_ERROR_OPERATION_NOT_SUPPORTED;
					break;
			}
			#endif
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
	int i = 0;
#ifndef DISKIO_DEBUG
	if( microSD_init() == 0 ) {
		devices[dev_num].type = DISKIO_DEVICE_TYPE_SD_CARD;
		devices[dev_num].number = dev_num;		
		devices[dev_num].num_sectors = microSD_query( MICROSD_CARD_SIZE );
		devices[dev_num].sector_size = microSD_query( MICROSD_CARD_SECTOR_SIZE );
		devices[dev_num].first_sector = 0;
		mbr_init( &mbr );
		mbr_read( devices[0], &mbr );
		for( i = 0; i < 4; ++i ) {
			if( mbr_hasPartition( mbr, i + 1 ) == TRUE ) {
				devices[dev_num + i + 1].type = DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION;
				devices[dev_num + i + 1].number = dev_num;
				devices[dev_num + i + 1].partition = i + 1;
				devices[dev_num + i + 1].num_sectors = mbr.partition[i].lba_num_sectors;
				devices[dev_num + i + 1].sector_size = devices[dev_num].sector_size;
				devices[dev_num + i + 1].first_sector = mbr.partition[i].lba_first_sector;
			}
		}
		dev_num += 1;
	}
#else
	if( !handle ) {
		handle = fopen( DISKIO_DEBUG_FILE_NAME, "rwb" );
		if( !handle ) {
			/*CRAP!*/
			//exit(1);
		}
	}
	devices[dev_num].type == DISKIO_DEVICE_TYPE_FILE;
	devices[dev_num].number = dev_num;
	devices[dev_num].num_sectors = DISKIO_DEBUG_FILE_NUM_SECTORS;
	devices[dev_num].sector_size = DISKIO_DEBUG_FILE_SECTOR_SIZE;
	devices[dev_num].first_sector = 0;
	mbr_init( &mbr );
	mbr_read( &devices[0], &mbr );
	for( i = 0; i < 4; ++i ) {
		if( mbr_hasPartition( &mbr, i + 1 ) == TRUE ) {
			devices[dev_num + i + 1].type = DISKIO_DEVICE_TYPE_FILE | DISKIO_DEVICE_TYPE_PARTITION;
			devices[dev_num + i + 1].number = dev_num;
			devices[dev_num + i + 1].partition = i + 1;
			devices[dev_num + i + 1].num_sectors = mbr.partition[i].lba_num_sectors;
			devices[dev_num + i + 1].sector_size = devices[dev_num].sector_size;
			devices[dev_num + i + 1].first_sector = mbr.partition[i].lba_first_sector;
		}
	}
	dev_num += 1;
#endif
	return 0;
}
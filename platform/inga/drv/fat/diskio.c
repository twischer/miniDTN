#include "diskio.h"
#include "mbr.h"
#ifndef DISKIO_DEBUG
#include "../../interfaces/flash-microSD.h"
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
	fflush( handle);
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
	fflush( handle);
}

void diskio_write_block_file( uint32_t block_start_address, uint8_t *buffer ) {
	uint16_t i;
	fseek( handle, block_start_address * DISKIO_DEBUG_FILE_SECTOR_SIZE, SEEK_SET );
	for( i = 0; i < DISKIO_DEBUG_FILE_SECTOR_SIZE; ++i ) {
		putc( buffer[i], handle );
	}
	fflush( handle);
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
	fflush( handle);
}
#endif

#define DISKIO_OP_WRITE_BLOCK  1
#define DISKIO_OP_READ_BLOCK   2
#define DISKIO_OP_WRITE_BLOCKS 3
#define DISKIO_OP_READ_BLOCKS  4

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
		case DISKIO_DEVICE_TYPE_FILE:
			printf("File");
			break;
		default:
			printf("Unknown");
	}
	if(dev->type & DISKIO_DEVICE_TYPE_PARTITION)
		printf(" (Partition)");
	printf("\n");
	printf("\tnumber = %d\n", dev->number);
	printf("\tpartition = %d\n", dev->partition);
	printf("\tnum_sectors = %ld\n", dev->num_sectors);
	printf("\tsector_size = %d\n", dev->sector_size);
	printf("\tfirst_sector = %ld\n", dev->first_sector);
}

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
			#ifndef DISKIO_DEBUG
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
			#endif
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
	int i = 0, index = 0;

	memset( devices, 0, DISKIO_MAX_DEVICES * sizeof(struct diskio_device_info) );
#ifndef DISKIO_DEBUG
	if( microSD_init() == 0 ) {
		devices[index].type = DISKIO_DEVICE_TYPE_SD_CARD;
		devices[index].number = dev_num;		
		devices[index].num_sectors = microSD_get_card_block_count();
		devices[index].sector_size = microSD_get_block_size();
		devices[index].first_sector = 0;
		if( devices[index].sector_size > DISKIO_MAX_SECTOR_SIZE )
			return DISKIO_FAILURE;

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
	} else {
		return DISKIO_FAILURE;
	}
#else
	if( !handle ) {
		handle = fopen( DISKIO_DEBUG_FILE_NAME, "r+b" );
		if( !handle ) {
			/*CRAP!*/
			return DISKIO_FAILURE;
		}
	}
	devices[index].type = DISKIO_DEVICE_TYPE_FILE;
	devices[index].number = dev_num;
	devices[index].num_sectors = DISKIO_DEBUG_FILE_NUM_SECTORS;
	devices[index].sector_size = DISKIO_DEBUG_FILE_SECTOR_SIZE;
	devices[index].first_sector = 0;
	mbr_init( &mbr );
	mbr_read( &devices[index], &mbr );
	index += 1;
	for( i = 0; i < 4; ++i ) {
		if( mbr_hasPartition( &mbr, i + 1 ) == TRUE ) {
			devices[index].type = DISKIO_DEVICE_TYPE_FILE | DISKIO_DEVICE_TYPE_PARTITION;
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
#endif
	return DISKIO_SUCCESS;
}

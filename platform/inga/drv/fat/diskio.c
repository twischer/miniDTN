#include "diskio.h"
#include "../../interfaces/flash_microSD.h"

static struct diskio_device_info *default_device = NULL;

int diskio_rw_op( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer, uint8_t op );

int diskio_read_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer ) {
	return diskio_rw_op( dev, block_address, 1, buffer, DISKIO_OP_WRITE_BLOCK );
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
		if( default_device == NULL )
			return DISKIO_ERROR_NO_DEVICE_SELECTED;
		dev = default_device;
	}
	switch( dev->type ) {
		case diskio_device_type_SD_CARD:
			switch( op ) {
				case DISKIO_OP_READ_BLOCK:
					uint8_t ret_code = microSD_read_block( block_start_address, buffer );
					if( ret_code != 0 )
						return DISKIO_ERROR_INTERNAL_ERROR;
					break;
				case DISKIO_OP_READ_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				case DISKIO_OP_WRITE_BLOCK:
					uint8_t ret_code = microSD_write_block( block_start_address, buffer );
					if( ret_code != 0 )
						return DISKIO_ERROR_INTERNAL_ERROR;
					break;
				case DISKIO_OP_WRITE_BLOCKS:
					return DISKIO_ERROR_TO_BE_IMPLEMENTED;
					break;
				default:
					return DISKIO_ERROR_NOT_SUPPORTED;
					break;
			}
			break;
		case diskio_device_type_GENERIC_FLASH:
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
		case diskio_device_type_NOT_RECOGNIZED:
		default:
			return DISKIO_ERROR_DEVICE_TYPE_NOT_RECOGNIZED;
	}
	return DISKIO_SUCCESS;
}

int diskio_set_default_device( struct diskio_device_info *dev ) {
	default_device = dev;
}

struct diskio_device_info * diskio_devices() {
	static struct diskio_device_info devices[DISKIO_MAX_DEVICES];
	static struct mbr mbr;
	// IMPLEMENT
	mbr_init( &mbr, microSD_get_card_size() );
	mbr_read( devices[i], &mbr );
	for( int i = 0; i < 4; ++i ) {
		if( mbr_hasPartition( mbr, i + 1 ) == TRUE ) {
			// make partition copy of current dev
		}
	}
}
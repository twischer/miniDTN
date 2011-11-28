#include "mbr.h"

static uint8_t mbr_buffer[512];

int mbr_init( struct diskio_device_info *dev ) {}

int mbr_part_info_cpy_from_buffer( uint8_t part_num, struct mbr *to) {
	static uint8_t start_index = 446 + 16 * (part_num - 1);
	static struct mbr_primary_partition *part = &(to->partition[part_num - 1]);
	part->status = mbr_buffer[start_index];
	part->chs_first_sector[0] 	= mbr_buffer[start_index + 1];
	part->chs_first_sector[1] 	= mbr_buffer[start_index + 2];
	part->chs_first_sector[2] 	= mbr_buffer[start_index + 3];
	part->type 					= mbr_buffer[start_index + 4];
	part->chs_last_sector[0] 	= mbr_buffer[start_index + 5];
	part->chs_last_sector[1] 	= mbr_buffer[start_index + 6];
	part->chs_last_sector[2] 	= mbr_buffer[start_index + 7];
	part->lba_first_sector 		= ((uint32_t) mbr_buffer[start_index + 11]) << 24 +  ((uint32_t) mbr_buffer[start_index + 10]) << 16 +  ((uint32_t) mbr_buffer[start_index + 9]) << 8 + mbr_buffer[start_index + 8];
	part->lba_num_sectors 		= ((uint32_t) mbr_buffer[start_index + 15]) << 24 +  ((uint32_t) mbr_buffer[start_index + 14]) << 16 +  ((uint32_t) mbr_buffer[start_index + 13]) << 8 + mbr_buffer[start_index + 12];
}

int mbr_read( struct diskio_device_info *from, struct mbr *to) {
	static int ret = diskio_read_block( from, 0, *mbr_buffer );
	if( ret != 0 ) return MBR_ERROR_DISKIO_ERROR;
	/*test if 0x55AA is at the end, otherwise it is no MBR*/
	if( mbr_buffer[510] == 0x55 && mbr_buffer[511] == 0xAA ) {
		mbr_part_info_cpy_from_buffer( 1, to );
		mbr_part_info_cpy_from_buffer( 2, to );
		mbr_part_info_cpy_from_buffer( 3, to );
		mbr_part_info_cpy_from_buffer( 4, to );
		return MBR_SUCCESS;
	}
	return MBR_ERROR_NO_MBR_FOUND;
}
int mbr_write( struct mbr *from, struct diskio_device_info *to );

int mbr_addPartition(struct mbr *mbr, uint8_t part_num, uint32_t start, uint32_t len );
int mbr_delPartition(struct mbr *mbr, uint8_t part_num);
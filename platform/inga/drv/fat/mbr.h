#ifndef _MBR_H_
#define _MBR_H_

#include "diskio.h"

#define MBR_SUCCESS 0
#define MBR_ERROR_DISKIO_ERROR 1
#define MBR_ERROR_NO_MBR_FOUND 2

struct mbr_primary_partition {
	uint8_t status;
	uint8_t chs_first_sector[3];
	uint8_t type;
	uint8_t chs_last_sector[3];
	uint32_t lba_first_sector;
	uint32_t lba_num_sectors;
};

struct mbr { //ignores everything but primary partitions (saves 448 bytes)
	struct mbr_primary_partition partition[4];
};

int mbr_init( struct diskio_device_info *dev );

int mbr_read( struct diskio_device_info *from, struct mbr *to);
int mbr_write( struct mbr *from, struct diskio_device_info *to );

int mbr_addPartition(struct mbr *mbr, uint8_t part_num, uint32_t start, uint32_t len );
int mbr_delPartition(struct mbr *mbr, uint8_t part_num);

#endif

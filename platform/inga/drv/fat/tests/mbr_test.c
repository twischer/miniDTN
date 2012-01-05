#include "../mbr.h"

struct diskio_device_info *dev = 0;

void setDevice() {
	int i = 0;
	diskio_detect_devices();
	dev = diskio_devices();
	for(i = 0; i < DISKIO_MAX_DEVICES; i++) {
		if( (dev + i)->type == DISKIO_DEVICE_TYPE_FILE ) {
			dev += i;
			break;
		}
	}
}

int main() {
	struct mbr mbr;
	setDevice();
	mbr_init( &mbr );
	mbr_addPartition( &mbr, 1, MBR_PARTITION_TYPE_FAT32_LBA, 0, 248 );
	mbr_addPartition( &mbr, 2, MBR_PARTITION_TYPE_FAT32_LBA, 248, 512 );
	mbr_write( &mbr, dev );
	mbr_init( &mbr );
	mbr_read( dev, &mbr );
	if( mbr.partition[0].type != MBR_PARTITION_TYPE_FAT32_LBA || mbr.partition[1].type != MBR_PARTITION_TYPE_FAT32_LBA ||
		mbr.partition[0].lba_first_sector != 0 || mbr.partition[0].lba_num_sectors != 248 ||
		mbr.partition[1].lba_first_sector != 248 || mbr.partition[1].lba_num_sectors != 512 )
		return 1;
	return 0;
}

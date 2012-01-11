#ifndef _FAT_H_
#define _FAT_H_

#include "diskio.h"

#define FAT12 0
#define FAT16 1
#define FAT32 2
#define FAT_INVALID 3

#define EOC 0x0FFFFFFF

struct FAT_Info {
	uint8_t type; /** Either FAT16, FAT32 or FAT_INVALID */
	uint16_t BPB_BytesPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint32_t BPB_TotSec;
	uint8_t BPB_Media;
	uint32_t BPB_FATSz;
	uint32_t BPB_RootClus; /** only valid for FAT32 */
};

int mkfs_fat( struct diskio_device_info *dev );

int fat_mkdir(char *);
int fat_rmdir(char *);

/**
 * Tries to mount the defined device.
 *
 * \param dev The device on which a FAT-FS should be mounted.
 * \return 0 on success, 1 if the bootsector was not found or corrupted, 2 if the FAT-Type wasn't supported.
 */
uint8_t fat_mount_device( struct diskio_device_info *dev );
void fat_umount_device();

#endif

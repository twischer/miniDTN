#ifndef _FAT_H_
#define _FAT_H_

#include "diskio.h"

#define FAT12 0
#define FAT16 1
#define FAT32 2
#define FAT_INVALID 3

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
};

int mkfs_fat( struct diskio_device_info *dev );

int fat_mkdir(char *);
int fat_rmdir(char *);

#endif

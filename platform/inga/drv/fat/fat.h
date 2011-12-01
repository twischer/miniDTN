#ifndef _FAT_H_
#define _FAT_H_

#include "diskio.h"

#define FAT_TYPE_FAT16 1
#define FAT_TYPE_FAT32 2

int mkfs_fat16( struct diskio_device_info *dev );
int mkfs_fat32( struct diskio_device_info *dev );

int fat_mkdir(char *);
int fat_rmdir(char *);

#endif

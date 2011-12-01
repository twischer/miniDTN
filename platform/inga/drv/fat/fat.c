#include <stdint.h>
#include <core/cfs/cfs.h>
#include "fat.h"
#include <string.h>

#define FAT_FD_POOL_SIZE 5

struct file {
	//metadata
	//position on disk
};

struct file_desc {
	cfs_offset_t offset;
	struct file *file;
	uint8_t flags;
};

struct file_desc fat_fd_pool[FAT_FD_POOL_SIZE];

int
cfs_open(const char *name, int flags)
{
	// get FileDescriptor
	// find file on Disk
	// put read/write position in the right spot
	// return FileDescriptor
}

void
cfs_close(int fd)
{
}


int
cfs_read(int fd, void *buf, unsigned int len)
{
}

int
cfs_write(int fd, const void *buf, unsigned int len)
{
}

cfs_offset_t
cfs_seek(int fd, cfs_offset_t offset, int whence)
{
}

int
cfs_remove(const char *name)
{
}

int
cfs_opendir(struct cfs_dir *dirp, const char *name)
{
}

int
cfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent)
{
}

void
cfs_closedir(cfs_dir *dirp)
{
}

void set_boot_sector( uint8_t *buffer, uint16_t sec_size, uint32_t total_sec_size, uint8_t fat_type ) {
	// BS_jmpBoot
	buffer[0] = 0x00; buffer[1] = 0x00; buffer[2] = 0x00;
	// BS_OEMName
	memset(  &(buffer[3]), "MSWIN4.1", 8 );
	// BPB_BytesPerSec
	buffer[11] = (uint8_t) sec_size; buffer[12] = sec_size >> 8;
	// BPB_SecPerClus //ToDo setze diesen Wert dynamisch
	buffer[13] = 1;
	//BPB_RsvdSecCnt
	buffer[14] = 1; buffer[15] = 0; // Für FAT16, für FAT32 ist es typ 32, ebenfalls dynamisch
	// BPB_NumFATs
	buffer[16] = 2;
	// BPB_RootEntCnt
	if( fat_type == FAT16 ) {
		buffer[17] = (uint8_t) 512; buffer[18] = (uint8_t) 512 >> 8;
	} else if( fat_type == FAT32 ) {
		buffer[17] = 0; buffer[18] = 0;
	}
	// BPB_TotSec16
	if( total_sec_size < 0x10000 ) {
		buffer[19] = (uint8_t) total_sec_size; buffer[20] = (uint8_t) total_sec_size >> 8;
	} else {
		buffer[19] = 0; buffer[20] = 0;
	}
	// BPB_Media
	buffer[21] = 0xF0; //for removable media, set dynamic
	// BPB_FATSz16
	buffer[22] = ; buffer[23] = ;
	// BPB_SecPerTrk
	buffer[24] = 0; buffer[25] = 0;
	// BPB_NumHeads
	buffer[26] = 0; buffer[27] = 0;
	// BPB_HiddSec
	buffer[28] = 0; buffer[29] = 0; buffer[30] = 0; buffer[31] = 0;
	// BPB_TotSec32
	if( total_sec_size < 0x10000 ) {
		buffer[32] = 0; buffer[33] = 0; buffer[34] = 0; buffer[35] = 0;
	} else {
		buffer[32] = (uint8_t) total_sec_size      ; buffer[33] = (uint8_t) total_sec_size >>  8;
		buffer[34] = (uint8_t) total_sec_size >> 16; buffer[35] = (uint8_t) total_sec_size >> 24;
	}
	// Specification demands this values at this precise positions
	buffer[510] = 0x55; buffer[511] = 0xAA;
}

int mkfs_fat( struct diskio_device_info *dev, uint16_t sec_size = 512 ) {
	static uint8_t buffer[512];
	memset( buffer, 0, 512 );
	uint8_t offset = 0;
	set_boot_sector( buffer );
}

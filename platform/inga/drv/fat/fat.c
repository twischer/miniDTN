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

uint8_t determine_fat_type( uint32_t TotSec, uint16_t ResvdSecCnt, uint8_t NumFATs, uint32_t FATSz, uint32_t RootDirSectors, uint8_t SecPerClus ) {
	uint32_t DataSec = TotSec - (ResvdSecCnt + (NumFATs * FATSz) + RootDirSectors;
	uint32_t CountofClusters = DataSec / SecPerClus;
	If(CountofClusters < 4085) {
		/* Volume is FAT12 */
		return FAT12;
	} else if(CountofClusters < 65525) {
		/* Volume is FAT16 */
		return FAT16;
	} else {
		/* Volume is FAT32 */
		return FAT32;
	}
}

uint8_t SPC( uint16_t sec_size, uint16_t bytes ) {
	uint8_t SecPerClus = 0;
	SecPerCluster = 1024 / sec_size;
	if( SecPerCluster == 0)
		SecPerCluster = 1;
	return SecPerCluster;
}

uint8_t init_lookup_table( uint32_t total_sec_count, uint16_t bytes_per_sec ) {
	uint64_t vol_size = (total_sec_count * bytes_per_sec) / 512;
	if( vol_size < 8400 * 512 ) {
		return FAT12 << 6 + 0;
	} else if( vol_size < 32680 ) {
		return FAT16 << 6 + SPC(bytes_per_sec,1024);
	} else if( vol_size < 262144 ) {
		return FAT16 << 6 + SPC(bytes_per_sec,2048);
	} else if( vol_size < 524288 ) {
		return FAT16 << 6 + SPC(bytes_per_sec,4096);
	} else if( vol_size < 1048576 ) {
		return FAT16 << 6 + SPC(bytes_per_sec,8192);
	} else if( vol_size < 2097152 ) {
		return FAT16 << 6 + SPC(bytes_per_sec,16384);
	} else if( vol_size < 4194304 ) {
		return FAT16 << 6 + SPC(bytes_per_sec,32768);
	}
	
	if( vol_size < 16777216 ) {
		return FAT32 << 6 + SPC(bytes_per_sec,4096);
	} else if( vol_size < 33554432 ) {
		return FAT32 << 6 + SPC(bytes_per_sec,8192);
	} else if( vol_size < 67108864 ) {
		return FAT32 << 6 + SPC(bytes_per_sec,16384);
	} else if( vol_size < 0xFFFFFFFF ) {
		return FAT32 << 6 + SPC(bytes_per_sec,32768);
	} else {
		return FAT_INVALID << 6 + 0;
	}
}

uint32_t compute_fat_size( uint8_t fat_type, uint16_t root_entry_count, uint16_t sec_size,
                           uint8_t resvd_sectors, uint8_t sectors_per_cluster, uint8_t num_fats,
						   uint32_t dsk_size ) {
	uint16_t RootDirSectors = ((root_entry_count * 32) + (sec_size - 1)) / sec_size;
	uint32_t TmpVal1 = dsk_size - (resvd_sectors + RootDirSectors);
	uint32_t TmpVal2 = (256 * sec_size) + num_fats;
	uint32_t FATSz = 0;
	if( fat_type == FAT32 )
		TmpVal2 /= 2;
	FATSz = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
	return FATSz;
}

void set_boot_sector( uint8_t *buffer, uint16_t sec_size, uint32_t total_sec_count ) {
	// Test if we can make FAT16 or FAT32
	uint8_t fat_type = init_lookup_table( total_sec_count, sec_size ); 
	uint8_t sectors_per_cluster = (fat_type << 2) >> 2;
	uint32_t FATsz = 0;
	fat_type = fat_type >> 6;
	
	if( fat_type == FAT12 || fat_type == FAT_INVALID ) {
		// ERROR, can't format this media
	}
	// BS_jmpBoot
	buffer[0] = 0x00; buffer[1] = 0x00; buffer[2] = 0x00;
	
	// BS_OEMName
	memset(  &(buffer[3]), "MSWIN4.1", 8 );
	
	// BPB_BytesPerSec
	buffer[11] = (uint8_t) sec_size; buffer[12] = (uint8_t) sec_size >> 8;
	
	// BPB_SecPerClus
	buffer[13] = sectors_per_cluster;
	
	//BPB_RsvdSecCnt
	if( fat_type == FAT16) {
		buffer[14] = 1; buffer[15] = 0;
	} else if( fat_type == FAT32 ) {
		buffer[14] = 32; buffer[15] = 0;
	}
	
	// BPB_NumFATs
	buffer[16] = 2;
	
	// BPB_RootEntCnt
	if( fat_type == FAT16 ) {
		buffer[17] = (uint8_t) 512; buffer[18] = (uint8_t) 512 >> 8;
	} else if( fat_type == FAT32 ) {
		buffer[17] = 0; buffer[18] = 0;
	}
	
	// BPB_TotSec16
	if( total_sec_count < 0x10000 ) {
		buffer[19] = (uint8_t) total_sec_count; buffer[20] = (uint8_t) total_sec_count >> 8;
	} else {
		buffer[19] = 0; buffer[20] = 0;
	}
	
	// BPB_Media
	if( dev->type & DISKIO_DEVICE_TYPE_SD_CARD )
		buffer[21] = 0xF0;
	else
		buffer[21] = 0xF8;
		
	// BPB_FATSz16
	FATsz = compute_fat_size( fat_type,
		(((uint16_t)buffer[18]) << 8) + buffer[17], 
		sec_size,
		buffer[14],
		sectors_per_cluster,
		buffer[16],
		total_sec_count );
	if( fat_type == FAT16 && FATsz < 0x10000) {
		buffer[22] = (uint8_t) FATsz; buffer[23] = (uint8_t) (FATsz >> 8);
	} else if( fat_type == FAT16 ) {
		// MASSIVE ERROR
	} else {
		buffer[22] = 0; buffer[23] = 0;
	}
	
	// BPB_SecPerTrk
	buffer[24] = 0; buffer[25] = 0;
	
	// BPB_NumHeads
	buffer[26] = 0; buffer[27] = 0;
	
	// BPB_HiddSec
	buffer[28] = 0; buffer[29] = 0; buffer[30] = 0; buffer[31] = 0;
	
	// BPB_TotSec32
	if( total_sec_count < 0x10000 ) {
		buffer[32] = 0; buffer[33] = 0; buffer[34] = 0; buffer[35] = 0;
	} else {
		buffer[32] = (uint8_t) total_sec_count      ; buffer[33] = (uint8_t) total_sec_count >>  8;
		buffer[34] = (uint8_t) total_sec_count >> 16; buffer[35] = (uint8_t) total_sec_count >> 24;
	}
	
	if( fat_type == FAT16 ) {
		// BS_DrvNum
		buffer[36] = 0x80;
		
		// BS_Reserved1
		buffer[37] = 0;
		
		// BS_BootSig
		buffer[38] = 0x29;
		
		// BS_VolID
		buffer[39] = 0; buffer[40] = 0; buffer[41] = 0; buffer[42] = 0;
		
		// BS_VolLab
		memset(  &(buffer[43]), "NO NAME    ", 11 );
		
		// BS_FilSysType
		memset(  &(buffer[54]), "FAT16   ", 8 );
	} else if( fat_type == FAT32 ) {
		// BPB_FATSz32
		buffer[36] = (uint8_t) FATsz      ; buffer[37] = (uint8_t) FATsz >>  8;
		buffer[38] = (uint8_t) FATsz >> 16; buffer[39] = (uint8_t) FATsz >> 24;
		
		// BPB_ExtFlags
		buffer[40] = 0; buffer[41] = 0; // Mirror enabled
		
		// BPB_FSVer
		buffer[42] = 0; buffer[43] = 0;
		
		// BPB_RootClus
		buffer[44] = 2; buffer[45] = 0; buffer[46] = 0; buffer[47] = 0;
		
		// BPB_FSInfo
		buffer[48] = 1; buffer[49] = 0;
		
		// BPB_BkBootSec
		buffer[50] = 0; buffer[51] = 0;
		
		// BPB_Reserved
		memset(  &(buffer[52]), 0, 12 );
		
		// BS_DrvNum
		buffer[64] = 0x80;
		
		// BS_Reserved1
		buffer[65] = 0;
		
		// BS_BootSig
		buffer[66] = 0x29;
		
		// BS_VolID
		buffer[67] = 0; buffer[68] = 0; buffer[69] = 0; buffer[70] = 0;
		
		// BS_VolLab
		memset(  &(buffer[71]), "NO NAME    ", 11 );
		
		// BS_FilSysType
		memset(  &(buffer[82]), "FAT32   ", 8 );		
	}
	
	// Specification demands this values at this precise positions
	buffer[510] = 0x55; buffer[511] = 0xAA;
}

int mkfs_fat( struct diskio_device_info *dev ) {
	static uint8_t buffer[512];
	memset( buffer, 0, 512 );		
	set_boot_sector( buffer, dev->sector_size, dev->num_sectors );
}

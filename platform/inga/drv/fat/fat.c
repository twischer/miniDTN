#include <stdint.h>
#include <core/cfs/cfs.h>
#include "fat.h"
#include <string.h>

#define FAT_FD_POOL_SIZE 5

uint8_t buffer

struct file {
	//metadata
	/** Cluster Position on disk */
	uint32_t cluster;
};

struct file_desc {
	cfs_offset_t offset;
	struct file *file;
	uint8_t flags;
};

struct file_desc fat_fd_pool[FAT_FD_POOL_SIZE];

struct dir_entry {
	uint8_t DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t CrtTimeTenth;
	uint8_t unknown[6];
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
};

uint8_t lookup( const char *path, int start_idx, int end_idx, struct dir_entry *dir_entry ) {

}

uint32_t find_file_cluster( const char *path ) {
#ifdef DISKIO_PATH_SUPPORT
	//: /[DirNum]/[Directories]/[name].[ext]
#else
	int start_idx = 0, end_idx = 0, i;
	struct dir_entry dir_entry;
	init( dir_entry );
	for( i = 0; i < strlen(path); ++i ) {
		if( start_idx == end_idx && path[i] != '/' ) {
			start_idx = i;
			end_idx = i - 1;
		} else if( start_idx == end_idx && path[i] == '/' ) {
			continue;
		} else if( start_idx != end_idx && (path[i] == '/' || path[i] == '\0')) {
			end_idx = i;
			if(!lookup( path, start_idx, end_idx, &dir_entry )) {
				return 0;
			}
			if( path[i] == '\0' ) {
				return (((uint32_t)dir_entry.DIR_FstClusHI) << 16) + dir_entry.DIR_FstClusLO;
			}
		}
	}
	return 0;
#endif
}

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

/**
 * Determines the type of the fat by using the BPB.
 */
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

/**
 * Tests if the given value is a power of 2.
 *
 * \param value Number which should be testet if it is a power of 2.
 * \return TRUE or FALSE.
 */
uint8_t is_a_power_of_2( uint32_t value ) {
	uint32_t test = 1;
	uint8_t i = 0;
	if( value == 0 )
		return TRUE;
	for( i = 0; i < 32; ++i ) {
		test << 1;
		if( test == value )
			return TRUE;
	}
	return FALSE;
}

/**
 * Rounds the value down to the next lower power of 2.
 *
 * \param value The number which should be rounded down.
 * \return the next lower number which is a power of 2
 */
uint32_t round_down_to_power_of_2( uint32_t value ) {
	uint32_t po2 = 1 << 31;
	while( value < po2 ) {
		po2 >> 1;
	}
	return po2;
}

/**
 * Determines the number of sectors per cluster.
 *
 * \param sec_size The size of one sector in bytes.
 * \param bytes number of bytes one cluster should be big
 * \return 1,2,4,8,16,32,64,128 depending on the number of sectors per cluster
 */
uint8_t SPC( uint16_t sec_size, uint16_t bytes ) {
	uint8_t SecPerClus = 0;
	SecPerCluster = (uint8_t)(bytes / sec_size);
	if( SecPerCluster == 0 )
		return 1;

	if( is_a_power_of_2( SecPerCluster ) == FALSE )
		SecPerCluster = (uint8_t) round_down_to_power_of_2( SecPerCluster );
	
	if( SecPerCluster > 128 || SecPerCluster * sec_size > 32 * 1024 )
		return 1;
	
	return SecPerCluster;
}

/**
 * Determines for mkfs which FAT-Type and cluster-size should be used.
 *
 * This is based upon the FAT specification, which states that the table only works for 512
 * Bytes sized sectors. For that the function SPC is used to compute this right. but
 * it probalby only works for powers of 2 in the bytes_per_sec field. It tries to use FAT16
 * if at all possible, FAT32 only when there is no other choice.
 * \param total_sec_count the total number of sectors of the filesystem
 * \param bytes_per_sec how many bytes are there in one sector
 * \return in the upper two bits is the fat type, either FAT12, FAT16, FAT32 or FAT_INVALID. In the lower 8 bits is the number of sectors per cluster.
 */
uint16_t mkfs_determine_fat_type_and_SPC( uint32_t total_sec_count, uint16_t bytes_per_sec ) {
	uint64_t vol_size = (total_sec_count * bytes_per_sec) / 512;
	if( vol_size < 8400 * 512 ) {
		return FAT12 << 8 + 0;
	} else if( vol_size < 32680 ) {
		return FAT16 << 8 + SPC(bytes_per_sec,1024);
	} else if( vol_size < 262144 ) {
		return FAT16 << 8 + SPC(bytes_per_sec,2048);
	} else if( vol_size < 524288 ) {
		return FAT16 << 8 + SPC(bytes_per_sec,4096);
	} else if( vol_size < 1048576 ) {
		return FAT16 << 8 + SPC(bytes_per_sec,8192);
	} else if( vol_size < 2097152 ) {
		return FAT16 << 8 + SPC(bytes_per_sec,16384);
	} else if( vol_size < 4194304 ) {
		return FAT16 << 8 + SPC(bytes_per_sec,32768);
	}
	
	if( vol_size < 16777216 ) {
		return FAT32 << 8 + SPC(bytes_per_sec,4096);
	} else if( vol_size < 33554432 ) {
		return FAT32 << 8 + SPC(bytes_per_sec,8192);
	} else if( vol_size < 67108864 ) {
		return FAT32 << 8 + SPC(bytes_per_sec,16384);
	} else if( vol_size < 0xFFFFFFFF ) {
		return FAT32 << 8 + SPC(bytes_per_sec,32768);
	} else {
		return FAT_INVALID << 8 + 0;
	}
}

/**
 * Computes the Size of one FAT. Only needed for mkfs.
 *
 * \param fi FAT_Info structure that must contain BPB_RootEntCnt, BPB_BytesPerSec, BPB_TotSec, BPB_RsvdSecCnt, BPB_NumFATs and type.
 * \return the Size of one FAT for this FS.
 */
uint32_t mkfs_compute_fat_size( struct FAT_Info *fi ) {
	uint16_t RootDirSectors = ((fi->BPB_RootEntCnt * 32) + (fi->BPB_BytesPerSec - 1)) / fi->BPB_BytesPerSec;
	uint32_t TmpVal1 = fi->BPB_TotSec - (fi->BPB_RsvdSecCnt + RootDirSectors);
	uint32_t TmpVal2 = (256 * fi->BytesPerSec) + fi->BPB_NumFATs;
	uint32_t FATSz = 0;
	if( fi->type == FAT32 )
		TmpVal2 /= 2;
	FATSz = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
	return FATSz;
}

int mkfs_write_boot_sector( uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi ) {
	// Test if we can make FAT16 or FAT32
	uint16_t type_SPC = mkfs_determine_fat_type_and_SPC( dev->num_sectors, dev->sector_size ); 
	uint8_t sectors_per_cluster = (uint8_t) type_SPC;
	fi->BPB_FATsz = 0;
	fi->type = (uint8_t) type_SPC >> 8;
	
	if( fi->type == FAT12 || fi->type == FAT_INVALID ) {
		return -1;		
	}
	
	// BS_jmpBoot
	buffer[0] = 0x00; buffer[1] = 0x00; buffer[2] = 0x00;
	
	// BS_OEMName
	memcpy(  &(buffer[3]), "MSWIN4.1", 8 );
	
	// BPB_BytesPerSec
	buffer[11] = (uint8_t) dev->sector_size; buffer[12] = (uint8_t) dev->sector_size >> 8;
	fi->BPB_BytesPerSec = dev->sector_size;
	
	// BPB_SecPerClus
	buffer[13] = sectors_per_cluster;
	fi->BPB_SecPerClus = sectors_per_cluster;
	
	//BPB_RsvdSecCnt
	if( fat_type == FAT16 ) {
		buffer[14] = 1; buffer[15] = 0;
		fi->BPB_RsvdSecCnt = 1;
	} else if( fat_type == FAT32 ) {
		buffer[14] = 32; buffer[15] = 0;
		fi->BPB_RsvdSecCnt = 32;
	}
	
	// BPB_NumFATs
	buffer[16] = 2;
	fi->BPB_NumFATs = 2;
	
	// BPB_RootEntCnt
	if( fat_type == FAT16 ) {
		buffer[17] = (uint8_t) 512; buffer[18] = (uint8_t) 512 >> 8;
		fi->BPB_RootEntCnt = 512;
	} else if( fat_type == FAT32 ) {
		buffer[17] = 0; buffer[18] = 0;
		fi->BPB_RootEntCnt = 0;
	}
	
	// BPB_TotSec16
	if( dev->num_sectors < 0x10000 ) {
		buffer[19] = (uint8_t) dev->num_sectors; buffer[20] = (uint8_t) dev->num_sectors >> 8;
	} else {
		buffer[19] = 0; buffer[20] = 0;
	}
	fi->BPB_TotSec = dev->num_sectors;
	
	// BPB_Media
	if( dev->type & DISKIO_DEVICE_TYPE_SD_CARD ){ 
		buffer[21] = 0xF0;
		fi->BPB_Media = 0xF0;
	} else {
		buffer[21] = 0xF8;
		fi->BPB_Media = 0xF8;
	}
		
	// BPB_FATSz16
	fi->BPB_FATSz = mkfs_compute_fat_size( fi );
	
	if( fi->type == FAT16 && fi->BPB_FATSz < 0x10000) {
		buffer[22] = (uint8_t) fi->BPB_FATSz; buffer[23] = (uint8_t) (fi->BPB_FATSz >> 8);
	} else if( fat_type == FAT16 ) {
		return -1;
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
	if( dev->num_sectors < 0x10000 ) {
		buffer[32] = 0; buffer[33] = 0; buffer[34] = 0; buffer[35] = 0;
	} else {
		buffer[32] = (uint8_t) fi->BPB_TotSec      ; buffer[33] = (uint8_t) fi->BPB_TotSec >>  8;
		buffer[34] = (uint8_t) fi->BPB_TotSec >> 16; buffer[35] = (uint8_t) fi->BPB_TotSec >> 24;
	}
	
	if( fi->type == FAT16 ) {
		// BS_DrvNum
		buffer[36] = 0x80;
		
		// BS_Reserved1
		buffer[37] = 0;
		
		// BS_BootSig
		buffer[38] = 0x29;
		
		// BS_VolID
		buffer[39] = 0; buffer[40] = 0; buffer[41] = 0; buffer[42] = 0;
		
		// BS_VolLab
		memcpy(  &(buffer[43]), "NO NAME    ", 11 );
		
		// BS_FilSysType
		memcpy(  &(buffer[54]), "FAT16   ", 8 );
	} else if( fi->type == FAT32 ) {
		// BPB_FATSz32
		buffer[36] = (uint8_t) fi->BPB_FATSz      ; buffer[37] = (uint8_t) fi->BPB_FATSz >>  8;
		buffer[38] = (uint8_t) fi->BPB_FATSz >> 16; buffer[39] = (uint8_t) fi->BPB_FATSz >> 24;
		
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
		memset( &(buffer[52]), 0, 12 );
		
		// BS_DrvNum
		buffer[64] = 0x80;
		
		// BS_Reserved1
		buffer[65] = 0;
		
		// BS_BootSig
		buffer[66] = 0x29;
		
		// BS_VolID
		buffer[67] = 0; buffer[68] = 0; buffer[69] = 0; buffer[70] = 0;
		
		// BS_VolLab
		memcpy( &(buffer[71]), "NO NAME    ", 11 );
		
		// BS_FilSysType
		memcpy( &(buffer[82]), "FAT32   ", 8 );		
	}
	
	// Specification demands this values at this precise positions
	buffer[510] = 0x55; buffer[511] = 0xAA;
	
	diskio_write_block( dev, 0, buffer );
	
	return 0;
}

/**
 * Writes the FAT-Portions of the FS.
 *
 * \param buffer Buffer / 2 and buffer / 4 should be integers.
 * \param dev the Device where the FATs are written to
 */
void mkfs_write_fats( uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi ) {
	uint32_t *fat32_buf = buffer;
	uint16_t *fat16_buf = buffer;
	uint32_t i = 0, j = 0;
	
	if( fi->type == FAT32 ) {
		// BPB_Media Copy
		fat32_buf[0] = 0x0FFFFF00 + fi->BPB_Media;
	
		// End of Clusterchain marker
		fat32_buf[1] = 0x0FFFFFF8;
	} else {
		// BPB_Media Copy
		fat16_buf[0] = 0xFF00 + fi->BPB_Media;
	
		// End of Clusterchain marker
		fat16_buf[1] = 0xFFF8;
	}
	
	// Write first sector of the FATs
	for( j = 0; j < fi->BPB_NumFATs; ++j ) {
		diskio_write_block( dev, fi->BPB_ResvSecCnt + fi->BPB_FATSz * j, buffer );
	}
	
	// Reset first 64 bits = 8 Bytes of the buffer
	memset( buffer, 0, 8 );
	
	for(i = 1; i < fi->BPB_FATSz; ++i) {
		// Write additional Sectors of the FATs
		for( j = 0; j < fi->BPB_NumFATs; ++j ) {
			diskio_write_block( dev, fi->BPB_ResvSecCnt + fi->BPB_FATSz * j, buffer );
		}
	}
}

void mkfs_write_fsinfo( uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi ) {
	uint32_t fsi_free_count = 0;
	uint32_t fsi_nxt_free = 0;
	// FSI_LeadSig
	buffer[0] = 0x52; buffer[1] = 0x52; buffer[2] = 0x61; buffer[3] = 0x41;
	
	// FSI_Reserved1
	
	// FSI_StrucSig
	buffer[484] = 0x72; buffer[485] = 0x72; buffer[486] = 0x41; buffer[487] = 0x61;
	
	// FSI_Free_Count
	fsi_free_count = (fi->BPB_TotSec - ((fi->BPB_FATSz * fi->BPB_NumFATs) + fi->BPB_RsvdSecCnt)) / fi->BPB_SecPerClus;
	buffer[488] = (uint8_t) fsi_free_count;       buffer[489] = (uint8_t) fsi_free_count >> 8;
	buffer[490] = (uint8_t) fsi_free_count >> 16; buffer[491] = (uint8_t) fsi_free_count >> 24;
	
	// FSI_Nxt_Free
	fsi_nxt_free = (fi->BPB_ResvdSecCnt + (fi->BPB_NumFATs * fi->BPB_FATSz));
	if( fsi_nxt_free % fi->BPB_SecPerClus ) {
		fsi_nxt_free = (fsi_nxt_free / fi->BPB_SecPerClus) + 1;
	} else {
		fsi_nxt_free = (fsi_nxt_free / fi->BPB_SecPerClus);
	}
	buffer[492] = (uint8_t) fsi_nxt_free;       buffer[493] = (uint8_t) fsi_nxt_free >> 8;
	buffer[494] = (uint8_t) fsi_nxt_free >> 16; buffer[495] = (uint8_t) fsi_nxt_free >> 24;	
	
	// FSI_Reserved2
	
	// FSI_TrailSig
	buffer[508] = 0x00; buffer[509] = 0x00; buffer[510] = 0x55; buffer[511] = 0xAA;
	
	diskio_write_block( dev, 1, buffer );
}

void set_fat16_entry( struct FAT_Info *fi, uint16_t cluster, uint16_t value ) {
	uint16_t *fat16_p;
}

void set_fat32_entry( struct FAT_Info *fi, uint32_t cluster, uint32_t value ) {
	uint32_t *fat32_p = get_fat_block( 1, 3 ):
	fat32_p += 2;
	fat32_p = value;
	writeback_fat_block();
}

void mkfs_write_root_directory( uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi ) {
	uint32_t FirstRootDirSecNum = fi->BPB_ResvdSecCnt + (fi->BPB_NumFATs * fi->BPB_FATSz);
	set_fat_entry( fi, 2, EOC );
}

int mkfs_fat( struct diskio_device_info *dev ) {
	uint8_t buffer[512];
	struct FAT_Info fi;
	memset( buffer, 0, 512 );
	mkfs_write_boot_sector( buffer, dev, &fi );	
	memset( buffer, 0, 512 );
	mkfs_write_fats( buffer, dev, &fi );
	memset( buffer, 0, 512 );
	if( fi->type == FAT32 ) {
		mkfs_write_fsinfo( buffer, dev, &fi );
	}
	mkfs_write_root_directory( buffer, dev, &fi );
}

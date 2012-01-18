#include <stdint.h>
#include "cfs/cfs.h"
#include "fat.h"
#include <string.h>

#define FAT_FD_POOL_SIZE 5

uint8_t sector_buffer[512];
uint32_t sector_buffer_addr = 0;
uint8_t sector_buffer_dirty = 0;

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

struct file_system {
	struct diskio_device_info *dev;
	struct FAT_Info info;
	uint32_t first_data_sector;
} mounted;

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

void fat_flush() {
	if( sector_buffer_dirty ) {
		diskio_write_block( mounted.dev, sector_buffer_addr, sector_buffer );
		sector_buffer_dirty = 0;
	}
}

void fat_sync_fats() {
	uint8_t fat_number;
	uint32_t fat_block;
	fat_flush();
	for(fat_block = 0; fat_block < mounted.info.BPB_FATSz; fat_block) {
		diskio_read_block( mounted.dev, fat_block + mounted.info.BPB_RsvdSecCnt, sector_buffer );
		for(fat_number = 2; fat_number <= mounted.info.BPB_NumFATs; fat_number++) {
			diskio_write_block( mounted.dev, (fat_block + mounted.info.BPB_RsvdSecCnt) + ((fat_number - 1) * mounted.info.BPB_FATSz), sector_buffer );
		}
	}
}

uint16_t _get_free_cluster_16();
uint16_t _get_free_cluster_32();
uint8_t is_a_power_of_2( uint32_t value );
void calc_fat_block( uint32_t cur_block, uint32_t *fat_sec_num, uint32_t *ent_offset );
uint32_t get_free_cluster(uint32_t start_cluster);
uint32_t read_fat_entry( uint32_t sec_addr );

void get_fat_info( struct FAT_Info *info ) {
	memcpy( info, &(mounted.info), sizeof(struct FAT_Info) );
}

void print_current_sector() {
	uint16_t i = 0;
	printf("\n");
	for(i = 0; i < 512; i++) {
		printf("%02x", sector_buffer[i]);
		if( ((i+1) % 2) == 0 )
			printf(" ");
		if( ((i+1) % 32) == 0 )
			printf("\n");
	}
}
/**
 * Determines the type of the fat by using the BPB.
 */
uint8_t determine_fat_type( struct FAT_Info *info ) {
	uint16_t RootDirSectors = ((info->BPB_RootEntCnt * 32) + (info->BPB_BytesPerSec - 1)) / info->BPB_BytesPerSec;
	uint32_t DataSec = info->BPB_TotSec - (info->BPB_RsvdSecCnt + (info->BPB_NumFATs * info->BPB_FATSz) + RootDirSectors);
	uint32_t CountofClusters = DataSec / info->BPB_SecPerClus;
	if(CountofClusters < 4085) {
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
 * Parses the Bootsector of a FAT-Filesystem and validates it.
 *
 * \param buffer One sector of the filesystem, must be at least 512 Bytes long.
 * \param info The FAT_Info struct which gets populated with the FAT information.
 * \return <ul>
 *          <li> 0 : Bootsector seems okay.
 *          <li> 1 : BPB_BytesPerSec is not a power of 2
 *          <li> 2 : BPB_SecPerClus is not a power of 2
 *          <li> 4 : Bytes per Cluster is more than 32K
 *          <li> 8 : More than 2 FATs (not supported)
 *          <li> 16: BPB_TotSec is 0
 *          <li> 32: BPB_FATSz is 0
 *          <li> 64: FAT Signature isn't correct
 *         </ul>
 *         More than one error flag may be set but return is 0 on no error.
 */
uint8_t parse_bootsector( uint8_t *buffer, struct FAT_Info *info ) {
	int ret = 0;
	info->BPB_BytesPerSec = (((uint16_t) buffer[12]) << 8) + buffer[11];
	info->BPB_SecPerClus = buffer[13];
	info->BPB_RsvdSecCnt = buffer[14] + (((uint16_t) buffer[15]) << 8);
	info->BPB_NumFATs = buffer[16];
	info->BPB_RootEntCnt = buffer[17] + (((uint16_t) buffer[18]) << 8);
	info->BPB_TotSec = buffer[19] + (((uint16_t) buffer[20]) << 8);
	if( info->BPB_TotSec == 0 )
		info->BPB_TotSec = buffer[32] +
			(((uint32_t) buffer[33]) << 8) +
			(((uint32_t) buffer[34]) << 16) +
			(((uint32_t) buffer[35]) << 24);
	info->BPB_Media = buffer[21];
	info->BPB_FATSz =  buffer[22] + (((uint16_t) buffer[23]) << 8);
	if( info->BPB_FATSz == 0 )
		info->BPB_FATSz = buffer[36] +
			(((uint32_t) buffer[37]) << 8) +
			(((uint32_t) buffer[38]) << 16) +
			(((uint32_t) buffer[39]) << 24);
	info->BPB_RootClus =  buffer[44] +
		(((uint32_t) buffer[45]) << 8) +
		(((uint32_t) buffer[46]) << 16) +
		(((uint32_t) buffer[47]) << 24);
	
	if( is_a_power_of_2( info->BPB_BytesPerSec ) == 0)
		ret += 1;
	if( is_a_power_of_2( info->BPB_SecPerClus ) == 0)
		ret += 2;
	if( info->BPB_BytesPerSec * info->BPB_SecPerClus > 32 * 1024 )
		ret += 4;
	if( info->BPB_NumFATs > 2 )
		ret += 8;
	if( info->BPB_TotSec == 0 )
		ret += 16;
	if( info->BPB_FATSz == 0 )
		ret += 32;
	if( buffer[510] != 0x55 || buffer[511] != 0xaa )
		ret += 64;
	#ifdef DEBUG
	printf("\nparse_bootsector() = %u", ret);
	#endif
	return ret;
}

uint8_t fat_mount_device( struct diskio_device_info *dev ) {
	uint32_t RootDirSectors = 0;
	if (mounted.dev != 0) {
		fat_umount_device();
	}

	//read first sector into buffer
	diskio_read_block( dev, 0, sector_buffer );
	//parse bootsector
	if( parse_bootsector( sector_buffer, &(mounted.info) ) != 0 )
		return 1;
	//call determine_fat_type
	mounted.info.type = determine_fat_type( &(mounted.info) );
	//return 2 if unsupported
	if( mounted.info.type != FAT16 && mounted.info.type != FAT32 )
		return 2;

	mounted.dev = dev;

	RootDirSectors = ((mounted.info.BPB_RootEntCnt * 32) + (mounted.info.BPB_BytesPerSec - 1)) / mounted.info.BPB_BytesPerSec;
	mounted.first_data_sector = mounted.info.BPB_RsvdSecCnt + (mounted.info.BPB_NumFATs * mounted.info.BPB_FATSz) + RootDirSectors;
	printf("fat_mount_device(): first_data_sector = %lu", mounted.first_data_sector );
	return 0;
}

void fat_umount_device() {
	uint8_t i = 0;
	// Write last buffer
	fat_flush();
	// Write second FAT
	fat_sync_fats();
	// invalidate file-descriptors
	for(i = 0; i < FAT_FD_POOL_SIZE; i++) {
		fat_fd_pool[i].file = 0;
	}
	// Reset the device pointer
	mounted.dev = 0;
}

uint8_t fat_read_block( uint32_t sector_addr ) {
	if( sector_buffer_addr == sector_addr && sector_addr != 0 )
		return 0;
	fat_flush();
	sector_buffer_addr = sector_addr;
	return diskio_read_block( mounted.dev, sector_addr, sector_buffer );
}

uint32_t cluster2sector(uint32_t cluster_num) {
	return ((cluster_num - 2) * mounted.info.BPB_SecPerClus) + mounted.first_data_sector;
}

uint8_t is_EOC( uint32_t fat_entry ) {
	if( mounted.info.type == FAT16 ) {
		if( fat_entry >= 0xFFF8 )
			return 1;
	} else if( mounted.info.type == FAT32 ) {
		if( (fat_entry & 0x0FFFFFFF) >= 0x0FFFFFF8 )
			return 1;
	}
	return 0;
}

uint8_t fat_next_block() {
	if( sector_buffer_dirty )
		fat_flush();
	/* Are we on a Cluster edge? */
	if( (sector_buffer_addr + 1) % mounted.info.BPB_SecPerClus == 0 ) {
		uint32_t entry = read_fat_entry( sector_buffer_addr );
		if( is_EOC( entry ) )
			return 128;
		fat_read_block( cluster2sector(entry) );
	} else {
		fat_read_block( sector_buffer_addr + 1 );
	}
}

uint8_t lookup( const char *path, struct dir_entry *dir_entry ) {

}

uint32_t read_fat_entry( uint32_t sec_addr ) {
	uint32_t fat_sec_num = 0, ent_offset = 0, ret = 0;;
	calc_fat_block( sec_addr, &fat_sec_num, &ent_offset );
	fat_read_block( fat_sec_num );
	if( mounted.info.type == FAT16 ) {
		ret = (((uint16_t) sector_buffer[ent_offset]) << 8) + ((uint16_t) sector_buffer[ent_offset]);
	} else if( mounted.info.type == FAT32 ) {
		ret = (((uint32_t) sector_buffer[ent_offset]) << 24) + (((uint32_t) sector_buffer[ent_offset+1]) << 16) + (((uint32_t) sector_buffer[ent_offset+2]) << 8) + ((uint32_t) sector_buffer[ent_offset+3]);
		ret &= 0x0FFFFFFF;
	}
	return ret;
}

void calc_fat_block( uint32_t cur_sec, uint32_t *fat_sec_num, uint32_t *ent_offset ) {
	uint32_t N = cur_sec / mounted.info.BPB_SecPerClus;
	if( mounted.info.type == FAT16 )
		*ent_offset = N * 2;
	else if( mounted.info.type == FAT32 )
		*ent_offset = N * 4;
	*fat_sec_num = mounted.info.BPB_RsvdSecCnt + (*ent_offset / mounted.info.BPB_BytesPerSec);
	*ent_offset = *ent_offset % mounted.info.BPB_BytesPerSec;
}

/**
 * \brief Looks through the FAT to find a free cluster.
 *
 * \TODO Check for end of FAT and no free clusters.
 * \return Returns the number of a free cluster.
 */
uint32_t get_free_cluster(uint32_t start_cluster) {
	uint32_t fat_sec_num = 0;
	uint32_t ent_offset = 0;
	uint16_t i = 0;
	calc_fat_block( start_cluster, &fat_sec_num, &ent_offset );
	do {
		fat_read_block( fat_sec_num );
		if( mounted.info.type == FAT16 )
			i = _get_free_cluster_16();
		else if( mounted.info.type == FAT32 )
			i = _get_free_cluster_32();
		fat_sec_num++;
	} while( i == 512 );
	ent_offset = (fat_sec_num - mounted.info.BPB_RsvdSecCnt) * mounted.info.BPB_BytesPerSec;
	if( mounted.info.type == FAT16 )
		ent_offset /= 2;
	else if( mounted.info.type == FAT32 )
		ent_offset /= 4;
	return ent_offset;
}

uint16_t _get_free_cluster_16() {
	uint16_t entry = 0;
	uint16_t i = 0;
	for( i = 0; i < 512; i += 2 ) {
		entry = (((uint16_t) sector_buffer[i]) << 8) + ((uint16_t) sector_buffer[i+1]);
		if( entry == 0 ) {
			return i;
		}
	}
	return 512;
}

uint16_t _get_free_cluster_32() {
	uint32_t entry = 0;
	uint16_t i = 0;
	for( i = 0; i < 512; i += 4 ) {
		entry = (((uint32_t) sector_buffer[i]) << 24) + (((uint32_t) sector_buffer[i+1]) << 16) + (((uint32_t) sector_buffer[i+2]) << 8) + ((uint32_t) sector_buffer[i+3]);
		if( entry & 0x0FFFFFFF == 0 ) {
			return i;
		}
	}
	return 512;
}

uint8_t get_name_part( const char *path, char *name, uint8_t part_num ) {
	uint8_t start = 0, end = 0, run = 0, i = 0, idx = 0, dot_found = 0;
	memset( name, 0x20, 11 );
	for( i = 0; path[i] != '\0'; ++i ){
		if( path[i] == '/' ) {
			run++;
			end = i;
			if( run > part_num ) {
				for(i = 0, idx = 0; i < end-start; ++i, ++idx) {
					// Part too long
					if( idx >= 11 )
						return 2;
					//ignore . but jump to last 3 chars of name
					if( path[start + i] == '.') {
						if( dot_found )
							return 3;
						idx = 7;
						dot_found = 1;
						continue;
					}
					if( !dot_found && idx > 7 )
						return 4;
					name[idx] = toupper(path[start + i]);
				}
				break;
			} else {
				start = end;
			}
		}
		if( i == 255 )
			return 1;
	}
	return 0;
}

uint32_t find_file_cluster( const char *path ) {
	uint32_t first_root_dir_sec_num = 0;
	struct dir_entry dir_ent;
	char name[11] = "\0";
	uint8_t i = 0;
	if( mounted.info.type == FAT16 ) {
		// calculate the first cluster of the root dir
		first_root_dir_sec_num = mounted.info.BPB_RsvdSecCnt + (mounted.info.BPB_NumFATs * mounted.info.BPB_FATSz);
	} else  if( mounted.info.type == FAT32 ) {
		// BPB_RootClus is the first cluster of the root dir
		first_root_dir_sec_num = mounted.info.BPB_RootClus * mounted.info.BPB_SecPerClus;
	}
	for(i = 0; get_name_part( path, name, i ) == 0; i++) {
		fat_read_block( first_root_dir_sec_num );
		if( !lookup( name, &dir_ent ) ) {
			return 0;
		}
		first_root_dir_sec_num = dir_ent.DIR_FstClusLO + ((uint32_t) dir_ent.DIR_FstClusHI) << 16;
	}
	return first_root_dir_sec_num;
}
/*
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
	if( fd < 0 || fd >= FAT_FD_POOL_SIZE )
		return;
	fat_flush( fd );
	fat_fd_pool[fd].file = NULL;
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
*/
/**
 * Tests if the given value is a power of 2.
 *
 * \param value Number which should be testet if it is a power of 2.
 * \return 0 on failure and 1 if value is a power of 2.
 */
uint8_t is_a_power_of_2( uint32_t value ) {
	uint32_t test = 1;
	uint8_t i = 0;
	if( value == 0 )
		return 0;
	for( i = 0; i < 32; ++i ) {
		test << 1;
		if( test == value )
			return 0;
	}
	return 1;
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
	uint8_t SecPerCluster = 0;
	SecPerCluster = (uint8_t)(bytes / sec_size);
	if( SecPerCluster == 0 )
		return 1;

	if( is_a_power_of_2( SecPerCluster ) != 0 )
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
	uint32_t TmpVal2 = (256 * fi->BPB_BytesPerSec) + fi->BPB_NumFATs;
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
	fi->BPB_FATSz = 0;
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
	if( fi->type == FAT16 ) {
		buffer[14] = 1; buffer[15] = 0;
		fi->BPB_RsvdSecCnt = 1;
	} else if( fi->type == FAT32 ) {
		buffer[14] = 32; buffer[15] = 0;
		fi->BPB_RsvdSecCnt = 32;
	}
	
	// BPB_NumFATs
	buffer[16] = 2;
	fi->BPB_NumFATs = 2;
	
	// BPB_RootEntCnt
	if( fi->type == FAT16 ) {
		buffer[17] = (uint8_t) 512; buffer[18] = (uint8_t) 512 >> 8;
		fi->BPB_RootEntCnt = 512;
	} else if( fi->type == FAT32 ) {
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
	} else if( fi->type == FAT16 ) {
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
		diskio_write_block( dev, fi->BPB_RsvdSecCnt + fi->BPB_FATSz * j, buffer );
	}
	
	// Reset first 64 bits = 8 Bytes of the buffer
	memset( buffer, 0, 8 );
	
	for(i = 1; i < fi->BPB_FATSz; ++i) {
		// Write additional Sectors of the FATs
		for( j = 0; j < fi->BPB_NumFATs; ++j ) {
			diskio_write_block( dev, fi->BPB_RsvdSecCnt + fi->BPB_FATSz * j, buffer );
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
	fsi_nxt_free = (fi->BPB_RsvdSecCnt + (fi->BPB_NumFATs * fi->BPB_FATSz));
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
/*
void set_fat16_entry( struct FAT_Info *fi, uint16_t cluster, uint16_t value ) {
	uint16_t *fat16_p;
}

void set_fat32_entry( struct FAT_Info *fi, uint32_t cluster, uint32_t value ) {
	uint32_t *fat32_p = get_fat_block( 1, 3 ):
	fat32_p += 2;
	fat32_p = value;
	writeback_fat_block();
}
*/
void mkfs_write_root_directory( uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi ) {
	uint32_t FirstRootDirSecNum = fi->BPB_RsvdSecCnt + (fi->BPB_NumFATs * fi->BPB_FATSz);
//	set_fat_entry( fi, 2, EOC );
}

int mkfs_fat( struct diskio_device_info *dev ) {
	uint8_t buffer[512];
	struct FAT_Info fi;
	memset( buffer, 0, 512 );
	mkfs_write_boot_sector( buffer, dev, &fi );	
	memset( buffer, 0, 512 );
	mkfs_write_fats( buffer, dev, &fi );
	memset( buffer, 0, 512 );
	if( fi.type == FAT32 ) {
		mkfs_write_fsinfo( buffer, dev, &fi );
	}
	mkfs_write_root_directory( buffer, dev, &fi );
}

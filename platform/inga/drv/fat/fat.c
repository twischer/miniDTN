#include "fat.h"

#define FAT_FD_POOL_SIZE 5

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

uint8_t sector_buffer[512];
uint32_t sector_buffer_addr = 0;
uint8_t sector_buffer_dirty = 0;

uint16_t cfs_readdir_offset = 0;

struct file_system {
	struct diskio_device_info *dev;
	struct FAT_Info info;
	uint32_t first_data_sector;
} mounted;

struct PathResolver {
	uint16_t start, end;
	const char *path;
	char name[11];
};

struct file fat_file_pool[FAT_FD_POOL_SIZE];
struct file_desc fat_fd_pool[FAT_FD_POOL_SIZE];

/* Declerations */
uint16_t _get_free_cluster_16();
uint16_t _get_free_cluster_32();
uint8_t fat_read_block( uint32_t sector_addr );
uint8_t is_a_power_of_2( uint32_t value );
void calc_fat_block( uint32_t cur_block, uint32_t *fat_sec_num, uint32_t *ent_offset );
uint32_t get_free_cluster(uint32_t start_cluster);
uint32_t read_fat_entry( uint32_t sec_addr );
uint32_t find_file_cluster( const char *path );
uint32_t cluster2sector(uint32_t cluster_num);
void pr_reset( struct PathResolver *rsolv );
uint8_t pr_get_next_path_part( struct PathResolver *rsolv );
uint8_t _make_valid_name( const char *path, uint8_t start, uint8_t end, char *name );
uint8_t pr_is_current_path_part_a_file( struct PathResolver *rsolv );
uint8_t get_dir_entry( const char *path, struct dir_entry *dir_ent, uint32_t *dir_entry_sector, uint16_t *dir_entry_offset );

/**
 * Writes the current buffered block back to the disk if it was changed.
 */
void fat_flush() {
	if( sector_buffer_dirty ) {
		printf("\nfat_flush() : Sector is dirty, writing back to addr = %lu", sector_buffer_addr);
		if( diskio_write_block( mounted.dev, sector_buffer_addr, sector_buffer ) != DISKIO_SUCCESS ) {
			printf("\nfat_flush() : Error writing sector %lu", sector_buffer_addr);
		}
		sector_buffer_dirty = 0;
	}
}

/**
 * Syncs every FAT with the first.
 */
void fat_sync_fats() {
	uint8_t fat_number;
	uint32_t fat_block;
	fat_flush();
	for(fat_block = 0; fat_block < mounted.info.BPB_FATSz; fat_block++) {
		diskio_read_block( mounted.dev, fat_block + mounted.info.BPB_RsvdSecCnt, sector_buffer );
		for(fat_number = 2; fat_number <= mounted.info.BPB_NumFATs; fat_number++) {
			diskio_write_block( mounted.dev, (fat_block + mounted.info.BPB_RsvdSecCnt) + ((fat_number - 1) * mounted.info.BPB_FATSz), sector_buffer );
		}
	}
}

void get_fat_info( struct FAT_Info *info ) {
	memcpy( info, &(mounted.info), sizeof(struct FAT_Info) );
}

void print_current_sector() {
	uint16_t i = 0;
	//printf("\n");
	for(i = 0; i < 512; i++) {
		//printf("%02x", sector_buffer[i]);
		if( ((i+1) % 2) == 0 ) {
			//printf(" ");
		}
		if( ((i+1) % 32) == 0 ) {
			//printf("\n");
		}
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
	
	if( is_a_power_of_2( info->BPB_BytesPerSec ) != 0)
		ret += 1;
	if( is_a_power_of_2( info->BPB_SecPerClus ) != 0)
		ret += 2;
	if( info->BPB_BytesPerSec * info->BPB_SecPerClus > 32 * ((uint32_t) 1024) )
		ret += 4;
	if( info->BPB_NumFATs > 2 )
		ret += 8;
	if( info->BPB_TotSec == 0 )
		ret += 16;
	if( info->BPB_FATSz == 0 )
		ret += 32;
	if( buffer[510] != 0x55 || buffer[511] != 0xaa )
		ret += 64;
	//printf("\nparse_bootsector() = %u", ret);
	return ret;
}

uint8_t fat_mount_device( struct diskio_device_info *dev ) {
	uint32_t RootDirSectors = 0;
	uint32_t dbg_file_cluster = 0;
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

	//sync every FAT to the first on mount
	//fat_sync_fats();

	//Calculated the first_data_sector
	RootDirSectors = ((mounted.info.BPB_RootEntCnt * 32) + (mounted.info.BPB_BytesPerSec - 1)) / mounted.info.BPB_BytesPerSec;
	mounted.first_data_sector = mounted.info.BPB_RsvdSecCnt + (mounted.info.BPB_NumFATs * mounted.info.BPB_FATSz) + RootDirSectors;
	//printf("\nfat_mount_device(): first_data_sector = %lu", mounted.first_data_sector );
	fat_read_block( mounted.first_data_sector );
	print_current_sector();
	//printf("\nfat_mount_device(): Looking for \"Traum.txt\" = %lu", find_file_cluster("traum.txt") );
	//printf("\nfat_mount_device(): Looking for \"prog1.txt\" = %lu", find_file_cluster("prog1.txt") );
	//printf("\nfat_mount_device(): Looking for \"prog1.csv\" = %lu", dbg_file_cluster = find_file_cluster("prog1.csv") );
	//printf("\nfat_mount_device(): Next cluster of \"prog1.csv\" = %lu", dbg_file_cluster = read_fat_entry(dbg_file_cluster * mounted.info.BPB_SecPerClus));
	//printf("\nfat_mount_device(): Next cluster of \"prog1.csv\" = %lu", dbg_file_cluster = read_fat_entry(dbg_file_cluster * mounted.info.BPB_SecPerClus));
	//printf("\nfat_mount_device(): Next cluster of \"prog1.csv\" = %lu", dbg_file_cluster = read_fat_entry(dbg_file_cluster * mounted.info.BPB_SecPerClus));
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
	//printf("\nfat_read_block()");
	if( sector_buffer_addr == sector_addr && sector_addr != 0 )
		return 0;
	fat_flush();
	//printf("READ_BLOCK");
	sector_buffer_addr = sector_addr;
	return diskio_read_block( mounted.dev, sector_addr, sector_buffer );
}

uint32_t cluster2sector(uint32_t cluster_num) {
	if(cluster_num < 2)
		cluster_num = 2;
	return ((cluster_num - 2) * mounted.info.BPB_SecPerClus) + mounted.first_data_sector;
}

/*UNTESTED*/
uint32_t sector2cluster(uint32_t sector_num) {
	return ((sector_num - mounted.first_data_sector) / mounted.info.BPB_SecPerClus) + 2;
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
	//printf("\nfat_next_block()");
	fat_flush();
	/* Are we on a Cluster edge? */
	if( (sector_buffer_addr + 1) % mounted.info.BPB_SecPerClus == 0 ) {
		uint32_t entry = read_fat_entry( sector_buffer_addr );
		//printf("\nfat_next_block(): Cluster edge reached");
		if( is_EOC( entry ) )
			return 128;
		return fat_read_block( cluster2sector(entry) );
	} else {
		//printf("\nfat_next_block(): read sector %lu", sector_buffer_addr + 1);
		return fat_read_block( sector_buffer_addr + 1 );
	}
}

uint8_t lookup( const char *name, struct dir_entry *dir_entry, uint32_t *dir_entry_sector, uint16_t *dir_entry_offset ) {
		uint16_t i = 0;
		for(;;) {
			for( i = 0; i < 512; i+=32 ) {
				//printf("\nlookup(): name = %c%c%c%c%c%c%c%c%c%c%c", name[0], name[1], name[2], name[3], name[4], name[5], name[6], name[7], name[8], name[9], name[10]);
				//printf("\nlookup(): sec_buf = %c%c%c%c%c%c%c%c%c%c%c", sector_buffer[i+0], sector_buffer[i+1], sector_buffer[i+2], sector_buffer[i+3], sector_buffer[i+4], sector_buffer[i+5], sector_buffer[i+6], sector_buffer[i+7], sector_buffer[i+8], sector_buffer[i+9], sector_buffer[i+10]);
				if( memcmp( name, &(sector_buffer[i]), 11 ) == 0 ) {
					memcpy( dir_entry, &(sector_buffer[i]), sizeof(struct dir_entry) );
					*dir_entry_sector = sector_buffer_addr;
					*dir_entry_offset = i;
					return 0;
				}
				// There are no more entries in this directory
				if( sector_buffer[i] == 0x00 ) {
					//printf("\nlookup(): No more directory entries");
					return 1;
				}
			}
			if( fat_next_block() != 0 )
				return 2;
		}
}

uint32_t read_fat_entry_cluster( uint32_t cluster ) {
	return read_fat_entry( cluster2sector( cluster ) );
}

uint32_t read_fat_entry( uint32_t sec_addr ) {
	uint32_t fat_sec_num = 0, ent_offset = 0, ret = 0;
	calc_fat_block( sec_addr, &fat_sec_num, &ent_offset );
	fat_read_block( fat_sec_num );
	if( mounted.info.type == FAT16 ) {
		ret = (((uint16_t) sector_buffer[ent_offset+1]) << 8) + ((uint16_t) sector_buffer[ent_offset]);
	} else if( mounted.info.type == FAT32 ) {
		ret = (((uint32_t) sector_buffer[ent_offset+3]) << 24) + (((uint32_t) sector_buffer[ent_offset+2]) << 16) + (((uint32_t) sector_buffer[ent_offset+1]) << 8) + ((uint32_t) sector_buffer[ent_offset+0]);
		ret &= 0x0FFFFFFF;
	}
	return ret;
}

void write_fat_entry( uint32_t cluster_num, uint32_t value ) {
	uint32_t fat_sec_num = 0, ent_offset = 0;
	//printf("\nwrite_fat_entry( %lu, %lu )", cluster_num, value);
	calc_fat_block( cluster2sector(cluster_num), &fat_sec_num, &ent_offset );
	fat_read_block( fat_sec_num );
	if( mounted.info.type == FAT16 ) {
		sector_buffer[ent_offset+1] = (uint8_t) (value >> 8);
		sector_buffer[ent_offset]   = (uint8_t) (value);
	} else if( mounted.info.type == FAT32 ) {
		sector_buffer[ent_offset+3] = ((uint8_t) (value >> 24) & 0x0FFF) + (0xF000 & sector_buffer[ent_offset+3]);
		sector_buffer[ent_offset+2] = (uint8_t) (value >> 16);
		sector_buffer[ent_offset+1] = (uint8_t) (value >> 8);
		sector_buffer[ent_offset]   = (uint8_t) (value);
	}
	sector_buffer_dirty = 1;
}

void calc_fat_block( uint32_t cur_sec, uint32_t *fat_sec_num, uint32_t *ent_offset ) {
	uint32_t N = sector2cluster( cur_sec );
	//printf("\ncalc_fat_block() : N = %lu", N);
	if( mounted.info.type == FAT16 )
		*ent_offset = N * 2;
	else if( mounted.info.type == FAT32 )
		*ent_offset = N * 4;
	*fat_sec_num = mounted.info.BPB_RsvdSecCnt + (*ent_offset / mounted.info.BPB_BytesPerSec);
	*ent_offset = *ent_offset % mounted.info.BPB_BytesPerSec;
	//printf("\ncalc_fat_block(): fat_sec_num = %lu", *fat_sec_num);
	//printf("\ncalc_fat_block(): ent_offset = %lu", *ent_offset);
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
	printf("\nget_free_cluster( %lu ) -> %lu", start_cluster, cluster2sector(start_cluster));
	calc_fat_block( cluster2sector( start_cluster ), &fat_sec_num, &ent_offset );
	do {
		fat_read_block( fat_sec_num );
		if( mounted.info.type == FAT16 )
			i = _get_free_cluster_16();
		else if( mounted.info.type == FAT32 )
			i = _get_free_cluster_32();
		fat_sec_num++;
	} while( i == 512 );
	ent_offset = ((fat_sec_num - 1) - mounted.info.BPB_RsvdSecCnt) * mounted.info.BPB_BytesPerSec + i;
	if( mounted.info.type == FAT16 )
		ent_offset /= 2;
	else if( mounted.info.type == FAT32 )
		ent_offset /= 4;
	printf("\nget_free_cluster(): next_free_cluster = %lu", ent_offset);
	//printf("\nget_free_cluster(): fat_sec_num = %lu", fat_sec_num);
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
		entry = (((uint32_t) sector_buffer[i+3]) << 24) + (((uint32_t) sector_buffer[i+2]) << 16) + (((uint32_t) sector_buffer[i+1]) << 8) + ((uint32_t) sector_buffer[i]);
		printf("\n_get_free_cluster_32() : entry = %lu", entry & 0x0FFFFFFF);
		if( (entry & 0x0FFFFFFF) == 0 ) {
			return i;
		}
	}
	return 512;
}

uint8_t _make_valid_name( const char *path, uint8_t start, uint8_t end, char *name ) {
	uint8_t i = 0, idx = 0, dot_found = 0;
	memset( name, 0x20, 11 );
	//printf("\n_make_valid_name() : name = ");
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
		//printf("%c", name[idx]);
	}
	return 0;
}

void print_dir_entry( struct dir_entry *dir_entry ) {
	//printf("\nDirectory Entry");
	//printf("\n\tDIR_Name = %c%c%c%c%c%c%c%c%c%c%c", dir_entry->DIR_Name[0], dir_entry->DIR_Name[1],dir_entry->DIR_Name[2],dir_entry->DIR_Name[3],dir_entry->DIR_Name[4],dir_entry->DIR_Name[5],dir_entry->DIR_Name[6],dir_entry->DIR_Name[7],dir_entry->DIR_Name[8],dir_entry->DIR_Name[9],dir_entry->DIR_Name[10]);
	//printf("\n\tDIR_Attr = %x", dir_entry->DIR_Attr);
	//printf("\n\tDIR_NTRes = %x", dir_entry->DIR_NTRes);
	//printf("\n\tCrtTimeTenth = %x", dir_entry->CrtTimeTenth);
	//printf("\n\tDIR_CrtTime = %x", dir_entry->DIR_CrtTime);
	//printf("\n\tDIR_CrtDate = %x", dir_entry->DIR_CrtDate);
	//printf("\n\tDIR_LstAccessDate = %x", dir_entry->DIR_LstAccessDate);
	//printf("\n\tDIR_FstClusHI = %x", dir_entry->DIR_FstClusHI);
	//printf("\n\tDIR_WrtTime = %x", dir_entry->DIR_WrtTime);
	//printf("\n\tDIR_WrtDate = %x", dir_entry->DIR_WrtDate);
	//printf("\n\tDIR_FstClusLO = %x", dir_entry->DIR_FstClusLO);
	//printf("\n\tDIR_FileSize = %lu Bytes", dir_entry->DIR_FileSize);
}

uint8_t get_dir_entry( const char *path, struct dir_entry *dir_ent, uint32_t *dir_entry_sector, uint16_t *dir_entry_offset ) {
	uint32_t first_root_dir_sec_num = 0;
	uint32_t file_sector_num = 0;
	uint8_t i = 0;
	struct PathResolver pr;
	pr_reset( &pr );
	pr.path = path;
	if( mounted.info.type == FAT16 ) {
		// calculate the first cluster of the root dir
		first_root_dir_sec_num = mounted.info.BPB_RsvdSecCnt + (mounted.info.BPB_NumFATs * mounted.info.BPB_FATSz); // TODO Verify this is correct
	} else if( mounted.info.type == FAT32 ) {
		// BPB_RootClus is the first cluster of the root dir
		first_root_dir_sec_num = cluster2sector( mounted.info.BPB_RootClus );
	}
	file_sector_num = first_root_dir_sec_num;
	for(i = 0; pr_get_next_path_part( &pr ) == 0 && i < 255; i++) {
		fat_read_block( file_sector_num );
		if( lookup( pr.name, dir_ent, dir_entry_sector, dir_entry_offset ) != 0 ) {
			return 0;
		}
		file_sector_num = cluster2sector( dir_ent->DIR_FstClusLO + (((uint32_t) dir_ent->DIR_FstClusHI) << 16) );
		print_dir_entry( dir_ent );
	}
	if( file_sector_num == first_root_dir_sec_num )
		return 0;
	return 1;
}

/**
 * Only user for easier Debuging output. Should NOT be used any longer.
 */
uint32_t find_file_cluster( const char *path ) {
	struct dir_entry dir_ent;
	uint32_t dir_entry_sector;
	uint16_t dir_entry_offset;
	get_dir_entry( path, &dir_ent, &dir_entry_sector, &dir_entry_offset );
	return dir_ent.DIR_FstClusLO + (((uint32_t) dir_ent.DIR_FstClusHI) << 16);
}

uint32_t find_nth_cluster( uint32_t start_cluster, uint32_t n ) {
	uint32_t cluster = start_cluster, i = 0;
	printf("\nfind_nth_cluster( %lu, %lu )", start_cluster, n);
	for( i = 0; i < n; i++ ) {
		cluster = read_fat_entry_cluster( cluster );
		printf("\n\t next cluster = %lu", cluster);
	}
	return cluster;
}

uint8_t fat_read_file( int fd, uint32_t clusters, uint32_t clus_offset ) {
	/*Read the clus_offset Sector of the clusters-th cluster of the file fd*/
	uint32_t cluster = 0;
	if( clusters == fat_file_pool[fd].n ) {
		cluster = fat_file_pool[fd].nth_cluster;
	} else if( clusters == fat_file_pool[fd].n + 1) {
		cluster = read_fat_entry_cluster(fat_file_pool[fd].nth_cluster);
		fat_file_pool[fd].nth_cluster = cluster;
		fat_file_pool[fd].n++;
	} else {
		cluster = find_nth_cluster( fat_file_pool[fd].cluster, clusters );
		fat_file_pool[fd].nth_cluster = cluster;
		fat_file_pool[fd].n = clusters;
	}
	//printf("\nfat_read_file(): cluster = %lu", cluster);
	//printf("\nfat_read_file(): sector = %lu", cluster2sector(cluster) + clus_offset);
	if( cluster == 0 || is_EOC( cluster ) )
		return 1;
	return fat_read_block( cluster2sector(cluster) + clus_offset );
}

uint8_t fat_write_file( int fd, uint32_t clusters, uint32_t clus_offset ) {
	uint32_t cluster = 0;
	//If we know the nth Cluster already we do not have to recalculate it
	if( clusters == fat_file_pool[fd].n ) {
		cluster = fat_file_pool[fd].nth_cluster;
	//If we now the nth-1 Cluster it is easy to get the next cluster
	} else if( clusters == fat_file_pool[fd].n + 1) {
		cluster = read_fat_entry_cluster(fat_file_pool[fd].nth_cluster);
		fat_file_pool[fd].nth_cluster = cluster;
		fat_file_pool[fd].n++;
	//Somehow our cluster-information is out of sync. We have to calculate the cluster the hard way.
	} else {
		cluster = find_nth_cluster( fat_file_pool[fd].cluster, clusters );
		fat_file_pool[fd].nth_cluster = cluster;
		fat_file_pool[fd].n = clusters;
	}
	//printf("\nfat_write_file() : cluster = %lu", cluster);
	if( cluster == 0 ) {
		return 1;
	//If this the last Cluster of the file, chain an additional cluster.
	} else if( is_EOC( cluster ) ) {
		uint32_t last_cluster_num = find_nth_cluster( fat_file_pool[fd].cluster, clusters-1 );
		uint32_t free_cluster = get_free_cluster( last_cluster_num );
		//printf("\nfat_write_file() : cluster = EOC");
		write_fat_entry( last_cluster_num, free_cluster );
		write_fat_entry( free_cluster, EOC );
		return fat_read_block( cluster2sector( free_cluster ) );
	}
	return fat_read_block( cluster2sector(cluster) + clus_offset );
}

void pr_reset( struct PathResolver *rsolv ) {
	rsolv->start = 0;
	rsolv->end = 0;
	rsolv->path = NULL;
	memset(rsolv->name, '\0', 11);
}

uint8_t pr_get_next_path_part( struct PathResolver *rsolv ) {
	uint16_t i = 0;
	if( rsolv->path == NULL )
		return 2;
	rsolv->start = rsolv->end;
	rsolv->end++;
	if( rsolv->path[rsolv->start] == '/' )
		rsolv->start++;

	for( i = rsolv->start; rsolv->path[i] != '\0'; i++ ){
		if( rsolv->path[i] != '/' ) {
			rsolv->end++;
		}
		if( rsolv->path[rsolv->end] == '/' || rsolv->path[rsolv->end] == '\0' ) {
			return _make_valid_name( rsolv->path, rsolv->start, rsolv->end, rsolv->name );
		}
	}
	return 1;
}

uint8_t pr_is_current_path_part_a_file( struct PathResolver *rsolv ) {
	if( rsolv->path[rsolv->end] == '/')
		return 0;
	else if(rsolv->path[rsolv->end] == '\0' )
		return 1;
	return 0;
}

uint32_t add_cluster_to_current_chain() {
	uint32_t current_sector = sector_buffer_addr;
	uint32_t free_cluster = get_free_cluster( sector2cluster( current_sector ) );
	//printf("\nadd_cluster_to_current_chain()");
	write_fat_entry( sector2cluster( current_sector ), free_cluster );
	write_fat_entry( free_cluster, EOC );
	return free_cluster;
}

uint8_t add_directory_entry_to_current( struct dir_entry *dir_ent, uint32_t *dir_entry_sector, uint16_t *dir_entry_offset ) {
	uint16_t i = 0;
	uint8_t ret = 0;
	//printf("\nadd_directory_entry_to_current(): BEGIN");
	for(;;) {
		for( i = 0; i < 512; i+=32 ) {
			if( sector_buffer[i] == 0x00 || sector_buffer[i] == 0xE5 ) {
				memcpy( &(sector_buffer[i]), dir_ent, sizeof(struct dir_entry) );
				sector_buffer_dirty = 1;
				*dir_entry_sector = sector_buffer_addr;
				*dir_entry_offset = i;
				return 0;
			}
		}
		if( (ret = fat_next_block()) != 0 ) {
			if( ret == 128 ) {
				uint32_t cluster = add_cluster_to_current_chain();
				if( fat_read_block( cluster2sector( cluster ) ) == 0 ){
					memcpy( &(sector_buffer[0]), dir_ent, sizeof(struct dir_entry) );
					sector_buffer_dirty = 1;
					*dir_entry_sector = sector_buffer_addr;
					*dir_entry_offset = 0;
					return 0;
				}
			}
			return 1;
		}
	}
	return 1;
}

uint8_t fat_create_file( const char *path, struct dir_entry *dir_ent, uint32_t *dir_entry_sector, uint16_t *dir_entry_offset ) {
	uint32_t first_root_dir_sec_num = 0;
	uint32_t file_sector_num = 0;
	uint8_t i = 0;
	struct PathResolver pr;
	//printf("\nfat_create_file(): BEGIN");
	pr_reset( &pr );
	pr.path = path;
	if( mounted.info.type == FAT16 ) {
		// calculate the first cluster of the root dir
		first_root_dir_sec_num = mounted.info.BPB_RsvdSecCnt + (mounted.info.BPB_NumFATs * mounted.info.BPB_FATSz); // TODO Verify this is correct
	} else if( mounted.info.type == FAT32 ) {
		// BPB_RootClus is the first cluster of the root dir
		first_root_dir_sec_num = cluster2sector( mounted.info.BPB_RootClus );
	}
	file_sector_num = first_root_dir_sec_num;
	for(i = 0; pr_get_next_path_part( &pr ) == 0 && i < 255; i++) {
		fat_read_block( file_sector_num );
		if( lookup( pr.name, dir_ent, dir_entry_sector, dir_entry_offset ) != 0 ) {
			//printf("\nfat_create_file(): X");
			if( pr_is_current_path_part_a_file( &pr ) ) {
				//printf("X");
				memset( dir_ent, 0, sizeof(struct dir_entry) );
				memcpy( dir_ent->DIR_Name, pr.name, 11 );
				dir_ent->DIR_Attr = 0;
				print_dir_entry( dir_ent );
				return add_directory_entry_to_current( dir_ent, dir_entry_sector, dir_entry_offset );
			}
			return 0;
		}
		file_sector_num = cluster2sector( dir_ent->DIR_FstClusLO + (((uint32_t) dir_ent->DIR_FstClusHI) << 16) );
	}
	return 0;
}

uint8_t _is_file( struct dir_entry *dir_ent ) {
	if( dir_ent->DIR_Attr & ATTR_DIRECTORY )
		return 0;
	if( dir_ent->DIR_Attr & ATTR_VOLUME_ID )
		return 0;
	return 1;
}

uint8_t _cfs_flags_ok( int flags, struct dir_entry *dir_ent ) {
	if( (flags & CFS_APPEND || flags & CFS_WRITE) && dir_ent->DIR_Attr & ATTR_READ_ONLY )
		return 0;
	return 1;
}

int
cfs_open(const char *name, int flags)
{
	// get FileDescriptor
	int fd = -1;
	uint8_t i = 0;
	struct dir_entry dir_ent;
	for( i = 0; i < FAT_FD_POOL_SIZE; i++ ) {
		if( fat_fd_pool[i].file == 0 ) {
			fd = i;
			break;
		}
	}
	/*No free FileDesciptors available*/
	if( fd == -1 )
		return fd;
	// find file on Disk
	if( !get_dir_entry( name, &dir_ent, &fat_file_pool[fd].dir_entry_sector, &fat_file_pool[fd].dir_entry_offset ) ) {
		if( flags & CFS_WRITE || flags & CFS_APPEND ) {
			if( fat_create_file( name, &dir_ent, &fat_file_pool[fd].dir_entry_sector, &fat_file_pool[fd].dir_entry_offset ) != 0 ) {
				return -1;
			}
		} else {
			return -1;
		}
	}
	if( !_is_file( &dir_ent ) ) {
		return -1;
	}
	if( !_cfs_flags_ok( flags, &dir_ent ) ) {
		return -1;
	}
	fat_file_pool[fd].cluster = dir_ent.DIR_FstClusLO + (((uint32_t) dir_ent.DIR_FstClusHI) << 16);
	fat_file_pool[fd].nth_cluster = fat_file_pool[fd].cluster;
	fat_file_pool[fd].n = 0;
	fat_fd_pool[fd].file = &(fat_file_pool[fd]);
	fat_fd_pool[fd].flags = (uint8_t) flags;
	// put read/write position in the right spot
	fat_fd_pool[fd].offset = 0;
	memcpy( &(fat_file_pool[fd].dir_entry), &dir_ent, sizeof(struct dir_entry) );
	if( flags & CFS_APPEND )
		cfs_seek( fd, CFS_SEEK_END, 0 );
	// return FileDescriptor
	return fd;
}

void update_dir_entry( int fd ) {
	//printf("\nupdate_dir_entry() : dir_entry_sector = %lu", fat_file_pool[fd].dir_entry_sector);
	if( fat_read_block( fat_file_pool[fd].dir_entry_sector ) != 0 )
		return;
	memcpy( &(sector_buffer[fat_file_pool[fd].dir_entry_offset]), &(fat_file_pool[fd].dir_entry), sizeof(struct dir_entry) );
	sector_buffer_dirty = 1;
}

void
cfs_close(int fd)
{
	if( fd < 0 || fd >= FAT_FD_POOL_SIZE )
		return;
	update_dir_entry( fd );
	fat_flush( fd );
	fat_fd_pool[fd].file = NULL;
}

int
cfs_read(int fd, void *buf, unsigned int len)
{
	uint32_t offset = fat_fd_pool[fd].offset % mounted.info.BPB_BytesPerSec;
	uint32_t clusters = (fat_fd_pool[fd].offset / mounted.info.BPB_BytesPerSec) / mounted.info.BPB_SecPerClus;
	uint8_t clus_offset = (fat_fd_pool[fd].offset / mounted.info.BPB_BytesPerSec) % mounted.info.BPB_SecPerClus;
	uint16_t i, j = 0;
	uint8_t *buffer = (uint8_t *) buf;
	if( fd < 0 || fd >= FAT_FD_POOL_SIZE )
		return -1;
	/*Special case of empty file, cluster is zero, no data*/
	if( fat_file_pool[fd].cluster == 0 )
		return 0;
	if( !(fat_fd_pool[fd].flags & CFS_READ) )
		return -1;
	while( fat_read_file( fd, clusters, clus_offset ) == 0 ) {
		for( i = offset; i < 512 && j < len; i++,j++,fat_fd_pool[fd].offset++ ) {
			buffer[j] = sector_buffer[i];
		}
		if( (clus_offset + 1) % mounted.info.BPB_SecPerClus == 0 ) {
			clus_offset = 0;
			clusters++;
		} else {
			clus_offset++;
		}
		if( j >= len )
			break;
	}
	return (int) j;
}

void _add_cluster_to_empty_file( int fd, uint32_t free_cluster ) {
	//printf("\n_add_cluster_to_empty_file( %d, %lu )", fd, free_cluster);
	write_fat_entry( free_cluster, EOC );
	fat_file_pool[fd].dir_entry.DIR_FstClusHI = (uint16_t) (free_cluster >> 16);
	fat_file_pool[fd].dir_entry.DIR_FstClusLO = (uint16_t) (free_cluster);
	update_dir_entry( fd );
	fat_file_pool[fd].cluster = free_cluster;
}

void add_cluster_to_file( int fd ) {
	uint32_t free = get_free_cluster( 0 );
	uint32_t cluster = fat_file_pool[fd].cluster;
	uint32_t n = cluster;
	//printf("\nadd_cluster_to_file()");
	if( fat_file_pool[fd].cluster == 0 ) {
		_add_cluster_to_empty_file( fd, free );
		return;
	}
	while( !is_EOC( n ) ) {
		//printf("\nadd_cluster_to_file() : n = %lu", n );
		cluster = n;
		n = read_fat_entry_cluster( cluster );
	}
	printf("\nadd_cluster_to_file() : cluster = %lu, free = %lu, n = %lx", cluster, free, n );
	write_fat_entry( cluster, free );
	write_fat_entry( free, EOC );
}

int
cfs_write(int fd, const void *buf, unsigned int len)
{
	uint32_t offset = fat_fd_pool[fd].offset % mounted.info.BPB_BytesPerSec;
	uint32_t clusters = (fat_fd_pool[fd].offset / mounted.info.BPB_BytesPerSec) / mounted.info.BPB_SecPerClus;
	uint8_t clus_offset = (fat_fd_pool[fd].offset / mounted.info.BPB_BytesPerSec) % mounted.info.BPB_SecPerClus;
	uint16_t i, j = 0;
	uint8_t *buffer = (uint8_t *) buf;
	if( len > 0 && fat_file_pool[fd].dir_entry.DIR_FileSize == 0 )
		add_cluster_to_file( fd );
	while( fat_write_file( fd, clusters, clus_offset ) == 0 ) {
		for( i = offset; i < 512 && j < len; i++,j++,fat_fd_pool[fd].offset++ ) {
			sector_buffer[i] = buffer[j];
			if( fat_fd_pool[fd].offset == fat_file_pool[fd].dir_entry.DIR_FileSize )
				fat_file_pool[fd].dir_entry.DIR_FileSize++;
		}
		sector_buffer_dirty = 1;
		if( (clus_offset + 1) % mounted.info.BPB_SecPerClus == 0 ) {
			clus_offset = 0;
			clusters++;
		} else {
			clus_offset++;
		}
		if( j >= len )
			break;
	}
	return j;
}

cfs_offset_t
cfs_seek(int fd, cfs_offset_t offset, int whence)
{
	if( fd < 0 || fd >= FAT_FD_POOL_SIZE )
		return -1;
	switch(whence) {
		case CFS_SEEK_SET:
			fat_fd_pool[fd].offset = offset;
			break;
		case CFS_SEEK_CUR:
			fat_fd_pool[fd].offset += offset;
			break;
		case CFS_SEEK_END:
			fat_fd_pool[fd].offset = (fat_file_pool[fd].dir_entry.DIR_FileSize - 1) + offset;
			break;
		default:
			break;
	}
	if( fat_fd_pool[fd].offset >= fat_file_pool[fd].dir_entry.DIR_FileSize )
		fat_fd_pool[fd].offset = (fat_file_pool[fd].dir_entry.DIR_FileSize - 1);
	if( fat_fd_pool[fd].offset < 0 )
		fat_fd_pool[fd].offset = 0;
	return fat_fd_pool[fd].offset;
}

void reset_cluster_chain( struct dir_entry *dir_ent ) {
	uint32_t cluster = (((uint32_t) dir_ent->DIR_FstClusHI) << 16) + dir_ent->DIR_FstClusLO;
	uint32_t next_cluster = read_fat_entry( cluster );
	while( !is_EOC( cluster ) && cluster >= 2 ) {
		write_fat_entry( cluster, 0L );
		cluster = next_cluster;
		next_cluster = read_fat_entry( cluster );
	}
	write_fat_entry( cluster, 0L );
}

void remove_dir_entry( uint32_t dir_entry_sector, uint16_t dir_entry_offset ) {
	if( fat_read_block( dir_entry_sector ) != 0 )
		return;
	memset( &(sector_buffer[dir_entry_offset]), 0, sizeof(struct dir_entry) );
	sector_buffer[dir_entry_offset] = 0xE5;
	sector_buffer_dirty = 1;
}

int
cfs_remove(const char *name)
{
	struct dir_entry dir_ent;
	uint32_t sector;
	uint16_t offset;
	if( !get_dir_entry( name, &dir_ent, &sector, &offset ) ) {
		//printf("\ncfs_remove(): File not found!");
		return -1;
	}
	if( _is_file( &dir_ent ) ) {
		//printf("\ncfs_remove(): File found!");
		reset_cluster_chain( &dir_ent );
		remove_dir_entry( sector, offset );
		fat_flush();
		return 0;
	}
	//printf("\ncfs_remove(): Urks!");
	return -1;
}

int
cfs_opendir(struct cfs_dir *dirp, const char *name)
{
	struct dir_entry dir_ent;
	uint32_t sector;
	uint16_t offset;
	uint32_t dir_cluster = get_dir_entry( name, &dir_ent, &sector, &offset );
	cfs_readdir_offset = 0;
	if( dir_cluster == 0 )
		return -1;
	memcpy( dirp, &dir_ent, sizeof(struct dir_entry) );
	return 0;
}

uint8_t get_dir_entry_old( struct dir_entry *dir, uint16_t offset, struct dir_entry *entry ) {
	uint32_t dir_off = offset * 32;
	uint16_t cluster_num = dir_off / mounted.info.BPB_SecPerClus;
	uint32_t cluster;
	cluster = find_nth_cluster( (((uint32_t) dir->DIR_FstClusHI) << 16) + dir->DIR_FstClusLO, (uint32_t) cluster_num );
	if( cluster == 0 )
		return 1;
	if( fat_read_block( cluster2sector(cluster) + dir_off / mounted.info.BPB_BytesPerSec ) != 0 )
		return 2;
	memcpy( entry, &(sector_buffer[dir_off % mounted.info.BPB_BytesPerSec]), sizeof(struct dir_entry) );
	return 0;
}

void make_readable_entry( struct dir_entry *dir, struct cfs_dirent *dirent ) {
	uint8_t i, j;
	for( i = 0, j = 0; i < 11; i++ ) {
		if( dir->DIR_Name[i] != ' ' ) {
			dirent->name[j] = dir->DIR_Name[i];
			j++;
		}
		if( i == 7 ) {
			dirent->name[j] = '.';
			j++;
		}
	}
}

int
cfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent)
{
	struct dir_entry *dir_ent = (struct dir_entry *) dirp;
	struct dir_entry entry;
	if( get_dir_entry_old( dir_ent, cfs_readdir_offset, &entry ) != 0 )
		return -1;
	make_readable_entry( &entry, dirent );
	dirent->size = entry.DIR_FileSize;
	cfs_readdir_offset++;
	return 0;
}

void
cfs_closedir(struct cfs_dir *dirp)
{
	cfs_readdir_offset = 0;
}

/**
 * Tests if the given value is a power of 2.
 *
 * \param value Number which should be testet if it is a power of 2.
 * \return 1 on failure and 0 if value is a power of 2.
 */
uint8_t is_a_power_of_2( uint32_t value ) {
	uint32_t test = 1;
	uint8_t i = 0;
	if( value == 0 )
		return 0;
	for( i = 0; i < 32; ++i ) {
		if( test == value )
			return 0;
		test = test << 1;
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
	uint32_t po2 = ((uint32_t) 1) << 31;
	while( value < po2 ) {
		po2 = po2 >> 1;
	}
	return po2;
}

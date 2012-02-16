#ifndef FAT_COOP_H
#define FAT_COOP_H

#define FAT_COOPERATIVE 1
#define FAT_COOP_BUFFER_SIZE 128
#define FAT_COOP_SLOT_SIZE_MS 50
#include "fat.h"

enum { WRITE,
	READ,
	OPEN,
	CLOSE
};

int8_t cfs_open( const char *name, int flags, uint8_t *token );
int8_t cfs_close( int fd, uint8_t *token); 
int8_t cfs_write( int fd, uint8_t *buf, uint16_t length, uint8_t *token );
int8_t cfs_read( int fd, uint8_t *buf, uint16_t length, uint8_t *token );
int8_t cfs_seek( int fd, cfs_offset_t offset, int whence, uint8_t *token);
int8_t cfs_remove( const char *name, uint8_t *token );
int8_t cfs_opendir( struct cfs_dir *dirp, const char *name, uint8_t *token );
int8_t cfs_readdir( struct cfs_dir *dirp, struct cfs_dirent *dirent, uint8_t *token);
int8_t cfs_closedir(struct cfs_dir *dirp, uint8_t *token);
uint8_t fat_op_status( uint8_t token );
uint16_t fat_estimate_by_token( uint8_t token );
uint16_t fat_estimate_by_parameter( uint8_t type, uint16_t length );
uint8_t fat_buffer_available( uint16_t length );
uint8_t fat_reserve_buffer( uint16_t length );

#endif

#ifndef FAT_COOP_H
#define FAT_COOP_H

#define FAT_COOPERATIVE 1
#define FAT_COOP_BUFFER_SIZE 128
#define FAT_COOP_QUEUE_SIZE 15
#define FAT_COOP_SLOT_SIZE_MS 50

#define FAT_COOP_TIME_READ_BLOCK_MS 8
#define FAT_COOP_TIME_WRITE_BLOCK_MS 12

enum {
	QUEUED = 1,
	INPROGRESS,
	DONE
};

#include "fat.h"

typedef enum {
	CFS_OPEN,
	CFS_CLOSE,
	CFS_WRITE,
	CFS_READ,
	CFS_SEEK,
	CFS_REMOVE,
	CFS_OPENDIR,
	CFS_READDIR,
	CFS_CLOSEDIR
} Operation;

int8_t ccfs_open( const char *name, int flags, uint8_t *token );
int8_t ccfs_close( int fd, uint8_t *token);
int8_t ccfs_write( int fd, uint8_t *buf, uint16_t length, uint8_t *token );
int8_t ccfs_read( int fd, uint8_t *buf, uint16_t length, uint8_t *token );
int8_t ccfs_seek( int fd, cfs_offset_t offset, int whence, uint8_t *token);
int8_t ccfs_remove( const char *name, uint8_t *token );
int8_t ccfs_opendir( struct cfs_dir *dirp, const char *name, uint8_t *token );
int8_t ccfs_readdir( struct cfs_dir *dirp, struct cfs_dirent *dirent, uint8_t *token);
int8_t ccfs_closedir(struct cfs_dir *dirp, uint8_t *token);
uint8_t fat_op_status( uint8_t token );
uint16_t fat_estimate_by_token( uint8_t token );
uint16_t fat_estimate_by_parameter( Operation type, uint16_t length );
uint8_t fat_buffer_available( uint16_t length );

#endif
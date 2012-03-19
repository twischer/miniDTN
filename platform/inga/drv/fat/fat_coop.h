/* Copyright (c) 2012, Christoph Peltz
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
 
 /**
 * \addtogroup Drivers
 * @{
 *
 * \defgroup fat_coop_driver FAT Driver - Cooperative Additions
 *
 * <p></p>
 * @{
 *
 */

/**
 * \file
 *		FAT driver Coop Additions definitions
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */
 
#ifndef FAT_COOP_H
#define FAT_COOP_H

#define FAT_COOPERATIVE 1
#define FAT_COOP_BUFFER_SIZE 128
#define FAT_COOP_QUEUE_SIZE 15
#define FAT_COOP_SLOT_SIZE_MS 50L

#define FAT_COOP_TIME_READ_BLOCK_MS 8
#define FAT_COOP_TIME_WRITE_BLOCK_MS 12

#include <stdint.h>

#include "fat.h"

enum {
	STATUS_QUEUED = 1,
	STATUS_INPROGRESS,
	STATUS_DONE
};

typedef struct q_entry_generic_file_op {
	int fd;
	uint8_t *buffer;
	uint16_t length;
} GenericFileOp;

typedef struct q_entry_open_file_op {
	const char *name;
	int flags;
} OpenFileOp;

typedef struct q_entry_seek_file_op {
	int fd;
	cfs_offset_t offset;
	int whence;
} SeekFileOp;

typedef struct q_entry_dir_op {
	struct cfs_dir *dirp;
	union {
		struct cfs_dirent *dirent;
		const char *name;
	} u;
} DirOp;

typedef enum {
	COOP_CFS_OPEN,
	COOP_CFS_CLOSE,
	COOP_CFS_WRITE,
	COOP_CFS_READ,
	COOP_CFS_SEEK,
	COOP_CFS_REMOVE,
	COOP_CFS_OPENDIR,
	COOP_CFS_READDIR,
	COOP_CFS_CLOSEDIR
} Operation;

typedef struct q_entry {
	uint8_t token;
	Operation op;
	struct process *source_process;
	union {
		GenericFileOp generic;
		OpenFileOp open;
		SeekFileOp seek;
		DirOp dir;
	} parameters;
	uint8_t state;
	int16_t ret_value;
} QueueEntry;

typedef struct event_op_finished {
	uint8_t token;
	int16_t ret_value;
} Event_OperationFinished;

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
void printQueueEntry( QueueEntry *entry );
process_event_t get_coop_event_id();

#endif

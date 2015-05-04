/*
 * Copyright (c) 2012, Institute of Operating Systems and Computer Networks (TU Brunswick).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */
 
 /**
 * \addtogroup cfs
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

#include <stdint.h>
#include "cfs-fat.h"

#define FAT_COOP_BUFFER_SIZE 128
#define FAT_COOP_QUEUE_SIZE 15
#define FAT_COOP_SLOT_SIZE_MS 50L

#define FAT_COOP_TIME_READ_BLOCK_MS 8
#define FAT_COOP_TIME_WRITE_BLOCK_MS 12

#define FAT_COOP_STACK_SIZE 192


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
uint8_t get_item_from_buffer( uint8_t *start, uint16_t index);

void operation(void *data);

PROCESS_NAME(fsp);
#endif

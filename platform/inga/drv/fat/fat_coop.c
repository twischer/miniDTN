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
 * \addtogroup Drivers
 * @{
 *
 * \addtogroup fat_coop_driver
 * @{
 */

/**
 * \file
 *      FAT driver coop additions implementation
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */
 
#include "fat_coop.h"

enum {
	READ = 1,
	WRITE,
	INTERNAL
};

static process_event_t coop_global_event_id = 0;
static uint8_t next_token = 0;

process_event_t get_coop_event_id() {
	if( coop_global_event_id == 0 ) {
		// Request global ID from contiki
		coop_global_event_id = process_alloc_event();
	}
	return coop_global_event_id;
}

void printQueueEntry( QueueEntry *entry ) {
	printf("\nQueueEntry %u", (uint16_t) entry);
	printf("\n\ttoken = %u", entry->token);
	printf("\n\tsource_process = %u", (uint16_t) entry->source_process);
	printf("\n\top = ");
	switch(entry->op) {
		case COOP_CFS_OPEN:
			printf("COOP_CFS_OPEN");
			printf("\n\tname = %s", entry->parameters.open.name);
			printf("\n\tflags = %d", entry->parameters.open.flags);
			break;
		case COOP_CFS_CLOSE:
			printf("COOP_CFS_CLOSE");
			printf("\n\tfd = %d", entry->parameters.generic.fd);
			printf("\n\tbuffer = %u", (uint16_t)entry->parameters.generic.buffer);
			printf("\n\tlength = %u", entry->parameters.generic.length);
			break;
		case COOP_CFS_WRITE:
			printf("COOP_CFS_WRITE");
			printf("\n\tfd = %d", entry->parameters.generic.fd);
			printf("\n\tbuffer = %u", (uint16_t)entry->parameters.generic.buffer);
			printf("\n\tlength = %u", entry->parameters.generic.length);
			break;
		case COOP_CFS_READ:
			printf(" COOP_CFS_READ");
			printf("\n\tfd = %d", entry->parameters.generic.fd);
			printf("\n\tbuffer = %u", (uint16_t)entry->parameters.generic.buffer);
			printf("\n\tlength = %u", entry->parameters.generic.length);
			break;
		case COOP_CFS_SEEK:
			printf(" COOP_CFS_SEEK");
			printf("\n\tfd = %d", entry->parameters.seek.fd);
			printf("\n\toffset = %d", entry->parameters.seek.offset);
			printf("\n\twhence = %d", entry->parameters.seek.whence);
			break;
		case COOP_CFS_REMOVE:
			printf("OOP_CFS_REMOVE");
			printf("\n\tname = %s", entry->parameters.open.name);
			break;
		case COOP_CFS_OPENDIR:
			printf("OP_CFS_OPENDIR");
			break;
		case COOP_CFS_READDIR:
			printf("OP_CFS_READDIR");
			break;
		case COOP_CFS_CLOSEDIR:
			printf("OP_CFS_CLOSEDIR");
			break;
	}
	printf("\n\tstate = %u", entry->state);
	printf("\n\tret_value = %d", entry->ret_value);
}

uint8_t writeBuffer[FAT_COOP_BUFFER_SIZE];
uint16_t writeBuffer_start, writeBuffer_len;

QueueEntry queue[FAT_COOP_QUEUE_SIZE];
Event_OperationFinished op_results[FAT_COOP_QUEUE_SIZE];
uint16_t queue_start, queue_len;

uint8_t coop_step_allowed = 0;

#define MS_TO_TICKS(ms) ((uint32_t)(ms * (RTIMER_SECOND / 1000.0f)))

#define FAT_COOP_SLOT_SIZE_TICKS MS_TO_TICKS(FAT_COOP_SLOT_SIZE_MS)
#define FAT_COOP_TIME_READ_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_READ_BLOCK_MS)
#define FAT_COOP_TIME_WRITE_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_WRITE_BLOCK_MS)

#define FAT_COOP_STACK_SIZE 192

static uint8_t stack[FAT_COOP_STACK_SIZE];
static uint8_t *sp = 0;
static uint8_t *sp_save = 0;
uint8_t next_step_type = INTERNAL;


/** From fat.c */
extern struct file_system mounted;
/** From fat.c */
extern struct file fat_file_pool[FAT_FD_POOL_SIZE];
/** From fat.c */
extern struct file_desc fat_fd_pool[FAT_FD_POOL_SIZE];

void coop_mt_init( void *data );
void coop_switch_sp();
QueueEntry* queue_current_entry();
void perform_next_step( QueueEntry *entry );
void finish_operation( QueueEntry *entry );
uint8_t time_left_for_step( QueueEntry *entry, uint32_t start_time );
uint8_t try_internal_operation();
uint8_t queue_rm_top_entry();
uint8_t queue_add_entry( QueueEntry *entry );

void operation(void *data) {
	QueueEntry *entry = (QueueEntry *) data;
	
	coop_step_allowed = 1;

	if( entry->state != STATUS_INPROGRESS ) {
		entry->state = STATUS_INPROGRESS;
	}

	switch(entry->op){
		case COOP_CFS_OPEN:
			{
				int fd = entry->ret_value;
				entry->ret_value = cfs_open( entry->parameters.open.name, entry->parameters.open.flags );
				if( entry->ret_value == -1 ) {
					fat_fd_pool[fd].file = NULL;
				}
			}			
			break;
		case COOP_CFS_CLOSE:
			cfs_close( entry->parameters.generic.fd );
			break;
		case COOP_CFS_WRITE:
			entry->ret_value = cfs_write( entry->parameters.generic.fd, entry->parameters.generic.buffer,
				entry->parameters.generic.length );
			break;
		case COOP_CFS_READ:
			entry->ret_value = cfs_read( entry->parameters.generic.fd, entry->parameters.generic.buffer,
				entry->parameters.generic.length );
			break;
		case COOP_CFS_SEEK:
			entry->ret_value = cfs_seek( entry->parameters.seek.fd, entry->parameters.seek.offset,
				entry->parameters.seek.whence );
			break;
		case COOP_CFS_REMOVE:
			entry->ret_value = cfs_remove( entry->parameters.open.name );
			break;
		case COOP_CFS_OPENDIR:
			entry->ret_value = cfs_opendir( entry->parameters.dir.dirp, entry->parameters.dir.u.name );
			break;
		case COOP_CFS_READDIR:
			entry->ret_value = cfs_readdir( entry->parameters.dir.dirp, entry->parameters.dir.u.dirent );
			break;
		case COOP_CFS_CLOSEDIR:
			cfs_closedir( entry->parameters.dir.dirp );
			break;
	}
	entry->state = STATUS_DONE;
	op_results[queue_start].token = entry->token;
	op_results[queue_start].ret_value = entry->ret_value;
}

/**
 * This Function jumps back to perform_next_step()
 */
void coop_finished_op() {
	/* Change SP back to original */
	coop_switch_sp();
}

/**
 * This function is mostly copied from the arm/mtarch.c file.
 */
void coop_mt_init( void *data ) {
	memset(stack, 0, FAT_COOP_STACK_SIZE);
  /* coop_init function that is to be invoked if the thread dies */
  stack[FAT_COOP_STACK_SIZE -  1] = (unsigned char)((unsigned short)coop_finished_op) & 0xff;
  stack[FAT_COOP_STACK_SIZE -  2] = (unsigned char)((unsigned short)coop_finished_op >> 8) & 0xff;

  /* The thread handler. Invoked when RET is called in coop_switch_sp */
  stack[FAT_COOP_STACK_SIZE -  3] = (unsigned char)((unsigned short)operation) & 0xff;
  stack[FAT_COOP_STACK_SIZE -  4] = (unsigned char)((unsigned short)operation >> 8) & 0xff;

  /* Register r0-r23 in t->stack[MTARCH_STACKSIZE -  5] to
   *  t->stack[MTARCH_STACKSIZE -  28].
   *
   * Per calling convention, the argument to the thread handler function
   *  (i.e. the 'data' pointer) is passed via r24-r25.
   * See http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_reg_usage) */
  stack[FAT_COOP_STACK_SIZE - 29] = (unsigned char)((unsigned short)data) & 0xff;
  stack[FAT_COOP_STACK_SIZE - 30] = (unsigned char)((unsigned short)data >> 8) & 0xff;

  /* Initialize stack pointer: Space for 2 2-byte-addresses and 32 registers,
   * post-decrement POP / pre-increment PUSH scheme */
  sp = &stack[FAT_COOP_STACK_SIZE - 1 - 4 - 32];
}

/**
 * Function switches the stack pointers for Atmel based devices (32 GPR's).
 */
void coop_switch_sp() {
/* Disable interrupts while we perform the context switch */
  cli ();

  /* Push 32 general purpuse registers */
  __asm__("push r0");
  __asm__("push r1");
  __asm__("push r2");
  __asm__("push r3");
  __asm__("push r4");
  __asm__("push r5");
  __asm__("push r6");
  __asm__("push r7");
  __asm__("push r8");
  __asm__("push r9");
  __asm__("push r10");
  __asm__("push r11");
  __asm__("push r12");
  __asm__("push r13");
  __asm__("push r14");
  __asm__("push r15");
  __asm__("push r16");
  __asm__("push r17");
  __asm__("push r18");
  __asm__("push r19");
  __asm__("push r20");
  __asm__("push r21");
  __asm__("push r22");
  __asm__("push r23");
  __asm__("push r24");
  __asm__("push r25");
  __asm__("push r26");
  __asm__("push r27");
  __asm__("push r28");
  __asm__("push r29");
  __asm__("push r30");
  __asm__("push r31");

  /* Switch stack pointer */
  if( sp_save == NULL ) {
	sp_save = (uint8_t *) SP;
	SP = (uint16_t) sp;
  } else {
	sp = (uint8_t *) SP;
	SP = (uint16_t) sp_save;
	sp_save = NULL;
  }

  /* Pop 32 general purpose registers */
  __asm__("pop r31");
  __asm__("pop r30");
  __asm__("pop r29");
  __asm__("pop r28");
  __asm__("pop r27");
  __asm__("pop r26");
  __asm__("pop r25");
  __asm__("pop r24");
  __asm__("pop r23");
  __asm__("pop r22");
  __asm__("pop r21");
  __asm__("pop r20");
  __asm__("pop r19");
  __asm__("pop r18");
  __asm__("pop r17");
  __asm__("pop r16");
  __asm__("pop r15");
  __asm__("pop r14");
  __asm__("pop r13");
  __asm__("pop r12");
  __asm__("pop r11");
  __asm__("pop r10");
  __asm__("pop r9");
  __asm__("pop r8");
  __asm__("pop r7");
  __asm__("pop r6");
  __asm__("pop r5");
  __asm__("pop r4");
  __asm__("pop r3");
  __asm__("pop r2");
  __asm__("pop r1");
  __asm__("pop r0");

  /* Reenable interrupts */
  sei ();
}

int calc_free_stack() {
	int i;
	for( i = 0; i < FAT_COOP_STACK_SIZE; i++ ) {
		if(stack[i] != 0)
			break;
	}
	return i;
}

PROCESS(fsp, "FileSystemProcess");
PROCESS_THREAD(fsp, ev, data)
{
	PROCESS_BEGIN();
	while(1) {
		PROCESS_WAIT_EVENT();
		static uint32_t start_time = 0, p_time = 0;
		static QueueEntry *entry;
		entry = queue_current_entry();
		start_time = RTIMER_NOW();
	
		while( entry != NULL && time_left_for_step( entry, start_time ) ) {
			watchdog_periodic();
			perform_next_step( entry );
			if( entry->state == STATUS_DONE ) {
				finish_operation( entry );
				entry = queue_current_entry();
			}
		}
		p_time = RTIMER_NOW();
		if( queue_len == 0 ) {
			if( !try_internal_operation() ) {
				break;
			}
		} else {
			process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL );
		}
	}
	//printf("\n free stack = %u", calc_free_stack());
	PROCESS_END();
}

uint8_t try_internal_operation() {
	// Write dirty cache back
	// fat_flush()
	// Write max. n changed sectors of the FAT (To be implemented in FAT-Driver)
	// fat_sync_fats(x);
	return 0;
}

/**
 * Changes Stack pointer to internal Thread. Returns after one step has been completed.
 *
 * If this function was called with one specific QueueEntry, then this function must be
 * called again and again with the same QueueEntry, until the QueueEntry is finished
 * executing.
 * \param *entry The QueueEntry, which should be processed.
 */
void perform_next_step( QueueEntry *entry ) {
	if( sp == NULL ) {
		coop_mt_init( (void *) entry );
	}
	coop_switch_sp();
}

/**
 * Sends a Message to the source_process of the Entry and removes the Entry from the queue.
 *
 * \param *entry The entry which should be finished.
 */
void finish_operation( QueueEntry *entry ) {
	// Send Event to source process
	process_post( entry->source_process, get_coop_event_id(), &(op_results[queue_start]));
	// Remove entry
	queue_rm_top_entry();
	/* Init the internal Stack for the next Operation */
	entry = queue_current_entry();
	if( entry != NULL ) {
		coop_mt_init( (void *) entry );
	} else {
		sp = NULL;
	}
}

uint8_t time_left_for_step( QueueEntry *entry, uint32_t start_time ) {
	uint8_t step_type = 0;
	uint32_t time_left = RTIMER_NOW();
	step_type = next_step_type;
	time_left = RTIMER_NOW() - start_time;
	if( FAT_COOP_SLOT_SIZE_TICKS < time_left) {
		return 0;
	}

	if( step_type == INTERNAL ) {
		return 1;
	}

	time_left = FAT_COOP_SLOT_SIZE_TICKS - time_left;
	if( step_type == READ ) {
		if( time_left > FAT_COOP_TIME_READ_BLOCK_TICKS ) {
			return 1;
		}
		return 0;
	}

	if( step_type == WRITE ) {
		if( time_left > FAT_COOP_TIME_WRITE_BLOCK_TICKS ) {
			return 1;
		}
		return 0;
	}
	return 0;
}

QueueEntry* queue_current_entry() {
	if( queue_len == 0 ) {
		return NULL;
	}
	return &(queue[queue_start]);
}

uint8_t queue_add_entry( QueueEntry *entry ) {
	uint16_t pos = (queue_start + queue_len) % FAT_COOP_QUEUE_SIZE;
	if( pos == queue_start && queue_len > 0 ) {
		return 1;
	}
	memcpy( &(queue[pos]), entry, sizeof(QueueEntry) );
	queue_len++;
	return 0;
}

uint8_t queue_rm_top_entry() {
	if( queue_len == 0 ) {
		return 1;
	}
	queue_start = (queue_start + 1) % FAT_COOP_QUEUE_SIZE;
	queue_len--;
	return 0;
}

uint8_t push_on_buffer( uint8_t *source, uint16_t length ) {
	uint16_t pos = (writeBuffer_start + writeBuffer_len) % FAT_COOP_BUFFER_SIZE;
	uint16_t free = 0;
	if( pos == writeBuffer_start ) {
		return 1;
	}

	if( pos < writeBuffer_start ) {
		free = writeBuffer_start - pos;
	} else {
		free = (FAT_COOP_BUFFER_SIZE - pos) + writeBuffer_start;
	}

	if( length > free ) {
		return 2;
	}

	if( pos + length > FAT_COOP_BUFFER_SIZE ) {
		memcpy( &(writeBuffer[pos]), source, FAT_COOP_BUFFER_SIZE - pos );
		memcpy( &(writeBuffer[0]), source, length - (FAT_COOP_BUFFER_SIZE - pos) );
	} else {
		memcpy( &(writeBuffer[pos]), source, length );
	}
	writeBuffer_len += length;
	return 0;
}

void pop_from_buffer( uint16_t length ) {
	if( length > writeBuffer_len ) {
		return ;
	}
	writeBuffer_start = (writeBuffer_start + length) % FAT_COOP_BUFFER_SIZE;
	writeBuffer_len -= length;
}

int8_t ccfs_open( const char *name, int flags, uint8_t *token ) {
	int fd = -1;
	uint8_t i = 0;
	QueueEntry entry;
	for( i = 0; i < FAT_FD_POOL_SIZE; i++ ) {
		if( fat_fd_pool[i].file == 0 ) {
			fd = i;
			break;
		}
	}
	if( fd == -1 ) {
		return 2;
	}

	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_OPEN;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.open.name = name;
	entry.parameters.open.flags = flags;
	entry.ret_value = fd;
	entry.state = STATUS_QUEUED;

	fat_fd_pool[fd].file = &(fat_file_pool[fd]);

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_close( int fd, uint8_t *token ) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_CLOSE;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.generic.fd = fd;
	entry.state = STATUS_QUEUED;
	return 0;
}

int8_t ccfs_read( int fd, uint8_t *buf, uint16_t length, uint8_t *token ) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_READ;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.generic.fd = fd;
	entry.parameters.generic.buffer = buf;
	entry.parameters.generic.length = length;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_write( int fd, uint8_t *buf, uint16_t length, uint8_t *token ) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_WRITE;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.generic.fd = fd;

	if( !fat_buffer_available( length ) ) {
		return 2;
	}
	entry.parameters.generic.buffer = &(writeBuffer[(writeBuffer_start + writeBuffer_len) % FAT_COOP_BUFFER_SIZE]);
	push_on_buffer( buf, length );
	entry.parameters.generic.length = length;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_seek( int fd, cfs_offset_t offset, int whence, uint8_t *token) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_SEEK;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.seek.fd = fd;
	entry.parameters.seek.offset = offset;
	entry.parameters.seek.whence = whence;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_remove( const char *name, uint8_t *token ) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_REMOVE;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.open.name = name;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_opendir( struct cfs_dir *dirp, const char *name, uint8_t *token ) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_OPENDIR;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.dir.dirp = dirp;
	entry.parameters.dir.u.name = name;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_readdir( struct cfs_dir *dirp, struct cfs_dirent *dirent, uint8_t *token) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_READDIR;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.dir.dirp = dirp;
	entry.parameters.dir.u.dirent = dirent;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

int8_t ccfs_closedir(struct cfs_dir *dirp, uint8_t *token) {
	QueueEntry entry;
	if( queue_len == FAT_COOP_QUEUE_SIZE ) {
		return 1;
	}
	
	process_start( &fsp, NULL );	
	if( process_post( &fsp, PROCESS_EVENT_CONTINUE, NULL ) == PROCESS_ERR_FULL ) {
		return 3;
	}

	entry.token = next_token++;
	if( token != NULL ) {
		*token = entry.token;
	}
	entry.op = COOP_CFS_CLOSEDIR;
	entry.source_process = PROCESS_CURRENT();
	entry.parameters.dir.dirp = dirp;
	entry.state = STATUS_QUEUED;

	queue_add_entry( &entry );

	return 0;
}

uint8_t fat_op_status( uint8_t token ) {
	uint16_t i = 0;
	QueueEntry *entry;
	for( i = 0; i < queue_len; i++ ) {
		entry = &(queue[(queue_start + i) % FAT_COOP_QUEUE_SIZE]);
		if( entry->token == token ) {
			return entry->state;
		}
	}
	return 0;
}

uint16_t _get_time_of_operation( Operation op, uint16_t length ) {
	uint8_t op_multiplikator = (length / 512) + ((length % 512 != 0) ? 1 : 0);
	switch(op){
		case COOP_CFS_OPENDIR:
		case COOP_CFS_READDIR:
		case COOP_CFS_READ:
		case COOP_CFS_OPEN:
			return (FAT_COOP_TIME_READ_BLOCK_MS) * op_multiplikator;
		case COOP_CFS_CLOSEDIR:
		case COOP_CFS_SEEK:
		case COOP_CFS_CLOSE:
			return 0;
		case COOP_CFS_REMOVE:
		case COOP_CFS_WRITE:
			return (FAT_COOP_TIME_READ_BLOCK_MS + FAT_COOP_TIME_WRITE_BLOCK_MS)  * op_multiplikator;
	}
	return 0;
}

uint16_t fat_estimate_by_token( uint8_t token ) { 
	uint16_t estimated_time = 0;
	uint16_t i;
	QueueEntry *entry;
	for( i = 0; i < queue_len; i++ ) {
		entry = &(queue[(queue_start + i) % FAT_COOP_QUEUE_SIZE]);
		estimated_time += _get_time_of_operation( entry->op );
		if(entry->token == token) {
			break;
		}
	}
	return estimated_time;
}

uint16_t fat_estimate_by_parameter( Operation type, uint16_t length ) { 
	uint16_t estimated_time = _get_time_of_operation( type );
	uint16_t i;
	QueueEntry *entry;
	for( i = 0; i < queue_len; i++ ) {
		entry = &(queue[(queue_start + i) % FAT_COOP_QUEUE_SIZE]);
		estimated_time += _get_time_of_operation( entry->op );
	}
	return estimated_time;
}

uint8_t fat_buffer_available( uint16_t length ) {
	uint16_t pos = (writeBuffer_start + writeBuffer_len) % FAT_COOP_BUFFER_SIZE;
	uint16_t free = 0;
	if( pos == writeBuffer_start && writeBuffer_len != 0) {
		return 0;
	}

	if( pos < writeBuffer_start ) {
		free = writeBuffer_start - pos;
	} else {
		free = (FAT_COOP_BUFFER_SIZE - pos) + writeBuffer_start;
	}

	if( length > free ) {
		return 0;
	}
	return 1;
}

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
 * \addtogroup fat_coop_driver
 * @{
 */

/**
 * \file
 *      FAT driver coop additions implementation
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */

#include "cfs-fat.h"
#include "fat_coop.h"
#include "watchdog.h"
#include "fat-coop-arch.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define MS_TO_TICKS(ms) ((uint32_t)(ms * (RTIMER_SECOND / 1000.0f)))

#define FAT_COOP_SLOT_SIZE_TICKS MS_TO_TICKS(FAT_COOP_SLOT_SIZE_MS)
#define FAT_COOP_TIME_READ_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_READ_BLOCK_MS)
#define FAT_COOP_TIME_WRITE_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_WRITE_BLOCK_MS)

enum {
  READ = 1,
  WRITE,
  INTERNAL
};

static process_event_t coop_global_event_id = 0;
static uint8_t next_token = 0;

uint8_t writeBuffer[FAT_COOP_BUFFER_SIZE];
uint16_t writeBuffer_start, writeBuffer_len;

QueueEntry queue[FAT_COOP_QUEUE_SIZE];
Event_OperationFinished op_results[FAT_COOP_QUEUE_SIZE];
uint16_t queue_start, queue_len;

uint8_t coop_step_allowed = 0;

uint8_t next_step_type = INTERNAL;

/** From fat.c */
extern struct file_system mounted;

/** From fat.c */
extern struct file fat_file_pool[FAT_FD_POOL_SIZE];

/** From fat.c */
extern struct file_desc fat_fd_pool[FAT_FD_POOL_SIZE];

void coop_mt_init(void *data);
void coop_switch_sp();
QueueEntry* queue_current_entry();
void perform_next_step(QueueEntry *entry);
void finish_operation(QueueEntry *entry);
uint8_t time_left_for_step(uint8_t step_type);
uint8_t try_internal_operation();
uint8_t queue_rm_top_entry();
uint8_t queue_add_entry(QueueEntry *entry);
void pop_from_buffer(uint16_t length);
uint32_t time_left();
uint8_t time_for_step(uint8_t step_type);

PROCESS(fsp, "FileSystemProcess");
/*----------------------------------------------------------------------------*/
process_event_t
get_coop_event_id()
{
  if (coop_global_event_id == 0) {
    // Request global ID from contiki
    coop_global_event_id = process_alloc_event();
  }

  return coop_global_event_id;
}
/*----------------------------------------------------------------------------*/
void
printQueueEntry(QueueEntry *entry)
{
  printf("\nQueueEntry %u", (uint16_t) entry);
  printf("\n\ttoken = %u", entry->token);
  printf("\n\tsource_process = %u", (uint16_t) entry->source_process);
  printf("\n\top = ");

  switch (entry->op) {
    case COOP_CFS_OPEN:
      printf("COOP_CFS_OPEN");
      printf("\n\tname = %s", entry->parameters.open.name);
      printf("\n\tflags = %d", entry->parameters.open.flags);
      break;
    case COOP_CFS_CLOSE:
      printf("COOP_CFS_CLOSE");
      printf("\n\tfd = %d", entry->parameters.generic.fd);
      printf("\n\tbuffer = %u", (uint16_t) entry->parameters.generic.buffer);
      printf("\n\tlength = %u", entry->parameters.generic.length);
      break;
    case COOP_CFS_WRITE:
      printf("COOP_CFS_WRITE");
      printf("\n\tfd = %d", entry->parameters.generic.fd);
      printf("\n\tbuffer = %u", (uint16_t) entry->parameters.generic.buffer);
      printf("\n\tlength = %u", entry->parameters.generic.length);
      break;
    case COOP_CFS_READ:
      printf(" COOP_CFS_READ");
      printf("\n\tfd = %d", entry->parameters.generic.fd);
      printf("\n\tbuffer = %u", (uint16_t) entry->parameters.generic.buffer);
      printf("\n\tlength = %u", entry->parameters.generic.length);
      break;
    case COOP_CFS_SEEK:
      printf(" COOP_CFS_SEEK");
      printf("\n\tfd = %d", entry->parameters.seek.fd);
      printf("\n\toffset = %lu", (uint32_t) entry->parameters.seek.offset);
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
/*----------------------------------------------------------------------------*/
void
operation(void *data)
{
  QueueEntry *entry = (QueueEntry *) data;

  coop_step_allowed = 1;

  if (entry->state != STATUS_INPROGRESS) {
    entry->state = STATUS_INPROGRESS;
  }

  switch (entry->op) {
    case COOP_CFS_OPEN:
      PRINTF("FSP: opening file %s\n", entry->parameters.open.name);
      int fd = entry->ret_value;
      entry->ret_value = cfs_open(entry->parameters.open.name, entry->parameters.open.flags);
      if (entry->ret_value == -1) {
        fat_fd_pool[fd].file = NULL;
      }
      break;
    case COOP_CFS_CLOSE:
      PRINTF("FSP: closing file\n");
      cfs_close(entry->parameters.generic.fd);
      break;
    case COOP_CFS_WRITE:
      PRINTF("FSP: write to file\n");
      /** FIXME: Ring Buffer functionality does not work, because cfs_write does not know, that the buffer may wrap around */
      entry->ret_value = cfs_write(entry->parameters.generic.fd, entry->parameters.generic.buffer,
              entry->parameters.generic.length);
      pop_from_buffer(entry->parameters.generic.length);
      break;
    case COOP_CFS_READ:
      PRINTF("FSP: read from file\n");
      entry->ret_value = cfs_read(entry->parameters.generic.fd, entry->parameters.generic.buffer,
              entry->parameters.generic.length);
      break;
    case COOP_CFS_SEEK:
      PRINTF("FSP: seek in file\n");
      entry->ret_value = cfs_seek(entry->parameters.seek.fd, entry->parameters.seek.offset,
              entry->parameters.seek.whence);
      break;
    case COOP_CFS_REMOVE:
      PRINTF("FSP: removing file %s\n", entry->parameters.open.name);
      entry->ret_value = cfs_remove(entry->parameters.open.name);
      break;
    case COOP_CFS_OPENDIR:
      entry->ret_value = cfs_opendir(entry->parameters.dir.dirp, entry->parameters.dir.u.name);
      break;
    case COOP_CFS_READDIR:
      entry->ret_value = cfs_readdir(entry->parameters.dir.dirp, entry->parameters.dir.u.dirent);
      break;
    case COOP_CFS_CLOSEDIR:
      cfs_closedir(entry->parameters.dir.dirp);
      break;
  }

  entry->state = STATUS_DONE;
  op_results[queue_start].token = entry->token;
  op_results[queue_start].ret_value = entry->ret_value;
}
/*----------------------------------------------------------------------------*/
PROCESS_THREAD(fsp, ev, data)
{
	static uint32_t deadline = 0;
	static QueueEntry *entry;
	static uint16_t operations = 0;

	PROCESS_BEGIN();

	while (1) {
		PROCESS_WAIT_EVENT();

		// How long is time for processing?
		deadline = RTIMER_NOW() + time_left();

		entry = queue_current_entry();

		while (entry != NULL && deadline > RTIMER_NOW()) {
			uint32_t my_end = RTIMER_NOW() + time_for_step(next_step_type);

			watchdog_periodic();
			perform_next_step(entry);

			if (entry->state == STATUS_DONE) {
				PRINTF("FSP: Operation finished\n");
				finish_operation(entry);
				entry = queue_current_entry();
				operations ++;
			}
		}
	}

	PROCESS_END();
}
/*----------------------------------------------------------------------------*/
uint8_t
try_internal_operation()
{
  // Write dirty cache back
  // fat_flush()
  // Write max. n changed sectors of the FAT (To be implemented in FAT-Driver)
  // fat_sync_fats(x);
  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * Sends a Message to the source_process of the Entry and removes the Entry from the queue.
 *
 * \param *entry The entry which should be finished.
 */
void
finish_operation(QueueEntry *entry)
{
  // Send Event to source process
  process_post(entry->source_process, get_coop_event_id(), &(op_results[queue_start]));

  // Remove entry
  queue_rm_top_entry();

  /* Init the internal Stack for the next Operation */
  entry = queue_current_entry();

  if (entry != NULL) {
    coop_mt_init((void *) entry);
  } else {
    coop_mt_stop();
  }
}
/*----------------------------------------------------------------------------*/
uint32_t time_left()
{
	  uint32_t time_left = 0;

	  // We have time for maximum one step
	  time_left = FAT_COOP_SLOT_SIZE_TICKS;

	  // If we do have a timer, maximum time may also be shorter
	  if( etimer_next_expiration_time() != 0 ) {
		  uint32_t time_left_etimer = (((uint32_t) (etimer_next_expiration_time() - clock_time())) * ((uint32_t) RTIMER_ARCH_SECOND)) / CLOCK_SECOND;

		  // Safety margin
		  time_left_etimer -= 5;

		  if( time_left_etimer < time_left ) {
			  time_left = time_left_etimer;
		  }
	  }

	  return time_left;
}
/*----------------------------------------------------------------------------*/
uint8_t
time_for_step(uint8_t step_type)
{
	uint8_t result = 0;

	if (step_type == INTERNAL ) {
		result = 0;
	}

	if (step_type == READ ) {
		result = FAT_COOP_TIME_READ_BLOCK_TICKS;
	}

	if (step_type == WRITE ) {
		result = FAT_COOP_TIME_WRITE_BLOCK_TICKS;
	}

	// Add a 5 tick safety margin
	result += 5;

	return result;
}
/*----------------------------------------------------------------------------*/
uint8_t
time_left_for_step(uint8_t step_type)
{
	if( time_left() > time_for_step(step_type) ) {
		return 1;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
QueueEntry*
queue_current_entry()
{
  if (queue_len == 0) {
    return NULL;
  }

  return &(queue[queue_start]);
}
uint8_t
queue_add_entry(QueueEntry *entry)
{
  uint16_t pos = (queue_start + queue_len) % FAT_COOP_QUEUE_SIZE;

  if (pos == queue_start && queue_len > 0) {
    return 1;
  }

  memcpy(&(queue[pos]), entry, sizeof (QueueEntry));
  queue_len++;

  return 0;
}
/*----------------------------------------------------------------------------*/
uint8_t
queue_rm_top_entry()
{
  if (queue_len == 0) {
    return 1;
  }

  queue_start = (queue_start + 1) % FAT_COOP_QUEUE_SIZE;
  queue_len--;

  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * This function is used to buffer the payload of write requests in a ring buffer
 * However, the ring functionality does not work
 */
uint8_t
push_on_buffer(uint8_t *source, uint16_t length)
{
  uint16_t pos = (writeBuffer_start + writeBuffer_len) % FAT_COOP_BUFFER_SIZE;
  uint16_t free = 0;

  // Test if writeBuffer is full
  if (pos == writeBuffer_start - 1) {
    return 1;
  }

  // Calculate free space in Buffer
  if (pos < writeBuffer_start) {
    free = writeBuffer_start - pos;
  } else {
    free = (FAT_COOP_BUFFER_SIZE - pos) + writeBuffer_start;
  }

  if (length > free) {
    return 2;
  }

  if (pos + length > FAT_COOP_BUFFER_SIZE) {
    memcpy(&(writeBuffer[pos]), source, FAT_COOP_BUFFER_SIZE - pos);
    memcpy(&(writeBuffer[0]), source, length - (FAT_COOP_BUFFER_SIZE - pos));
  } else {
    memcpy(&(writeBuffer[pos]), source, length);
  }

  writeBuffer_len += length;

  return 0;
}
/*----------------------------------------------------------------------------*/
void
pop_from_buffer(uint16_t length)
{
  if (length > writeBuffer_len) {
    return;
  }

  writeBuffer_start = (writeBuffer_start + length) % FAT_COOP_BUFFER_SIZE;
  writeBuffer_len -= length;

  if (writeBuffer_len == 0) {
    // If the buffer is empty, we can set the start pointer to the beginning
    writeBuffer_start = 0;
  }
}
/*----------------------------------------------------------------------------*/
uint8_t
get_item_from_buffer(uint8_t *start, uint16_t index)
{
  // Calculate the start_index from the start-Buffer Offset from the writeBuffer
  uint16_t start_index = (uint16_t) (start - writeBuffer);
  return writeBuffer[(start_index + index) % FAT_COOP_BUFFER_SIZE];
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_open(const char *name, int flags, uint8_t *token)
{
  int fd = -1;
  uint8_t i = 0;
  QueueEntry entry;

  for (i = 0; i < FAT_FD_POOL_SIZE; i++) {
    if (fat_fd_pool[i].file == 0) {
      fd = i;
      break;
    }
  }

  if (fd == -1) {
    return 2;
  }

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_OPEN;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.open.name = name;
  entry.parameters.open.flags = flags;
  entry.ret_value = fd;
  entry.state = STATUS_QUEUED;

  fat_fd_pool[fd].file = &(fat_file_pool[fd]);

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_close(int fd, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_CLOSE;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.generic.fd = fd;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_read(int fd, uint8_t *buf, uint16_t length, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_READ;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.generic.fd = fd;
  entry.parameters.generic.buffer = buf;
  entry.parameters.generic.length = length;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_write(int fd, uint8_t *buf, uint16_t length, uint8_t *token)
{
  QueueEntry entry;

  memset(&entry, 0, sizeof(QueueEntry));

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    PRINTF("ccfs_write: Buffer full\n");
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    PRINTF("ccfs_write: Event Queue full\n");
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_WRITE;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.generic.fd = fd;

  if (!fat_buffer_available(length)) {
    PRINTF("ccfs_write: Buffer full\n");
    return 2;
  }

  entry.parameters.generic.buffer = &(writeBuffer[(writeBuffer_start + writeBuffer_len) % FAT_COOP_BUFFER_SIZE]);

  /** FIXME: We push information in a ring buffer, which actually does not work as a ring, because the eventual cfs_write call does not know anything about rings */
  if (push_on_buffer(buf, length) != 0) {
    return 4;
  }
  entry.parameters.generic.length = length;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_seek(int fd, cfs_offset_t offset, int whence, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_SEEK;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.seek.fd = fd;
  entry.parameters.seek.offset = offset;
  entry.parameters.seek.whence = whence;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_remove(const char *name, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_REMOVE;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.open.name = name;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_opendir(struct cfs_dir *dirp, const char *name, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_OPENDIR;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.dir.dirp = dirp;
  entry.parameters.dir.u.name = name;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_readdir(struct cfs_dir *dirp, struct cfs_dirent *dirent, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_READDIR;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.dir.dirp = dirp;
  entry.parameters.dir.u.dirent = dirent;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
ccfs_closedir(struct cfs_dir *dirp, uint8_t *token)
{
  QueueEntry entry;

  if (queue_len == FAT_COOP_QUEUE_SIZE) {
    return 1;
  }

  process_start(&fsp, NULL);
  if (process_post(&fsp, PROCESS_EVENT_CONTINUE, NULL) == PROCESS_ERR_FULL) {
    return 3;
  }

  entry.token = next_token++;
  if (token != NULL) {
    *token = entry.token;
  }

  entry.op = COOP_CFS_CLOSEDIR;
  entry.source_process = PROCESS_CURRENT();
  entry.parameters.dir.dirp = dirp;
  entry.state = STATUS_QUEUED;

  queue_add_entry(&entry);

  return 0;
}
/*----------------------------------------------------------------------------*/
uint8_t
fat_op_status(uint8_t token)
{
  uint16_t i = 0;
  QueueEntry *entry;

  for (i = 0; i < queue_len; i++) {
    entry = &(queue[(queue_start + i) % FAT_COOP_QUEUE_SIZE]);
    if (entry->token == token) {
      return entry->state;
    }
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
uint16_t
_get_time_of_operation(Operation op, uint16_t length)
{
  uint8_t op_multiplikator = (length / 512) + ((length % 512 != 0) ? 1 : 0);

  switch (op) {
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
      return (FAT_COOP_TIME_READ_BLOCK_MS + FAT_COOP_TIME_WRITE_BLOCK_MS) * op_multiplikator;
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
uint16_t
fat_estimate_by_token(uint8_t token)
{
  uint16_t estimated_time = 0;
  uint16_t i;
  int length;

  QueueEntry *entry;
  for (i = 0; i < queue_len; i++) {
    entry = &(queue[(queue_start + i) % FAT_COOP_QUEUE_SIZE]);

    length = 0;
    if (entry->op == COOP_CFS_CLOSE || entry->op == COOP_CFS_READ || entry->op == COOP_CFS_WRITE) {
      length = entry->parameters.generic.length;
    }

    estimated_time += _get_time_of_operation(entry->op, length);
    if (entry->token == token) {
      break;
    }
  }

  return estimated_time;
}
/*----------------------------------------------------------------------------*/
uint16_t
fat_estimate_by_parameter(Operation type, uint16_t length)
{
  uint16_t estimated_time = _get_time_of_operation(type, length);
  uint16_t i;
  int sublength = 0;

  QueueEntry *entry;
  for (i = 0; i < queue_len; i++) {
    entry = &(queue[(queue_start + i) % FAT_COOP_QUEUE_SIZE]);

    sublength = 0;
    if (entry->op == COOP_CFS_CLOSE || entry->op == COOP_CFS_READ || entry->op == COOP_CFS_WRITE) {
      sublength = entry->parameters.generic.length;
    }

    estimated_time += _get_time_of_operation(entry->op, sublength);
  }
  return estimated_time;
}
/*----------------------------------------------------------------------------*/
uint8_t
fat_buffer_available(uint16_t length)
{
  uint16_t pos = (writeBuffer_start + writeBuffer_len) % FAT_COOP_BUFFER_SIZE;
  uint16_t free = 0;

  if (pos == writeBuffer_start && writeBuffer_len != 0) {
    return 0;
  }

  if (pos < writeBuffer_start) {
    free = writeBuffer_start - pos;
  } else {
    free = (FAT_COOP_BUFFER_SIZE - pos) + writeBuffer_start;
  }

  if (length > free) {
    return 0;
  }

  return 1;
}
/*----------------------------------------------------------------------------*/

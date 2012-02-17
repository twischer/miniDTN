#include "fat_coop.h"

enum {
	READ = 1,
	WRITE,
	INTERNAL
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

static uint8_t writeBuffer[FAT_COOP_BUFFER_SIZE];
static uint16_t writeBuffer_start, writeBuffer_len;

static QueueEntry queue[FAT_COOP_QUEUE_SIZE];
static uint16_t queue_start, queue_len;

static uint8_t coop_step_allowed = 0;

#define MS_TO_TICKS(ms) ((uint32_t)(ms * (RTIMER_SECOND / 1000.0)))

#define FAT_COOP_SLOT_SIZE_TICKS MS_TO_TICKS(FAT_COOP_SLOT_SIZE_MS)
#define FAT_COOP_TIME_READ_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_READ_BLOCK_MS)
#define FAT_COOP_TIME_WRITE_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_WRITE_BLOCK_MS)

#define FAT_COOP_STACK_SIZE 256

static uint8_t stack[FAT_COOP_STACK_SIZE];
static uint8_t *sp = 0;
static uint8_t *sp_save = 0;
static uint8_t next_step_type = INTERNAL;

void operation(void *data) {
	QueueEntry *entry = (QueueEntry *) data;
	
	coop_step_allowed = 1;
	
	if( entry->state != INPROGRESS ) {
		entry->state = INPROGRESS;
	}
	
	switch(entry->op){
		case CFS_OPEN:
			//_ccfs_open( entry );
			entry->ret_value = cfs_open( entry->parameters.open.name, entry->parameters.open.flags );
			break;
		case CFS_CLOSE:
			cfs_close( entry->parameters.generic.fd );
			break;
		case CFS_WRITE:
			entry->ret_value = cfs_write( entry->parameters.generic.fd, entry->parameters.generic.buffer,
				entry->parameters.generic.length );
			break;
		case CFS_READ:
			entry->ret_value = cfs_read( entry->parameters.generic.fd, entry->parameters.generic.buffer,
				entry->parameters.generic.length );
			break;
		case CFS_SEEK:
			entry->ret_value = cfs_seek( entry->parameters.seek.fd, entry->parameters.seek.offset,
				entry->parameters.seek.whence );
			break;
		case CFS_REMOVE:
			entry->ret_value = cfs_remove( entry->parameters.open.name );
			break;
		case CFS_OPENDIR:
			entry->ret_value = cfs_opendir( entry->parameters.dir.dirp, entry->parameters.dir.u.name );
			break
		case CFS_READDIR:
			entry->ret_value = cfs_readdir( entry->parameters.dir.dirp, entry->parameters.dir.u.dirent );
			break;
		case CFS_CLOSEDIR:
			entry->ret_value = cfs_closedir( entry->parameters.dir.dirp );
			break;
	}
	entry->state = DONE;
}

/**
 * This Function jumps back to perform_next_step()
 */
void coop_finsihed_op() {
	/* Change SP back to original */
	coop_switch_sp();
	/* Init the internal Stack for the next Operation */
	coop_mt_init();
}

/**
 * This function is mostly copied from the arm/mtarch.c file.
 */
void coop_mt_init( void *data ) {
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

  /* Renable interrupts */
  sei ();
}

PROCESS(fsp, "FileSystemProcess");
PROCESS_THREAD(fsp, ev, data)
{
	PROCESS_BEGIN();	
	rtimer_arch_init();
	static uint32_t start_time = RTIMER_NOW();
	static QueueEntry *entry = queue_current_entry();
	while( entry != NULL && time_left_for_step( entry, start_time ) ) {
		perform_next_step( entry );
		if( entry->state == DONE ) {
			finish_operation( entry );
		}
	}
	if( queue_len == 0 ) {
		try_internal_operation();
	} else {
		//reschedule
	}
	PROCESS_END();
}

/**
 * Changes Stack pointer to internal Thread. Returns after one step has been completed.
 */
void perform_next_step( QueueEntry *entry ) {
	if( sp == NULL ) {
		coop_mt_init( (void *) entry );
	}
	coop_switch_sp();
}

void finish_operation( QueueEntry *entry ) {
	// Send Event to source process
	// Reset memory
	// Remove entry
	queue_rm_top_entry();
}

uint8_t time_left_for_step( QueueEntry *entry, uint32_t start_time ) {
	uint8_t step_type = 0;
	uint32_t time_left = 0;
	step_type = next_step_type;
	if( step_type == INTERNAL ) {
		return 1;
	}
	
	time_left = FAT_COOP_SLOT_SIZE_TICKS - (RTIMER_NOW() - start_time);
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
	if( pos == queue_start ) {
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
	memset( &(queue[queue_start]), 0, sizeof(QueueEntry) );
	queue_start = (queue_start + 1) % FAT_COOP_QUEUE_SIZE;
	queue_len--;
	return 0;
}

void push_on_buffer( uint8_t *source, uint16_t length ) {
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
}

void pop_from_buffer( uint16_t length ) {
	if( length > writeBuffer_len ) {
		return ;
	}
	writeBuffer_start = (writeBuffer_start + length) % FAT_COOP_BUFFER_SIZE;
	writeBuffer_len -= length;
}


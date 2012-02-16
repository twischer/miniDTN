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
	uint8_t progress;
} QueueEntry;

static uint8_t writeBuffer[FAT_COOP_BUFFER_SIZE];
static uint16_t writeBuffer_start, writeBuffer_len;

static QueueEntry queue[FAT_COOP_QUEUE_SIZE];
static uint16_t queue_start, queue_len;

#define MS_TO_TICKS(ms) ((uint32_t)(ms * (RTIMER_SECOND / 1000.0)))

#define FAT_COOP_SLOT_SIZE_TICKS MS_TO_TICKS(FAT_COOP_SLOT_SIZE_MS)
#define FAT_COOP_TIME_READ_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_READ_BLOCK_MS)
#define FAT_COOP_TIME_WRITE_BLOCK_TICKS MS_TO_TICKS(FAT_COOP_TIME_WRITE_BLOCK_MS)

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

void perform_next_step( QueueEntry *entry ) {
	dispatch( entry, 0 );
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
	step_type = dispatch( entry, 1 );
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

uint8_t dispatch( QueueEntry *entry, uint8_t flag ) {
	switch(entry->op){
		case CFS_OPEN:
			return _ccfs_open( entry, flag );
		case CFS_CLOSE:
			return _ccfs_close( entry, flag );
		case CFS_WRITE:
			return _ccfs_write( entry, flag );
		case CFS_READ:
			return _ccfs_read( entry, flag );
		case CFS_SEEK:
			return _ccfs_seek( entry, flag );
		case CFS_REMOVE:
			return _ccfs_remove( entry, flag );
		case CFS_OPENDIR:
			return _ccfs_opendir( entry, flag );
		case CFS_READDIR:
			return _ccfs_readdir( entry, flag );
		case CFS_CLOSEDIR:
			return _ccfs_closedir( entry, flag );
		default:
			return 0;
	}
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


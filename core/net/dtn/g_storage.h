#ifndef __G_STORAGE_H__
#define __G_STORAGE_H__

#define BUNDLE_STORAGE_SIZE 10
#define BUNDLE_STARAGE_FILE_NAME "list_file"
#include "net/dtn/bundle.h"

struct file_list_entry_t{
	uint16_t bundle_num;
	uint16_t file_size;
	uint32_t lifetime;
	uint32_t src;
	uint32_t time_stamp;
	uint32_t time_stamp_seq;
	uint32_t fraq_offset;
};
extern struct file_list_entry_t file_list[BUNDLE_STORAGE_SIZE];



void init(void);
/**
returns bundle_num
*/
int32_t save_bundle(uint8_t *offset_tab, struct bundle_t *bundle);

uint16_t del_bundle(uint16_t bundle_num);

uint16_t read_bundle(uint16_t bundle_num, struct bundle_t *bundle);
#endif

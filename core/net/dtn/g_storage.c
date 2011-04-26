#include "core/net/dtn/storage.h"
#include "core/net/dtn/dtn_config.h"
#include "core/net/dtn/bundle.h"
#include "core/net/sdnv.h"

struct file_list_entry_t file_list[BUNDLE_STORAGE_SIZE];
char *filename = BUNDLE_STARAGE_FILE_NAME; 
int fd_write, fd_read;


void init(void){
	fd_read = cfs_open(filename, CFS_READ);
	if(fd_read!=-1) {
		cfs_read(fd_read,file_list,8*BUNDLE_STORAGE_SIZE);
		cfs_close(fd_read);
	}else{
		uint16_t i;
		for(i=0; i < BUNDLE_STORAGE_SIZE; i++){
			file_list[i].bundle_num=i;
			file_list[i].file_size=0;
			file_list[i].lifetime=0;
		}
	}


int32_t save_bundle(uint8_t *offset_tab, struct bundle_t *bundle){
	uint16_t i=0;
	int32_t free=-1;
	uint8_t *tmp=bundle->block;
	tmp=tmp+bundle->offset_tab[SRC_NODE][OFFSET];
	uint32_t src;
	sdnv_decode(tmp, , &src);
	tmp=bundle->block+bundle->offset_tab[TIME_STAMP][OFFSET];
	uint32_t time_stamp;
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP][STATE], &time_stamp);
	tmp=bundle->block+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET];
	uint32_t time_stamp_seq;
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE], &time_stamp_seq);
	tmp=bundle->block+bundle->offset_tab[FRAG_OFFSET][OFFSET];
	uint32_t fraq_offset;
	sdnv_decode(tmp, bundle->offset_tab[FRAG_OFFSET][STATE], &fraq_offset);
	
	while ( i < BUNDLE_STORAGE_SIZE) {
		if (free == -1){
			file_list[i].file_size == 0
			free=(int32_t)i;
		}
		if ( time_stamp_seq == file_list[i].time_stamp_seq && 
		    time_stamp == file_list[i].time_stamp &&
		    src == file_list[i].src &&
		    fraq_offset == file_list[i].fraq_offset) {  // is bundle in storage?
			return (int32_t)i;
		}
		i++;
	}
	if(free == -1){
		return -1;
	}
	i--;
	
	file_list[i].file_size = bundle->size+sizeof(bundle->offset_tab); 
	tmp=bundle->block+bundle->offset_tab[LIFE_TIME][OFFSET];
	sdnv_decode(tmp, bundle->offset_tab[LIFE_TIME][STATE], &file_list[i].lifetime);
	
	char b_file[5];
	sprintf(b_file,"%u",file_list[i].file_num);
	fd_write = cfs_open(b_file, CFS_WRITE);
	int n=0;
	if(fd_write != -1) {
		n = cfs_write(fd_write, bundle->offset_tab, sizeof(bundle->offset_tab));
		cfs_close(fd_write);
	}else{
		return -1;
	}
	fd_write = cfs_open(b_file, CFS_WRITE | CFS_APPEND);
	if(fd_write != -1) {
		n += cfs_write(fd_write, bundle->block, bundel->size);
		cfs_close(fd_write);
	}else{
		return -1;
	}
	return n;
		



}


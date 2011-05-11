#include "contiki.h"
#include "net/dtn/storage.h"
#include "cfs/cfs.h"
#include "net/dtn/g_storage.h"
#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

struct file_list_entry_t file_list[BUNDLE_STORAGE_SIZE];
char *filename = BUNDLE_STARAGE_FILE_NAME; 
int fd_write, fd_read;


void init(void)
{
	PRINTF("init g_storage\n");
	fd_read = cfs_open(filename, CFS_READ);
	if(fd_read!=-1) {
		PRINTF("file opened\n");
		cfs_read(fd_read,file_list,29*BUNDLE_STORAGE_SIZE);
		cfs_close(fd_read);
		PRINTF("file closed\n");
		#if DEBUG
		uint16_t i;
	//	file_list[0].file_size=1;
		for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
			PRINTF("slot %u state is %u\n", i, file_list[i].file_size);
		}
		#endif
	}else{
		PRINTF("no file found\n");
		uint16_t i;
		for(i=0; i < BUNDLE_STORAGE_SIZE; i++){
			file_list[i].bundle_num=i;
			file_list[i].file_size=0;
			file_list[i].lifetime=0;
			PRINTF("deleting old bundles\n");
			del_bundle(i);	
		}
		PRINTF("write new list-file\n");
		fd_write = cfs_open(filename, CFS_WRITE);
		PRINTF("file opened\n");
		cfs_write(fd_write, file_list, sizeof(file_list));
		PRINTF("write inro new file\n");
		cfs_close(fd_write);
		PRINTF("file closed\n");
	}
}

void reinit(void)
{
	uint16_t i;
	cfs_remove(filename);
	for(i=0; i < BUNDLE_STORAGE_SIZE; i++){
		file_list[i].bundle_num=i;
		file_list[i].file_size=0;
		file_list[i].lifetime=0;
		del_bundle(i);
		fd_write = cfs_open(filename, CFS_WRITE);
		cfs_write(fd_write, file_list, sizeof(file_list));
		cfs_close(fd_write);
	}

}

int32_t save_bundle(struct bundle_t *bundle)
{
	uint16_t i=0;
	int32_t free=-1;
	uint8_t *tmp=bundle->block;
	tmp=tmp+bundle->offset_tab[SRC_NODE][OFFSET];
	uint32_t src;
	sdnv_decode(tmp ,bundle->offset_tab[SRC_NODE][STATE], &src);
	tmp=bundle->block+bundle->offset_tab[TIME_STAMP][OFFSET];
	uint32_t time_stamp;
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP][STATE], &time_stamp);
	tmp=bundle->block+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET];
	uint32_t time_stamp_seq;
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE], &time_stamp_seq);
	tmp=bundle->block+bundle->offset_tab[FRAG_OFFSET][OFFSET];
	uint32_t fraq_offset;
	sdnv_decode(tmp, bundle->offset_tab[FRAG_OFFSET][STATE], &fraq_offset);

		#if DEBUG
		for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
			PRINTF("slot %u state is %u\n", i, file_list[i].file_size);
		}
		i=0;
		#endif
	
	while ( i < BUNDLE_STORAGE_SIZE) {
		if (free == -1 && file_list[i].file_size == 0){
			free=(int32_t)i;
			PRINTF("%u is a free slot\n",i);
		}
		if ( time_stamp_seq == file_list[i].time_stamp_seq && 
		    time_stamp == file_list[i].time_stamp &&
		    src == file_list[i].src &&
		    fraq_offset == file_list[i].fraq_offset) {  // is bundle in storage?
		    	PRINTF("%u is the same bundle\n",i);
			return (int32_t)i;
		}//else{
		//	PRINTF("bundle are different\n");
		//}
		i++;
	}
	if(free == -1){
		PRINTF("no free slots in bundlestorage\n");
		return -1;
	}
	i=(uint16_t)free;
	PRINTF("bundle will be safed in solt %u, size of bundle is %u\n",i,bundle->size);	
	file_list[i].file_size = bundle->size; 
		#if DEBUG
		for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
			PRINTF("b slot %u state is %u\n", i, file_list[i].file_size);
		}
		i=0;
		#endif
	i=(uint16_t)free;
	tmp=bundle->block+bundle->offset_tab[LIFE_TIME][OFFSET];
	sdnv_decode(tmp, bundle->offset_tab[LIFE_TIME][STATE], &file_list[i].lifetime);
	
	char b_file[7];
	sprintf(b_file,"%u.b",file_list[i].bundle_num);
	PRINTF("filename: %s\n", b_file);
	fd_write = cfs_open(b_file, CFS_WRITE);
	int n=0;
	fd_write = cfs_open(b_file, CFS_WRITE | CFS_APPEND);
	if(fd_write != -1) {
		n = cfs_write(fd_write, bundle->block, bundle->size);
		cfs_close(fd_write);
	}else{
		return -1;
	}
	if (n != bundle->size){
		PRINTF("write failed\n");
		return -1;
	}
	file_list[i].time_stamp_seq = time_stamp_seq;
	file_list[i].time_stamp = time_stamp;
	file_list[i].src = src ;
	file_list[i].fraq_offset = fraq_offset;
	file_list[i].rec_time= bundle->rec_time;
	//file_list[i].custody=bundle->custody;
	//save file list	
	cfs_remove(filename);
	fd_write = cfs_open(filename, CFS_WRITE);
	if(fd_write != -1) {
		cfs_write(fd_write, file_list, sizeof(file_list));
		cfs_close(fd_write);
	}else{
		return -2;
	}
	return (int32_t)file_list[i].bundle_num;
}

uint16_t del_bundle(uint16_t bundle_num)
{
	char b_file[7];
	sprintf(b_file,"%u.b",bundle_num);
	cfs_remove(b_file);
	file_list[bundle_num].file_size=0;
	file_list[bundle_num].src=0;
	//save file list	
	cfs_remove(filename);
	fd_write = cfs_open(filename, CFS_WRITE);
	if(fd_write != -1) {
		cfs_write(fd_write, file_list, sizeof(file_list));
		cfs_close(fd_write);
	}else{
		return 0;
	}
	return 1;
}

uint16_t read_bundle(uint16_t bundle_num,struct bundle_t *bundle)
{
	char b_file[7];
	sprintf(b_file,"%u.b",bundle_num);
	fd_read = cfs_open(b_file, CFS_READ);
	
	if(fd_read!=-1) {
#if DEBUG
		uint8_t i;
		for (i = 0; i<20; i++){
			PRINTF("val in [%u]; %u ,%u\n",i,bundle->offset_tab[i][0], bundle->offset_tab[i][1]);
		}
#endif
		PRINTF("file-size %u\n", file_list[bundle_num].file_size);
		bundle->block = (uint8_t *) malloc(file_list[bundle_num].file_size);
		cfs_read(fd_read, bundle->block, file_list[bundle_num].file_size);
		cfs_close(fd_read);
		recover_bundel(bundle,bundle->block,(int) file_list[bundle_num].file_size);
		bundle->rec_time=file_list[bundle_num].rec_time;
		bundle->custody = file_list[bundle_num].custody;
		PRINTF("first byte in bundel %u\n",*bundle->block);
		return file_list[bundle_num].file_size;
	}
	return 0;
}

uint16_t free_space(struct bundle_t *bundle)
{
	uint16_t free=0, i;
	for (i =0; i< BUNDLE_STORAGE_SIZE; i++){
		if(file_list[i].file_size == 0){
			free++;
		}
	}
	return free;
}

const struct storage_driver g_storage = {
	"G_STORAGE",
	init,
	reinit,
	save_bundle,
	del_bundle,
	read_bundle,
	free_space,
};

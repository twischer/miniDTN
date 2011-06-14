#include "contiki.h"
#include "net/dtn/storage.h"
#include "cfs/cfs.h"
#include "net/dtn/g_storage.h"
#include "net/dtn/bundle.h"
#include "net/dtn/sdnv.h"
#include "agent.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define R_DEBUG 1
#if R_DEBUG
#include <stdio.h>
#define R_PRINTF(...) printf(__VA_ARGS__)
#else
#define R_PRINTF(...)
#endif
struct file_list_entry_t file_list[BUNDLE_STORAGE_SIZE];
char *filename = BUNDLE_STARAGE_FILE_NAME; 
int fd_write, fd_read;
static uint16_t bundles_in_storage;
static struct ctimer g_store_timer;


void init(void)
{
	PRINTF("init g_storage\n");
	fd_read = cfs_open(filename, CFS_READ);
	bundles_in_storage=0;
	if(fd_read!=-1) {
		PRINTF("file opened\n");
		cfs_read(fd_read,file_list,29*BUNDLE_STORAGE_SIZE);
		cfs_close(fd_read);
		PRINTF("file closed\n");
		uint16_t i;
	//	file_list[0].file_size=1;
		for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
			PRINTF("slot %u state is %u\n", i, file_list[i].file_size);
			if(file_list[i].file_size >0 ){
				bundles_in_storage++;
			}
		}
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
//	PRINTF("STORAGE: schedule ctimer\n");
	ctimer_set(&g_store_timer,CLOCK_SECOND*5,g_store_reduce_lifetime,NULL);
}

void g_store_reduce_lifetime()
{
	uint16_t i=0;
	for(i=0; i < BUNDLE_STORAGE_SIZE;i++) {
		if (file_list[i].file_size >0){

			if( file_list[i].lifetime < (uint32_t)6){
				PRINTF("STORAGE: bundle lifetime expired of bundle %u\n",i);
				del_bundle(i);
			}else{
				file_list[i].lifetime-=5;
				PRINTF("STORAGE: remaining lifefime of bundle %u : %lu\n",i,file_list[i].lifetime);
			}
		}
	}
	ctimer_restart(&g_store_timer);
	
}

void reinit(void)
{
	uint16_t i;
	cfs_remove(filename);
	bundles_in_storage=0;
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
			PRINTF("STORAGE: slot %u state is %u\n", i, file_list[i].file_size);
		}
		i=0;
		#endif
	
	while ( i < BUNDLE_STORAGE_SIZE) {
		if (free == -1 && file_list[i].file_size == 0){
			free=(int32_t)i;
			PRINTF("STORAGE: %u is a free slot\n",i);
		}
		if ( time_stamp_seq == file_list[i].time_stamp_seq && 
		    time_stamp == file_list[i].time_stamp &&
		    src == file_list[i].src &&
		    fraq_offset == file_list[i].fraq_offset) {  // is bundle in storage?
		    	PRINTF("STORAGE: %u is the same bundle\n",i);
			return (int32_t)i;
		}//else{
		//	PRINTF("bundle are different\n");
		//}
		i++;
	}
	if(free == -1){
		uint16_t index=0;
		uint32_t min_lifetime=-1;
		int32_t delet=-1;

		while ( index < BUNDLE_STORAGE_SIZE) {
			if (file_list[index].file_size>0 && file_list[index].lifetime < min_lifetime){
				delet=(int32_t) index;
				min_lifetime=file_list[index].lifetime;
			}
			index++;
		}
		if (delet !=-1){
			printf("STORAGE: del %u\n",delet);
			del_bundle(delet);
			free=delet;
		}
//		PRINTF("STORAGE: no free slots in bundlestorage\n");
//		return -1;
	}
	i=(uint16_t)free;
	PRINTF(" STORAGE: bundle will be safed in solt %u, size of bundle is %u\n",i,bundle->size);	
	file_list[i].file_size = bundle->size; 
		#if DEBUG
		for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
			PRINTF("STORAGE: b slot %u state is %u\n", i, file_list[i].file_size);
		}
		i=0;
		#endif
	i=(uint16_t)free;
	tmp=bundle->block+bundle->offset_tab[LIFE_TIME][OFFSET];
	//sdnv_decode(tmp, bundle->offset_tab[LIFE_TIME][STATE], &file_list[i].lifetime);
	file_list[i].lifetime=bundle->lifetime;
	char b_file[7];
	sprintf(b_file,"%u.b",file_list[i].bundle_num);
	PRINTF("STORAGE: write filename: %s\n", b_file);
	fd_write = cfs_open(b_file, CFS_WRITE);
	int n=0;
	PRINTF("STORAGE: write filename: %s opened\n", b_file);
#if DEBUG
	PRINTF("STORAGE: bundle->block: ");
	uint8_t j;
	for(j=0;j<bundle->size;j++){
		PRINTF("%u:",*(bundle->block+j));
	}
	PRINTF("\n");
#endif
	if(fd_write != -1) {
		n = cfs_write(fd_write, bundle->block, bundle->size);
		cfs_close(fd_write);
		bundles_in_storage++;
	}else{
		PRINTF("STORAGE: write failed\n");
		return -1;
	}
	if (n != bundle->size){
		PRINTF("STORAGE: write failed\n");
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
		PRINTF("STORAGE: write failed\n");
		return -2;
	}
	PRINTF("STORAGE: bundle_num %u\n",file_list[i].bundle_num);
	return (int32_t)file_list[i].bundle_num;
}

uint16_t del_bundle(uint16_t bundle_num)
{
	uint16_t *num;
	num = malloc(2);
	if (num==NULL){
		printf("\n\n MALLOC ERROR\n\n");
	}

	*num=bundle_num;
	char b_file[7];
	sprintf(b_file,"%u.b",bundle_num);
	cfs_remove(b_file);
	if (bundles_in_storage >0){
		bundles_in_storage--;
	}
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
	

	process_post(&agent_process,dtn_bundle_deleted_event, num);
	return 1;
}

uint16_t read_bundle(uint16_t bundle_num,struct bundle_t *bundle)
{
	char b_file[7];
	sprintf(b_file,"%u.b",bundle_num);
	fd_read = cfs_open(b_file, CFS_READ);
	
	if(fd_read != -1) {
		PRINTF("file-size %u\n", file_list[bundle_num].file_size);
		bundle->block = (uint8_t *) malloc(file_list[bundle_num].file_size);
		if (bundle->block==NULL){
			printf("\n\n MALLOC ERROR\n\n");
		}

		cfs_read(fd_read, bundle->block, file_list[bundle_num].file_size);
		cfs_close(fd_read);
#if R_DEBUG
		uint8_t i;
		R_PRINTF(" STORAGE 1: ");
		for (i=0; i<20; i++){
			R_PRINTF("%x:",*(bundle->block+i));
		}
		R_PRINTF("\n");
#endif
#if DEBUG
		uint8_t i;
		PRINTF("STORAGE: bundle->block: ");
		for (i = 0; i<file_list[bundle_num].file_size; i++){
			PRINTF("%u:",*(bundle->block+i));
		}
		PRINTF("\n");
#endif
		if( !recover_bundel(bundle,bundle->block,(int) file_list[bundle_num].file_size)){
			return 0;
		}

#if DEBUG
		for (i = 0; i<17; i++){
			PRINTF("STORAGE: val in [%u]; %u ,%u\n",i,bundle->offset_tab[i][0], bundle->offset_tab[i][1]);
		}
#endif
		bundle->rec_time=file_list[bundle_num].rec_time;
		bundle->custody = file_list[bundle_num].custody;
		PRINTF("STORAGE: first byte in bundel %u\n",*bundle->block);
		bundle->lifetime=file_list[bundle_num].lifetime;
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

uint16_t get_g_bundel_num(void){
	return bundles_in_storage;
}

const struct storage_driver g_storage = {
	"G_STORAGE",
	init,
	reinit,
	save_bundle,
	del_bundle,
	read_bundle,
	free_space,
	get_g_bundel_num,
};

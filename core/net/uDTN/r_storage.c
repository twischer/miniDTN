/**
 * \addtogroup bstorage
 * @{
 */

 /**
 * \defgroup r_storage RAM storage modules
 *
 * @{
 */

/**
 * \file 
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de)
 */

#include "contiki.h"

#include "storage.h"
#include "bundle.h"
#include "sdnv.h"
#include "agent.h"
#include "lib/mmem.h"
#include "lib/list.h"
#include "r_storage.h"
#include <stdlib.h>
#include <stdio.h>
#include "dtn_config.h"
#include "status-report.h"
#include "forwarding.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

struct file_list_entry_t file_list[BUNDLE_STORAGE_SIZE];
int fd_write, fd_read;
static uint16_t bundles_in_storage;
static struct ctimer r_store_timer;
struct memb_blocks *saved_as_mem;
LIST(store_l);
uint16_t del_num;

void r_store_reduce_lifetime();
/**
* /brief called by agent at startup
*/
void rs_init(void)
{
	PRINTF("init g_storage\n");
	mmem_init();
	list_init(store_l);
	//fd_read = cfs_open(filename, CFS_READ);
	bundles_in_storage=0;
	MEMB(saved_as_memb,uint16_t , 100);
        saved_as_mem=&saved_as_memb;
        memb_init(saved_as_mem);

	uint8_t i;	
	for(i=0; i < BUNDLE_STORAGE_SIZE; i++){
		file_list[i].bundle_num=i;
		file_list[i].file_size=0;
		file_list[i].lifetime=0;
		PRINTF("deleting old bundles\n");
		rs_del_bundle(i,4);	
	}
	ctimer_set(&r_store_timer,CLOCK_SECOND*5,r_store_reduce_lifetime,NULL);
}
/**
* \brief reduces the lifetime of all stored bundles
*/
void r_store_reduce_lifetime()
{
        uint16_t i=0;
        for(i=0; i < BUNDLE_STORAGE_SIZE;i++) {
                if (file_list[i].file_size >0){

                        if( file_list[i].lifetime < (uint32_t)6){
                                PRINTF("STORAGE: bundle lifetime expired of bundle %u\n",i);
                                rs_del_bundle(i,1);
                        }else{
                                file_list[i].lifetime-=5;
                                PRINTF("STORAGE: remaining lifefime of bundle %u : %lu\n",i,file_list[i].lifetime);
                        }
                }
        }
        ctimer_restart(&r_store_timer);
}


void rs_reinit(void)
{
	uint16_t i;
	bundles_in_storage=0;
	for(i=0; i < BUNDLE_STORAGE_SIZE; i++){
		file_list[i].bundle_num=i;
		file_list[i].file_size=0;
		file_list[i].lifetime=0;
		rs_del_bundle(i,4);
	}

}
/**
* \brief saves a bundle in storage
* \param bundle pointer to the bundle
* \return the bundle number given to the bundle or <0 on errors
*/
int32_t rs_save_bundle(struct bundle_t *bundle)
{
	uint16_t i=0;
	int32_t free=-1;
	uint8_t *tmp=bundle->mem.ptr;
	tmp=tmp+bundle->offset_tab[SRC_NODE][OFFSET];
	uint32_t src;
	sdnv_decode(tmp ,bundle->offset_tab[SRC_NODE][STATE], &src);
	tmp=bundle->mem.ptr+bundle->offset_tab[TIME_STAMP][OFFSET];
	uint32_t time_stamp;
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP][STATE], &time_stamp);
	tmp=bundle->mem.ptr+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET];
	uint32_t time_stamp_seq;
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE], &time_stamp_seq);
	tmp=bundle->mem.ptr+bundle->offset_tab[FRAG_OFFSET][OFFSET];
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
//		PRINTF("STORAGE: no free slots in bundlestorage\n");
//		return -1;

                uint16_t index=0,nodel_winner=0,nodel_max=0;
                uint32_t min_lifetime=-1;
                int32_t delet=-1;
		uint8_t mult_old=0;

                while ( index < BUNDLE_STORAGE_SIZE) {
                        if (file_list[index].file_size>0 && file_list[index].lifetime < min_lifetime){
                                delet=(int32_t) index;
                                min_lifetime=file_list[index].lifetime;
                        }
			if (file_list[index].lifetime == min_lifetime){
				
				file_list[index].not_del++;
//				printf("not_del %lu %u\n", file_list[index].time_stamp_seq, file_list[index].not_del);
				if(nodel_max<=file_list[index].not_del){
					nodel_winner=index;
					nodel_max=file_list[index].not_del;
				}
				mult_old=1;
			}
			index++;
                }
		if (mult_old){
			delet=(int32_t)nodel_winner;
//			printf("mult_old\n");
		}
                if (delet !=-1){
//			printf("delete %u\n", file_list[delet].bundle_num);
			file_list[delet].not_del=0;
                        PRINTF("STORAGE: del %ld\n",delet);

                        PRINTF("STORAGE: bundle->mem.ptr %p (%p + %p)\n", bundle->mem.ptr, bundle, &bundle->mem );
                        if(!rs_del_bundle(delet,4)){
                                return -1;
                        }
                        PRINTF("STORAGE: bundle->mem.ptr %p (%p + %p)\n", bundle->mem.ptr, bundle, &bundle->mem);
                        free=delet;
                }

	}
	i=(uint16_t)free;
	PRINTF(" STORAGE: bundle will be safed in solt %u, size of bundle is %u\n",i,bundle->size);	
	file_list[i].file_size = bundle->size; 
		#if DEBUG
		for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
			PRINTF("STORAGE: b slot %u state is %u %p\n", i, file_list[i].file_size,file_list);
		}
		i=0;
		#endif
	i=(uint16_t)free;
	tmp=bundle->mem.ptr+bundle->offset_tab[LIFE_TIME][OFFSET];
	sdnv_decode(tmp, bundle->offset_tab[LIFE_TIME][STATE], &file_list[i].lifetime);
	
#if DEBUG
	PRINTF("STORAGE: bundle->block: ");
	uint8_t j;
	for(j=0;j<bundle->size;j++){
		PRINTF("%u:",*((uint8_t*)bundle->mem.ptr+j));
	}
	PRINTF("\n");
#endif
	int mem = mmem_alloc(&file_list[i].ptr,bundle->size);
	if(mem) {
		memcpy(file_list[i].ptr.ptr,bundle->mem.ptr,bundle->size);
		bundles_in_storage++;
		PRINTF("bundles_in_storage %u\n",bundles_in_storage);
	}else{
		PRINTF("STORAGE: write failed\n");
		return -1;
	}
	file_list[i].time_stamp_seq = time_stamp_seq;
	file_list[i].time_stamp = time_stamp;
	file_list[i].src = src ;
	file_list[i].fraq_offset = fraq_offset;
	file_list[i].rec_time= bundle->rec_time;
	//printf("stored seq_nr %lu in %u\n",time_stamp_seq, file_list[i].bundle_num);
	PRINTF("STORAGE: bundle_num %u %p\n",file_list[i].bundle_num, file_list[i]);
	return (int32_t)file_list[i].bundle_num;
}

static uint16_t gl_bundle_num;
static struct bundle_t bundle_str;
/**
* \brief delets a bundle form storage
* \param bundle_num bundle number to be deleted
* \param reason reason code
* \return 1 on succes or 0 on error
*/
uint16_t rs_del_bundle(uint16_t bundle_num,uint8_t reason)
{
	if(bundles_in_storage >0){
		if(rs_read_bundle(bundle_num,&bundle_str)){
			bundle_str.del_reason=reason;
			del_num=bundle_num;
			if( ((bundle_str.flags & 8 ) || (bundle_str.flags & 0x40000)) &&(reason !=0xff )){
				uint32_t src;
				sdnv_decode(bundle_str.mem.ptr+ bundle_str.offset_tab[SRC_NODE][OFFSET],bundle_str.offset_tab[SRC_NODE][STATE],&src);
				if (src != dtn_node_id){
					STATUS_REPORT.send(&bundle_str,16,bundle_str.del_reason);
				}
			}
		}
		delete_bundle(&bundle_str);

		if (MMEM_PTR(&file_list[bundle_num].ptr) != 0){
			mmem_free(&file_list[bundle_num].ptr);
		}	
		bundles_in_storage--;
		file_list[bundle_num].file_size=0;
		file_list[bundle_num].src=0;
		//save file list	
		gl_bundle_num =bundle_num;
	//	process_post(&agent_process,dtn_bundle_deleted_event, &gl_bundle_num);
		agent_del_bundle();
	}
	return 1;
}

/**
* \brief reads a bundle from storage
* \param bundle_num bundle nuber to read
* \param bundle empty bundle struct, bundle will be accessable here
* \return 1 on succes or 0 on error
*/
uint16_t rs_read_bundle(uint16_t bundle_num,struct bundle_t *bundle)
{
	
	if(MMEM_PTR(&file_list[bundle_num].ptr) != 0) {
		PRINTF("file-size %u %p %u\n", file_list[bundle_num].file_size, file_list[bundle_num], bundle_num);
		mmem_alloc(&bundle->mem,file_list[bundle_num].file_size);
		memcpy(bundle->mem.ptr, file_list[bundle_num].ptr.ptr, file_list[bundle_num].file_size);
#if DEBUG
		uint8_t i;
		for (i = 0; i<17; i++){
		//	PRINTF("val in [%u]; %u ,%u\n",i,bundle->offset_tab[i][0], bundle->offset_tab[i][1]);
		}
		PRINTF("test\n");
#endif
		recover_bundel(bundle,&bundle->mem,(int) file_list[bundle_num].file_size);
#if DEBUG
		for (i = 0; i<17; i++){
	//		PRINTF("STORAGE: val in [%u]; %u ,%u\n",i,bundle->offset_tab[i][0], bundle->offset_tab[i][1]);
		}
		PRINTF("test\n");
#endif
		PRINTF("STORAGE: 11111\n");
		bundle->rec_time=file_list[bundle_num].rec_time;
		bundle->custody = file_list[bundle_num].custody;
		PRINTF("STORAGE: first byte in bundel %u\n",*((uint8_t*)bundle->mem.ptr));
		memcpy(bundle->msrc.u8,file_list[bundle_num].msrc.u8,sizeof(bundle->msrc.u8));
		return file_list[bundle_num].file_size;
	}
	return 0;
}
/**
* \brief checks if there is space for a bundle
* \param bundle pointer to a bundel struct (not used here)
* \return number of free solts
*/
uint16_t rs_free_space(struct bundle_t *bundle)
{
	uint16_t free=0, i;
	for (i =0; i< BUNDLE_STORAGE_SIZE; i++){
		if(file_list[i].file_size == 0){
			free++;
		}
	}
	return free;
}
/**
* \returns the number of saved bundles
*/
uint16_t rs_get_g_bundel_num(void){
	return bundles_in_storage;
}
const struct storage_driver r_storage = {
	"R_STORAGE",
	rs_init,
	rs_reinit,
	rs_save_bundle,
	rs_del_bundle,
	rs_read_bundle,
	rs_free_space,
	rs_get_g_bundel_num,
};

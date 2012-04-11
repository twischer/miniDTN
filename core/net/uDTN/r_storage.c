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
#include <string.h>
#include "dtn_config.h"
#include "status-report.h"
#include "forwarding.h"
#include "profiling.h"
#include "statistics.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// defined in mmem.c, no function to access it though
extern unsigned int avail_memory;

struct file_list_entry_t file_list[BUNDLE_STORAGE_SIZE];
static uint16_t bundles_in_storage;
static struct ctimer r_store_timer;
struct memb *saved_as_mem;
static struct bundle_t bundle_str;

void r_store_reduce_lifetime();

/**
 * \brief internal function to send statistics to statistics module
 */
void rs_update_statistics() {
	statistics_storage_bundles(bundles_in_storage);
	statistics_storage_memory(avail_memory);
}

/**
* /brief called by agent at startup
*/
void rs_init(void)
{
	PRINTF("STORAGE: init r_storage\n");

	mmem_init();

	bundles_in_storage=0;
	MEMB(saved_as_memb, uint16_t, 50);
	saved_as_mem = &saved_as_memb;
	memb_init(saved_as_mem);

	rs_reinit();
	rs_update_statistics();

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
			if( file_list[i].storage_lifetime < (uint32_t)6){
				PRINTF("STORAGE: bundle lifetime expired of bundle %u\n",i);
				rs_del_bundle(i,1);
			}else{
				PRINTF("STORAGE: remaining lifetime of bundle %u : %lu\n",i,file_list[i].storage_lifetime);
				file_list[i].storage_lifetime -= 5;
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
		file_list[i].bundle_num = i;
		file_list[i].file_size = 0;
		file_list[i].storage_lifetime = 0;
		rs_del_bundle(i,4);
	}

}

/**
 * This function delete as many bundles from the storage as necessary to
 * have at least one slot and the number of required of memory free
 * besides the high watermark for MMEM
 */
uint8_t rs_make_room(struct bundle_t * bundle)
{
	if( bundles_in_storage < BUNDLE_STORAGE_SIZE && (avail_memory - bundle->size) > STORAGE_HIGH_WATERMARK ) {
		// We have enough memory, no need to do anything
		return 1;
	}

	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE || (avail_memory - bundle->size) < STORAGE_HIGH_WATERMARK ) {
		// We need space, delete bundles
		int i;
		uint32_t maximum_storagetime_value = 0;
		uint16_t pointer_maximum_storagetime = -1;

		for(i=0; i<BUNDLE_STORAGE_SIZE; i++) {
			if( file_list[i].file_size < 1 ) {
				continue;
			}

			if( (clock_seconds() - file_list[i].rec_time) > maximum_storagetime_value ) {
				// Here we have a bundle that could be deleted
				maximum_storagetime_value = clock_seconds() - file_list[i].rec_time;
				pointer_maximum_storagetime = i;
			}
		}

		if( pointer_maximum_storagetime == -1 ) {
			PRINTF("STORAGE: no bundle for deletion found\n");
			return -1;
		}

		if( !rs_del_bundle(pointer_maximum_storagetime, 4) ){
				return -1;
		}

	}

	return 1;
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

	uint32_t src;
	tmp=tmp+bundle->offset_tab[SRC_NODE][OFFSET];
	sdnv_decode(tmp ,bundle->offset_tab[SRC_NODE][STATE], &src);

	uint32_t dest;
	tmp=bundle->mem.ptr+bundle->offset_tab[DEST_NODE][OFFSET];
	sdnv_decode(tmp ,bundle->offset_tab[DEST_NODE][STATE], &dest);

	uint32_t time_stamp;
	tmp=bundle->mem.ptr+bundle->offset_tab[TIME_STAMP][OFFSET];
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP][STATE], &time_stamp);

	uint32_t time_stamp_seq;
	tmp=bundle->mem.ptr+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET];
	sdnv_decode(tmp, bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE], &time_stamp_seq);

	uint32_t fraq_offset;
	tmp=bundle->mem.ptr+bundle->offset_tab[FRAG_OFFSET][OFFSET];
	sdnv_decode(tmp, bundle->offset_tab[FRAG_OFFSET][STATE], &fraq_offset);

#if DEBUG
	for (i=0; i<BUNDLE_STORAGE_SIZE; i++){
		PRINTF("STORAGE: Slot %u, Size %u, Lifetime %lu, Src %lu, Dest %lu, SeqNo %lu\n", i, file_list[i].file_size, file_list[i].storage_lifetime, file_list[i].src, file_list[i].dest, file_list[i].time_stamp_seq);
	}
#endif
	
	for(i=0; i<BUNDLE_STORAGE_SIZE; i++) {
		if (free == -1 && file_list[i].file_size == 0){
			free = (int32_t) i;
			PRINTF("STORAGE: %u is a free slot\n",i);
		}

		// is bundle in storage?
		if ( time_stamp_seq == file_list[i].time_stamp_seq && 
		    time_stamp == file_list[i].time_stamp &&
		    src == file_list[i].src &&
		    fraq_offset == file_list[i].fraq_offset) {
		    	PRINTF("STORAGE: %u is the same bundle\n",i);
			return (int32_t) i;
		}
	}

	if( free == -1 ) {
		if( !rs_make_room(bundle) ) {
			// Cannot store bundle, no room
			PRINTF("STORAGE: Cannot store bundle, no room\n");
			return -1;
		}

		for(i=0; i<BUNDLE_STORAGE_SIZE; i++) {
			if (free == -1 && file_list[i].file_size == 0){
				free = (int32_t) i;
				PRINTF("STORAGE: %u is a free slot\n",i);
				break;
			}
		}
	}

	i = (uint16_t) free;
	PRINTF("STORAGE: bundle will be saved in slot %u, size of bundle is %u\n",i, bundle->size);

	// Clear the memory area
	memset(&file_list[i], 0, sizeof(struct file_list_entry_t));

	// Allocate some memory
	int mem = mmem_alloc(&file_list[i].ptr,bundle->size);
	if( !mem ) {
		PRINTF("STORAGE: write failed\n");
		return -1;
	}

	bundles_in_storage++;

	// Set all required fields
	file_list[i].bundle_num = i;
	file_list[i].file_size = bundle->size;
	file_list[i].time_stamp_seq = time_stamp_seq;
	file_list[i].time_stamp = time_stamp;
	file_list[i].src = src;
	file_list[i].dest = dest;
	file_list[i].fraq_offset = fraq_offset;
	file_list[i].rec_time = bundle->rec_time;
	file_list[i].storage_lifetime = bundle->lifetime;
	rimeaddr_copy(&file_list[i].msrc, &bundle->msrc);

	// Copy bundle into memory storage
	memcpy(file_list[i].ptr.ptr,bundle->mem.ptr,bundle->size);

	PRINTF("STORAGE: New Bundle %u at %u, Src %lu, Dest %lu, Seq %lu, Size %u\n", file_list[i].bundle_num, i, src, dest, time_stamp_seq, file_list[i].file_size);

	// Notify the statistics module
	rs_update_statistics();

	return (int32_t) file_list[i].bundle_num;
}

/**
* \brief delets a bundle form storage
* \param bundle_num bundle number to be deleted
* \param reason reason code
* \return 1 on succes or 0 on error
*/
uint16_t rs_del_bundle(uint16_t bundle_num, uint8_t reason)
{
	if(bundles_in_storage < 1){
		return 1;
	}

	PRINTF("STORAGE: Deleting Bundle %u with reason %u\n", bundle_num, reason);

	if(rs_read_bundle(bundle_num, &bundle_str)){
		bundle_str.del_reason = reason;

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
	file_list[bundle_num].file_size = 0;
	file_list[bundle_num].src = 0;

	agent_del_bundle(bundle_num);

	// Notify the statistics module
	rs_update_statistics();

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
		recover_bundel(bundle, &file_list[bundle_num].ptr,(int) file_list[bundle_num].file_size);

		bundle->rec_time=file_list[bundle_num].rec_time;
		bundle->custody = file_list[bundle_num].custody;
		rimeaddr_copy(&bundle->msrc, &file_list[bundle_num].msrc);

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

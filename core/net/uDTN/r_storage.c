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

LIST(bundle_list);
MEMB(bundle_mem, struct file_list_entry_t, BUNDLE_STORAGE_SIZE);

static uint16_t bundles_in_storage;
static struct ctimer r_store_timer;
static uint16_t bundle_number = 0;

void r_store_prune();

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

	// Initialize the neighbour list
	list_init(bundle_list);

	// Initialize the neighbour memory block
	memb_init(&bundle_mem);

	// Initialize MMEM for the binary bundle storage
	mmem_init();

	bundles_in_storage = 0;
	bundle_number = 0;

	rs_reinit();
	rs_update_statistics();

	ctimer_set(&r_store_timer, CLOCK_SECOND*5, r_store_prune, NULL);
}

/**
* \brief deletes expired bundles from storage
*/
void r_store_prune()
{
	uint32_t elapsed_time;
	struct file_list_entry_t * entry = NULL;
	struct bundle_t *bundle = NULL;

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
		elapsed_time = clock_seconds() - bundle->rec_time;

		if( bundle->lifetime < elapsed_time ) {
			PRINTF("STORAGE: bundle lifetime expired of bundle %u\n", entry->bundle_num);
			rs_del_bundle(bundle->bundle_num, REASON_LIFETIME_EXPIRED);
		}
	}

	ctimer_restart(&r_store_timer);
}


void rs_reinit(void)
{
	struct file_list_entry_t * entry = NULL;
	struct bundle_t *bundle = NULL;

	// Start counting the bundle number from zero again
	bundle_number = 0;

	// Delete all bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		rs_del_bundle(bundle->bundle_num, REASON_NO_INFORMATION);
	}
}

/**
 * This function delete as many bundles from the storage as necessary to
 * have at least one slot and the number of required of memory free
 * besides the high watermark for MMEM
 */
uint8_t rs_make_room(struct mmem *bundlemem)
{
//	if( bundles_in_storage < BUNDLE_STORAGE_SIZE && (avail_memory - bundle->block.block_size) > STORAGE_HIGH_WATERMARK ) {
//		// We have enough memory, no need to do anything
//		return 1;
//	}

	// Now delete expired bundles
	r_store_prune();

	// Keep deleting bundles until we have enough MMEM and slots
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) { // || (avail_memory - bundle->size) < STORAGE_HIGH_WATERMARK ) {
		struct file_list_entry_t * entry = list_head(bundle_list);

		if( entry == NULL ) {
			// We do not have bundles in storage, stop deleting them
			break;
		}

		rs_del_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}

	return 1;
}

/**
* \brief saves a bundle in storage
* \param bundle pointer to the bundle
* \return the bundle number given to the bundle or <0 on errors
*/
int32_t rs_save_bundle(struct mmem * bundlemem)
{
	struct bundle_t *entrybdl = NULL,
					*bundle = NULL;
	struct file_list_entry_t * entry;

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		entrybdl = (struct bundle_t *) MMEM_PTR(entry->bundle);
		if ( bundle->tstamp_seq == entrybdl->tstamp_seq &&
		    bundle->tstamp == entrybdl->tstamp &&
		    bundle->src_node == entrybdl->src_node &&
		    bundle->frag_offs == entrybdl->frag_offs) {

			PRINTF("STORAGE: %u is the same bundle\n", entry->bundle_num);
			return (int32_t) bundle->bundle_num;
		}
	}

	if( !rs_make_room(bundlemem) ) {
		printf("STORAGE: Cannot store bundle, no room\n");
		return -1;
	}

	// Now we have to update the pointer to our bundle, because MMEM may have been modified (freed) and thus the pointer may have changed
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	entry = memb_alloc(&bundle_mem);
	if( entry == NULL ) {
		printf("STORAGE: unable to allocate struct, cannot store bundle\n");
		return -1;
	}

	// Clear the memory area
	memset(entry, 0, sizeof(struct file_list_entry_t));

	// we copy the reference to the bundle, therefore we have to increase the reference counter
	entry->bundle = bundlemem;
	bundle_inc(bundlemem);
	bundles_in_storage++;

	// Set all required fields
	bundle->bundle_num = bundle_number++;
	entry->bundle_num = bundle->bundle_num;

	PRINTF("STORAGE: New Bundle %u (%u), Src %lu, Dest %lu, Seq %lu\n", bundle->bundle_num, entry->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq);

	// Notify the statistics module
	rs_update_statistics();

	// Add bundle to the list
	list_add(bundle_list, entry);

	// Now we have to (virtually) free the incoming bundle slot
	// This should do nothing, as we have incremented the reference counter before
	bundle_dec(bundlemem);

	// Now we have to send an event to our daemon
	// We can use the pointer to our internal bundle number, as the
	// memory is not in MMEM and therefore the address is static
	process_post(&agent_process, dtn_bundle_in_storage_event, &entry->bundle_num);

	return (int32_t) bundle->bundle_num;
}

/**
* \brief deletes a bundle form storage
* \param bundle_num bundle number to be deleted
* \param reason reason code
* \return 1 on success or 0 on error
*/
uint16_t rs_del_bundle(uint16_t bundle_number, uint8_t reason)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;

	PRINTF("STORAGE: Deleting Bundle %u with reason %u\n", bundle_number, reason);

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle->bundle_num == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		PRINTF("STORAGE: Could not find bundle %u on rs_del_bundle\n", bundle_number);
		return 0;
	}

	// Figure out the source to send status report
	bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
	bundle->del_reason = reason;

	if( reason != REASON_DELIVERED ) {
		// REASON_DELIVERED means "bundle delivered"
		if( (bundle->flags & 8 ) || (bundle->flags & 0x40000) ){
			if (bundle->src_node != dtn_node_id){
				STATUS_REPORT.send(bundle, 16, bundle->del_reason);
			}
		}
	}

	bundle_dec(entry->bundle);

	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	bundles_in_storage--;

	// Notified the agent, that a bundle has been deleted
	agent_del_bundle(bundle_number);

	// Notify the statistics module
	rs_update_statistics();

	// Free the storage struct
	memb_free(&bundle_mem, entry);

	return 1;
}

/**
* \brief reads a bundle from storage
* \param bundle_num bundle nuber to read
* \param bundle empty bundle struct, bundle will be accessable here
* \return 1 on succes or 0 on error
*/
struct mmem *rs_read_bundle(uint16_t bundle_num)
{
	struct file_list_entry_t * entry = NULL;
	struct bundle_t * bundle = NULL;

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle->bundle_num == bundle_num ) {
			break;
		}
	}

	if( entry == NULL ) {
		printf("STORAGE: Could not find bundle %u in rs_read_bundle\n", bundle_num);
		return 0;
	}

	if( entry->bundle->size == 0 ) {
		printf("STORAGE: Found bundle %u but file size is %u\n", bundle_num, entry->bundle->size);
		return 0;
	}

	// Someone requested the bundle, he will have to decrease the reference counter again
	bundle_inc(entry->bundle);

	return entry->bundle;
}


/**
* \brief checks if there is space for a bundle
* \param bundle pointer to a bundel struct (not used here)
* \return number of free solts
*/
uint16_t rs_free_space(struct mmem *bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
* \returns the number of saved bundles
*/
uint16_t rs_get_g_bundel_num(void){
	return bundles_in_storage;
}

/**
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * rs_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
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
	rs_get_bundles,
};

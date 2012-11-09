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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "lib/mmem.h"
#include "lib/list.h"

#include "bundle.h"
#include "sdnv.h"
#include "agent.h"
#include "statusreport.h"
#include "profiling.h"
#include "statistics.h"
#include "hash.h"

#include "storage.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// defined in mmem.c, no function to access it though
extern unsigned int avail_memory;

/**
 * Internal representation of a bundle
 *
 * The layout is quite fixed - the next pointer and the bundle_num have to go first because this struct
 * has to be compatible with the struct storage_entry_t in storage.h!
 */
struct bundle_list_entry_t {
	/** pointer to the next list element */
	struct bundle_list_entry_t * next;

	/** copy of the bundle number - necessary to have
	 * a static address that we can pass on as an
	 * argument to an event
	 */
	uint32_t bundle_num;

	/** pointer to the actual bundle stored in MMEM */
	struct mmem *bundle;
};

// List and memory blocks for the bundles
LIST(bundle_list);
MEMB(bundle_mem, struct bundle_list_entry_t, BUNDLE_STORAGE_SIZE);

// global, internal variables
/** Counts the number of bundles in storage */
static uint16_t bundles_in_storage;

/** Is used to periodically traverse all bundles and delete those that are expired */
static struct ctimer r_store_timer;

/**
 * "Internal" functions
 */
void storage_mmem_prune();
void storage_mmem_reinit(void);
uint16_t storage_mmem_delete_bundle(uint32_t bundle_number, uint8_t reason);
void storage_mmem_update_statistics();

/**
 * \brief internal function to send statistics to statistics module
 */
void storage_mmem_update_statistics() {
	statistics_storage_bundles(bundles_in_storage);
	statistics_storage_memory(avail_memory);
}

/**
* /brief called by agent at startup
*/
void storage_mmem_init(void)
{
	PRINTF("STORAGE: init r_storage\n");

	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	// Initialize MMEM for the binary bundle storage
	mmem_init();

	bundles_in_storage = 0;

	storage_mmem_reinit();
	storage_mmem_update_statistics();

	ctimer_set(&r_store_timer, CLOCK_SECOND*5, storage_mmem_prune, NULL);
}

/**
* \brief deletes expired bundles from storage
*/
void storage_mmem_prune()
{
	uint32_t elapsed_time;
	struct bundle_list_entry_t * entry = NULL;
	struct bundle_t *bundle = NULL;

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
		elapsed_time = clock_seconds() - bundle->rec_time;

		if( bundle->lifetime < elapsed_time ) {
			PRINTF("STORAGE: bundle lifetime expired of bundle %lu\n", entry->bundle_num);
			storage_mmem_delete_bundle(bundle->bundle_num, REASON_LIFETIME_EXPIRED);
		}
	}

	ctimer_restart(&r_store_timer);
}

void storage_mmem_reinit(void)
{
	struct bundle_list_entry_t * entry = NULL;
	struct bundle_t *bundle = NULL;

	// Delete all bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

		storage_mmem_delete_bundle(bundle->bundle_num, REASON_NO_INFORMATION);
	}
}

/**
 * This function delete as many bundles from the storage as necessary to
 * have at least one slot and the number of required of memory free
 * besides the high watermark for MMEM
 */
uint8_t storage_mmem_make_room(struct mmem *bundlemem)
{
//	if( bundles_in_storage < BUNDLE_STORAGE_SIZE && (avail_memory - bundle->block.block_size) > STORAGE_HIGH_WATERMARK ) {
//		// We have enough memory, no need to do anything
//		return 1;
//	}

	// Now delete expired bundles
	storage_mmem_prune();

	// Keep deleting bundles until we have enough MMEM and slots
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) { // || (avail_memory - bundle->size) < STORAGE_HIGH_WATERMARK ) {
		struct bundle_list_entry_t * entry = list_head(bundle_list);

		if( entry == NULL ) {
			// We do not have bundles in storage, stop deleting them
			break;
		}

		storage_mmem_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}

	return 1;
}

/**
* \brief saves a bundle in storage
* \param bundlemem pointer to the bundle
* \param bundle_number pointer where the bundle number will be stored (on success)
* \return 0 on error, 1 on success
*/
uint8_t storage_mmem_save_bundle(struct mmem * bundlemem, uint32_t ** bundle_number_ptr)
{
	struct bundle_t *entrybdl = NULL,
					*bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;
	uint32_t bundle_number = 0;

	if( bundlemem == NULL ) {
		printf("STORAGE: rs_save_bundle with invalid pointer %p\n", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		printf("STORAGE: rs_save_bundle with invalid MMEM structure\n");
		return 0;
	}

	// Calculate the bundle number
	bundle_number = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs);

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		entrybdl = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle_number == entrybdl->bundle_num ) {
			PRINTF("STORAGE: %lu is the same bundle\n", entry->bundle_num);
			bundle_decrement(bundlemem);
			return 1;
		}
	}

	if( !storage_mmem_make_room(bundlemem) ) {
		printf("STORAGE: Cannot store bundle, no room\n");
		return 0;
	}

	// Now we have to update the pointer to our bundle, because MMEM may have been modified (freed) and thus the pointer may have changed
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	entry = memb_alloc(&bundle_mem);
	if( entry == NULL ) {
		printf("STORAGE: unable to allocate struct, cannot store bundle\n");
		bundle_decrement(bundlemem);
		return 0;
	}

	// Clear the memory area
	memset(entry, 0, sizeof(struct bundle_list_entry_t));

	// we copy the reference to the bundle, therefore we have to increase the reference counter
	entry->bundle = bundlemem;
	bundle_increment(bundlemem);
	bundles_in_storage++;

	// Set all required fields
	bundle->bundle_num = bundle_number;
	entry->bundle_num = bundle_number;

	PRINTF("STORAGE: New Bundle %lu (%lu), Src %lu, Dest %lu, Seq %lu\n", bundle->bundle_num, entry->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq);

	// Notify the statistics module
	storage_mmem_update_statistics();

	// Add bundle to the list
	list_add(bundle_list, entry);

	// Now we have to (virtually) free the incoming bundle slot
	// This should do nothing, as we have incremented the reference counter before
	bundle_decrement(bundlemem);

	// Now copy over the STATIC pointer to the bundle number, so that
	// the caller can stick it into an event
	*bundle_number_ptr = &entry->bundle_num;

	return 1;
}

/**
* \brief deletes a bundle form storage
* \param bundle_num bundle number to be deleted
* \param reason reason code
* \return 1 on success or 0 on error
*/
uint16_t storage_mmem_delete_bundle(uint32_t bundle_number, uint8_t reason)
{
	struct bundle_t * bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;

	PRINTF("STORAGE: Deleting Bundle %lu with reason %u\n", bundle_number, reason);

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
		printf("STORAGE: Could not find bundle %lu on storage_mmem_delete_bundle\n", bundle_number);
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

	bundle_decrement(entry->bundle);

	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	bundles_in_storage--;

	// Notified the agent, that a bundle has been deleted
	agent_delete_bundle(bundle_number);

	// Notify the statistics module
	storage_mmem_update_statistics();

	// Free the storage struct
	memb_free(&bundle_mem, entry);

	return 1;
}

/**
* \brief reads a bundle from storage
* \param bundle_num bundle number to read
* \return pointer to the MMEM struct
*/
struct mmem *storage_mmem_read_bundle(uint32_t bundle_num)
{
	struct bundle_list_entry_t * entry = NULL;
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
		printf("STORAGE: Could not find bundle %lu in rs_read_bundle\n", bundle_num);
		return 0;
	}

	if( entry->bundle->size == 0 ) {
		printf("STORAGE: Found bundle %lu but file size is %u\n", bundle_num, entry->bundle->size);
		return 0;
	}

	// Someone requested the bundle, he will have to decrease the reference counter again
	bundle_increment(entry->bundle);

	return entry->bundle;
}


/**
* \brief checks if there is space for a bundle
* \param bundle pointer to a bundle struct (not used here)
* \return number of free slots
*/
uint16_t storage_mmem_get_free_space(struct mmem *bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
* \returns the number of saved bundles
*/
uint16_t storage_mmem_get_bundle_numbers(void){
	return bundles_in_storage;
}

/**
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * storage_mmem_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
}

const struct storage_driver storage_mmem = {
	"R_STORAGE",
	storage_mmem_init,
	storage_mmem_reinit,
	storage_mmem_save_bundle,
	storage_mmem_delete_bundle,
	storage_mmem_read_bundle,
	storage_mmem_get_free_space,
	storage_mmem_get_bundle_numbers,
	storage_mmem_get_bundles,
};

/** @} */
/** @} */

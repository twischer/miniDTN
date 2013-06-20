/**
 * \addtogroup bundle_storage
 * @{
 */

/**
 * \defgroup bundle_storage_mmem MMEM-based temporary Storage
 *
 * @{
 */

/**
 * \file 
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "lib/mmem.h"
#include "lib/list.h"
#include "logging.h"

#include "bundle.h"
#include "sdnv.h"
#include "agent.h"
#include "statusreport.h"
#include "profiling.h"
#include "statistics.h"
#include "hash.h"

#include "storage.h"

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

	/** Flags */
	uint8_t flags;

	/** pointer to the actual bundle stored in MMEM */
	struct mmem *bundle;
};

/**
 * Flags for the storage
 */
#define STORAGE_MMEM_FLAGS_LOCKED 	0x1

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
 * \brief called by agent at startup
 */
void storage_mmem_init(void)
{
	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "storage_mmem init");

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
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "bundle lifetime expired of bundle %lu", entry->bundle_num);
			storage_mmem_delete_bundle(bundle->bundle_num, REASON_LIFETIME_EXPIRED);
		}
	}

	ctimer_restart(&r_store_timer);
}

/**
 * \brief Sets the storage to its initial state
 */
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
 * \brief This function delete as many bundles from the storage as necessary to have at least one slot and the number of required of memory free
 * \param bundlemem Pointer to the MMEM struct containing the bundle
 * \return 1 on success, 0 if no room could be made free
 */
uint8_t storage_mmem_make_room(struct mmem * bundlemem)
{
	/* Now delete expired bundles */
	storage_mmem_prune();

	/* If we do not have a pointer, we cannot compare - do nothing */
	if( bundlemem == NULL ) {
		return 0;
	}

#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DO_NOT_DELETE
	/* We do not delete at all. If storage is used up, we sit there and wait */
	if( bundles_in_storage >= BUNDLE_STORAGE_SIZE ) {
		return 0;
	}
#elif (BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDEST || BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGEST )
	struct bundle_t * bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;

	/* Keep deleting bundles until we have enough slots */
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) {
		unsigned long comparator = 0;
		struct bundle_list_entry_t * deletor = NULL;

		for( entry = list_head(bundle_list);
			 entry != NULL;
			 entry = list_item_next(entry) ) {

			bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);

			/* Never delete locked bundles */
			if( entry->flags & STORAGE_MMEM_FLAGS_LOCKED ) {
				continue;
			}

#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDEST
			if( (clock_seconds() - bundle->rec_time) > comparator || comparator == 0) {
				comparator = clock_seconds() - bundle->rec_time;
				deletor = entry;
			}
#elif BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGEST
			if( (clock_seconds() - bundle->rec_time) < comparator || comparator == 0) {
				comparator = clock_seconds() - bundle->rec_time;
				deletor = entry;
			}
#endif
		}

		/* Either the for loop did nothing or did not break */
		if( entry == NULL ) {
			/* We do not have deletable bundles in storage, stop deleting them */
			return 0;
		}

		/* Delete Bundle */
		storage_mmem_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}
#elif (BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDER || BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGER )
	struct bundle_t * bundle_new = NULL;
	struct bundle_t * bundle_old = NULL;
	struct bundle_list_entry_t * entry = NULL;

	/* Keep deleting bundles until we have enough slots */
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) {
		/* Obtain the new pointer each time, since the address may change */
		bundle_new = (struct bundle_t *) MMEM_PTR(bundlemem);

		/* We need this double-loop because otherwise we would be modifying the list
		 * while iterating through it
		 */
		for( entry = list_head(bundle_list);
			 entry != NULL;
			 entry = list_item_next(entry) ) {
			bundle_old = (struct bundle_t *) MMEM_PTR(entry->bundle);

			/* Never delete locked bundles */
			if( entry->flags & STORAGE_MMEM_FLAGS_LOCKED ) {
				continue;
			}

#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDER
			/* If the new bundle has a longer lifetime than the bundle in our storage,
			 * delete the bundle from storage to make room
			 */
			if( bundle_new->lifetime - (clock_seconds() - bundle_new->rec_time) >= bundle_old->lifetime - (clock_seconds() - bundle_old->rec_time) ) {
				break;
			}
#elif BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGER
			/* Delete youngest bundle in storage */
			if( bundle_new->lifetime - (clock_seconds() - bundle_new->rec_time) >= bundle_old->lifetime - (clock_seconds() - bundle_old->rec_time) ) {
				break;
			}
#endif
		}

		/* Either the for loop did nothing or did not break */
		if( entry == NULL ) {
			/* We do not have deletable bundles in storage, stop deleting them */
			return 0;
		}

		/* Delete Bundle */
		storage_mmem_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}
#else
#error No Bundle Deletion Strategy defined
#endif

	return 1;
}

/**
 * \brief saves a bundle in storage
 * \param bundlemem pointer to the bundle
 * \param bundle_number_ptr pointer where the bundle number will be stored (on success)
 * \return 0 on error, 1 on success
 */
uint8_t storage_mmem_save_bundle(struct mmem * bundlemem, uint32_t ** bundle_number_ptr)
{
	struct bundle_t *entrybdl = NULL,
					*bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;
	uint32_t bundle_number = 0;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "storage_mmem_save_bundle with invalid pointer %p", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "storage_mmem_save_bundle with invalid MMEM structure");
		bundle_decrement(bundlemem);
		return 0;
	}

	// Calculate the bundle number
	bundle_number = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs, bundle->app_len);

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {
		entrybdl = (struct bundle_t *) MMEM_PTR(entry->bundle);

		if( bundle_number == entrybdl->bundle_num ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "%lu is the same bundle", entry->bundle_num);
			*bundle_number_ptr = &entry->bundle_num;
			bundle_decrement(bundlemem);
			return 1;
		}
	}

	if( !storage_mmem_make_room(bundlemem) ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Cannot store bundle, no room");

		/* Throw away bundle to not take up RAM */
		bundle_decrement(bundlemem);

		return 0;
	}

	// Now we have to update the pointer to our bundle, because MMEM may have been modified (freed) and thus the pointer may have changed
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	entry = memb_alloc(&bundle_mem);
	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot store bundle");
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

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "New Bundle %lu (%lu), Src %lu, Dest %lu, Seq %lu", bundle->bundle_num, entry->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq);

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
 * \brief deletes a bundle from storage
 * \param bundle_number bundle number to be deleted
 * \param reason reason code
 * \return 1 on success or 0 on error
 */
uint16_t storage_mmem_delete_bundle(uint32_t bundle_number, uint8_t reason)
{
	struct bundle_t * bundle = NULL;
	struct bundle_list_entry_t * entry = NULL;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Deleting Bundle %lu with reason %u", bundle_number, reason);

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
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Could not find bundle %lu on storage_mmem_delete_bundle", bundle_number);
		return 0;
	}

	// Figure out the source to send status report
	bundle = (struct bundle_t *) MMEM_PTR(entry->bundle);
	bundle->del_reason = reason;

	if( reason != REASON_DELIVERED ) {
		if( (bundle->flags & BUNDLE_FLAG_CUST_REQ ) || (bundle->flags & BUNDLE_FLAG_REP_DELETE) ){
			if (bundle->src_node != dtn_node_id){
				STATUSREPORT.send(entry->bundle, 16, bundle->del_reason);
			}
		}
	}

	// Notified the agent, that a bundle has been deleted
	agent_delete_bundle(bundle_number);

	bundle_decrement(entry->bundle);
	bundle = NULL;

	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	bundles_in_storage--;

	// Notify the statistics module
	storage_mmem_update_statistics();

	// Free the storage struct
	memb_free(&bundle_mem, entry);

	return 1;
}

/**
 * \brief reads a bundle from storage
 * \param bundle_num bundle number to read
 * \return pointer to the MMEM struct, NULL on error
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
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Could not find bundle %lu in storage_mmem_read_bundle", bundle_num);
		return 0;
	}

	if( entry->bundle->size == 0 ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Found bundle %lu but file size is %u", bundle_num, entry->bundle->size);
		return 0;
	}

	// Someone requested the bundle, he will have to decrease the reference counter again
	bundle_increment(entry->bundle);

	/* How long did this bundle rot in our storage? */
	uint32_t elapsed_time = clock_seconds() - bundle->rec_time;

	/* Update lifetime of bundle */
	if( bundle->lifetime < elapsed_time ) {
		bundle->lifetime = 0;
		bundle->rec_time = clock_seconds();
	} else {
		bundle->lifetime = bundle->lifetime - elapsed_time;
		bundle->rec_time = clock_seconds();
	}

	return entry->bundle;
}

/**
 * \brief checks if there is space for a bundle
 * \param bundlemem pointer to a bundle struct (not used here)
 * \return number of free slots
 */
uint16_t storage_mmem_get_free_space(struct mmem * bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
 * \brief Get the number of slots available in storage
 * \returns the number of free slots
 */
uint16_t storage_mmem_get_bundle_numbers(void){
	return bundles_in_storage;
}

/**
 * \brief Get the bundle list
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * storage_mmem_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
}

/**
 * \brief Mark a bundle as locked so that it will not be deleted even if we are running out of space
 *
 * \param bundle_num Bundle number
 * \return 1 on success or 0 on error
 */
uint8_t storage_mmem_lock_bundle(uint32_t bundle_num)
{
	struct bundle_list_entry_t * entry = NULL;

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( entry->bundle_num == bundle_num ) {
			break;
		}
	}

	if( entry == NULL ) {
		return 0;
	}

	entry->flags |= STORAGE_MMEM_FLAGS_LOCKED;

	return 1;
}

/**
 * \brief Mark a bundle as unlocked after being locked previously
 */
void storage_mmem_unlock_bundle(uint32_t bundle_num)
{
	struct bundle_list_entry_t * entry = NULL;

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( entry->bundle_num == bundle_num ) {
			break;
		}
	}

	if( entry == NULL ) {
		return;
	}

	entry->flags &= ~STORAGE_MMEM_FLAGS_LOCKED;
}


const struct storage_driver storage_mmem = {
	"STORAGE_MMEM",
	storage_mmem_init,
	storage_mmem_reinit,
	storage_mmem_save_bundle,
	storage_mmem_delete_bundle,
	storage_mmem_read_bundle,
	storage_mmem_lock_bundle,
	storage_mmem_unlock_bundle,
	storage_mmem_get_free_space,
	storage_mmem_get_bundle_numbers,
	storage_mmem_get_bundles,
};

/** @} */
/** @} */

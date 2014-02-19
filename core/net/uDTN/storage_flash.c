/**
 * \addtogroup bundle_storage
 * @{
 */

/**
 * \defgroup bundle_storage_flash Flash-based persitent storage
 *
 * @{
 */

/**
 * \file 
 * \author Wolf-Bastian Pöttner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "lib/list.h"
#include "logging.h"
#include "watchdog.h"

/* <> is necessary to include the header in the platform (instead of our fake header) */
#include <flash_arch.h>

#include "bundle.h"
#include "sdnv.h"
#include "agent.h"
#include "statusreport.h"
#include "profiling.h"
#include "statistics.h"
#include "hash.h"
#include "bundle_ageing.h"
#include "system_clock.h"

#include "storage.h"

struct storage_flash_entry_t {
	/** pointer to the next list element */
	struct storage_entry_t * next;

	/** Internal number of the bundle */
	uint32_t bundle_num;

	/** Internal flash for the storage */
	uint8_t storage_flags;

	/** Flash page where bundle is stored */
	uint16_t page;

	/** Flags of the primary bundle block */
	uint32_t bundle_flags;

	/** Timestamp at which the bundle will expire */
	uint32_t expiration;
};

struct storage_flash_page_t {
	uint32_t tag;
	uint32_t size;
	uint8_t bundle[FLASH_PAGE_SIZE - 8];
};

struct mmem * last_bundle = NULL;
uint32_t last_bundle_number = 0;

struct storage_flash_page_t page_buffer;

// List and memory blocks for the bundles
LIST(bundle_list);
MEMB(bundle_mem, struct storage_flash_entry_t, BUNDLE_STORAGE_SIZE);

/**
 * Flags for the storage
 */
#define STORAGE_FLASH_FLAGS_LOCKED 	0x1

/**
 * "Magic" tag to detect flash pages that contain a bundle
 */
#define STORAGE_FLASH_TAG			0xAA55A5AA

/** Is used to periodically traverse all bundles and delete those that are expired */
static struct ctimer storage_flash_timer;

uint32_t bundles_in_storage = 0;

#if BUNDLE_STORAGE_SIZE > FLASH_PAGES
#error More storage slots than flash pages, not supported
#endif

/**
 * We want to use as many flash pages as possible (for wear leveling)
 * However, using too many pages makes scanning and formatting very slow
 * Therefore we use 4 times as many pages as we have slots in the storage
 */
#if (4 * BUNDLE_STORAGE_SIZE) > FLASH_PAGES
#define FLASH_PAGES_IN_USE FLASH_PAGES
#else
#define FLASH_PAGES_IN_USE (4 * BUNDLE_STORAGE_SIZE)
#endif

/**
 * Forward declarations of our internal functions
 */
struct mmem *storage_flash_read_bundle(uint32_t bundle_number);
uint8_t storage_flash_delete_bundle(uint32_t bundle_number, uint8_t reason);
void storage_flash_reinit(void);

int storage_flash_page_in_use(uint32_t page)
{
	uint32_t tag = 0;

	FLASH_READ_PAGE(page + FLASH_PAGE_OFFSET, 0, (uint8_t *) &tag, 4);

	return tag == (STORAGE_FLASH_TAG + page);
}

void storage_flash_scan(void)
{
	struct storage_flash_entry_t * n = NULL;
	struct mmem * bundlemem = NULL;
	struct bundle_t * bundle = NULL;
	int h;
	int ret = 0;
	udtn_timeval_t tv;

	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Scanning flash for bundles");

	/* First of all we have to delete the list in memory */
	while( list_head(bundle_list) != NULL ) {
		n = list_head(bundle_list);
		list_remove(bundle_list, n);
		memb_free(&bundle_mem, n);
	}

	/* Now scan all flash pages for potential bundles */
	for(h=0; h<FLASH_PAGES_IN_USE; h++) {
		// Read bundle from flash+
		memset(&page_buffer, 0, sizeof(page_buffer));
		FLASH_READ_PAGE(h + FLASH_PAGE_OFFSET, 0, (uint8_t *) &page_buffer, sizeof(page_buffer));

		if( h % 10 == 0 ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Reading flash page %u of %u", h, FLASH_PAGES_IN_USE);
		}

		watchdog_periodic();

		// Check if page is in use
		if( page_buffer.tag == STORAGE_FLASH_TAG + h ) {

			// Allocate memory for the bundle
			bundlemem = bundle_create_bundle();
			if( bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "cannot allocate memory to resture bundle");
				return;
			}

			// Allocate the proper amount
			ret = mmem_realloc(bundlemem, page_buffer.size);
			if( !ret ) {
				LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to realloc enough memory");
				bundle_decrement(bundlemem);
				return;
			}

			// Copy data into MMEM
			memcpy(MMEM_PTR(bundlemem), page_buffer.bundle, page_buffer.size);

			bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

			// Allocate list entry for bundle
			n = memb_alloc(&bundle_mem);

			if( n == NULL ) {
				LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot restore bundle");
				return;
			}

			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Restored Bundle %lu, Src %lu, Dest %lu, Seq %lu from page %d", bundle->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq, h);

			// Fill the struct of the bundle
			n->page = h;
			n->bundle_num = bundle->bundle_num;
			n->bundle_flags = bundle->flags;

			// Figure out when the bundle is going to expire
			udtn_uptime(&tv);
			n->expiration = tv.tv_sec + (bundle->lifetime - bundle_ageing_get_age(bundlemem) / 1000);

			// Increment the storage counter
			bundles_in_storage++;

			// Add bundle to list
			list_add(bundle_list, n);

			// Deallocate bundle
			bundle_decrement(bundlemem);		}
	}
}

void storage_flash_prune()
{
	struct storage_flash_entry_t * entry = NULL;
	uint32_t * deletion[BUNDLE_STORAGE_SIZE];
	int pointer = 0;
	int h = 0;
	udtn_timeval_t tv;

	udtn_uptime(&tv);

	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Pruning expired bundles");

	memset(deletion, 0, sizeof(deletion));

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {

		if( entry->expiration <= tv.tv_sec ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "bundle lifetime expired of bundle %lu", entry->bundle_num);
			deletion[pointer++] = &entry->bundle_num;
		}
	}

	for(h=0; h<pointer; h++) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Deleting bundle %lu LT", *deletion[h]);
		storage_flash_delete_bundle(*deletion[h], REASON_LIFETIME_EXPIRED);
	}

	ctimer_restart(&storage_flash_timer);
}

void storage_flash_format(void)
{
	int h;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Formatting flash");

	/* Delete all flash pages */
	for(h=0; h<FLASH_PAGES_IN_USE; h++) {
		if( h % 10 == 0 ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Erasing page %u of %u", h, FLASH_PAGES_IN_USE);
		}

		FLASH_ERASE_PAGE(h + FLASH_PAGE_OFFSET);
		watchdog_periodic();
	}
}

void storage_flash_init(void)
{
	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "storage_flash init");

	// Initialize flash
	FLASH_INIT();

	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	bundles_in_storage = 0;

#if BUNDLE_STORAGE_INIT
	storage_flash_format();
#else
	// Look for bundles in the flash
	storage_flash_scan();
#endif

	ctimer_set(&storage_flash_timer, CLOCK_SECOND*5, storage_flash_prune, NULL);
}

void storage_flash_reinit(void)
{
	struct storage_flash_entry_t * n = NULL;

	/* Delete all flash pages */
	storage_flash_format();

	/* Delete the bundle list in memory */
	while( list_head(bundle_list) != NULL ) {
		n = list_head(bundle_list);
		list_remove(bundle_list, n);
		memb_free(&bundle_mem, n);
	}
}

uint8_t storage_flash_make_room(struct mmem * bundlemem)
{
	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Making room for new bundles");

	/* Now delete expired bundles */
	storage_flash_prune();

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
	struct storage_flash_entry_t * entry = NULL;

	/* Keep deleting bundles until we have enough slots */
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) {
		uint32_t comparator = 0;
		struct storage_flash_entry_t * deletor = NULL;

		/* Obtain the new pointer each time, since the address may change */
		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

		for( entry = list_head(bundle_list);
			 entry != NULL;
			 entry = list_item_next(entry) ) {

			/* Never delete locked bundles */
			if( entry->storage_flags & STORAGE_FLASH_FLAGS_LOCKED ) {
				continue;
			}

			/* Always keep bundles with higher priority */
			if( (entry->bundle_flags & BUNDLE_PRIORITY_MASK) > (bundle->flags & BUNDLE_PRIORITY_MASK ) ) {
				continue;
			}

#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDEST
			if( entry->expiration < comparator || comparator == 0) {
				comparator = entry->expiration;
				deletor = entry;
			}
#elif BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGEST
			if( entry->expiration > comparator || comparator == 0) {
				comparator = entry->expiration;
				deletor = entry;
			}
#endif
		}

		/* Either the for loop did nothing or did not break */
		if( deletor == NULL ) {
			/* We do not have deletable bundles in storage, stop deleting them */
			return 0;
		}

		/* Delete Bundle */
		storage_flash_delete_bundle(deletor->bundle_num, REASON_DEPLETED_STORAGE);
	}

#elif (BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDER || BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGER )
	struct bundle_t * bundle = NULL;
	struct storage_flash_entry_t * entry = NULL;
	uint32_t expiration = 0;
	udtn_timeval_t tv;

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
	udtn_uptime(&tv);
	expiration = tv.tv_sec + (bundle->lifetime - bundle_ageing_get_age(bundlemem) / 1000);

	/* Keep deleting bundles until we have enough slots */
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) {
		/* Obtain the new pointer each time, since the address may change */
		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

		/* We need this double-loop because otherwise we would be modifying the list
		 * while iterating through it
		 */
		for( entry = list_head(bundle_list);
			 entry != NULL;
			 entry = list_item_next(entry) ) {
			/* Never delete locked bundles */
			if( entry->storage_flags & STORAGE_FLASH_FLAGS_LOCKED ) {
				continue;
			}

			/* Always keep bundles with higher priority */
			if( (entry->bundle_flags & BUNDLE_PRIORITY_MASK) > (bundle->flags & BUNDLE_PRIORITY_MASK ) ) {
				continue;
			}

#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDER
			/* If the new bundle has a longer lifetime than the bundle in our storage,
			 * delete the bundle from storage to make room
			 */
			if( expiration >= entry->expiration ) {
				break;
			}
#elif BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGER
			/* Delete youngest bundle in storage */
			if( expiration < entry->expiration ) {
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
		storage_flash_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
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
uint8_t storage_flash_save_bundle(struct mmem * bundlemem, uint32_t ** bundle_number_ptr)
{
	struct storage_flash_entry_t * n = NULL;
	struct bundle_t * bundle = NULL;
	int page;
	udtn_timeval_t tv;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "storage_flash_save_bundle with invalid pointer %p", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "storage_flash_save_bundle with invalid MMEM structure");
		bundle_decrement(bundlemem);
		return 0;
	}

	/* This function call may actually change the location of our bundle in memory, so be careful! */
	if( !storage_flash_make_room(bundlemem) ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Cannot store bundle, no room");

		/* Throw away bundle to not take up RAM */
		bundle_decrement(bundlemem);

		return 0;
	}

	// Update the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	// Normally we would store a bundle in his own slot
	page = bundle->bundle_num % FLASH_PAGES_IN_USE;

	// Look for duplicates in the storage
	for(n = list_head(bundle_list);
			n != NULL;
			n = list_item_next(n) ) {

		// In case the destined slot is taken, move on
		if( n->page == page ) {
			page ++;
		}

		// If we find the bundle, return it right away (no need to do anything)
		if( n->bundle_num == bundle->bundle_num ) {
			*bundle_number_ptr = &n->bundle_num;
			bundle_decrement(bundlemem);
			return 1;
		}
	}

	// Find the next empty page
	while( storage_flash_page_in_use(page) ) {
		// TODO: This loop fails if all pages are used
		// This should not happen, since there must be space at this point
		page ++;
	}

	n = memb_alloc(&bundle_mem);

	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot store bundle");
		bundle_decrement(bundlemem);
		return 0;
	}

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "New Bundle %lu, Src %lu, Dest %lu, Seq %lu on Page %d", bundle->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq, page);

	// Fill the struct of the bundle
	n->page = page;
	n->bundle_num = bundle->bundle_num;
	n->bundle_flags = bundle->flags;

	// Figure out when the bundle is going to expire
	udtn_uptime(&tv);
	n->expiration = tv.tv_sec + (bundle->lifetime - bundle_ageing_get_age(bundlemem) / 1000);

	// Create the flash page
	memset(&page_buffer, 0, sizeof(page_buffer));
	page_buffer.tag = STORAGE_FLASH_TAG + page;
	page_buffer.size = bundlemem->size;
	memcpy(page_buffer.bundle, bundle, bundlemem->size);

	// Write the flash page
	FLASH_WRITE_PAGE(page + FLASH_PAGE_OFFSET, 0, (uint8_t *) &page_buffer, sizeof(page_buffer));

	// Increment the storage counter
	bundles_in_storage++;

	// Add bundle to list
	list_add(bundle_list, n);

	// Now copy over the STATIC pointer to the bundle number, so that
	// the caller can stick it into an event
	*bundle_number_ptr = &n->bundle_num;

	/* Always keep a pointer to the last written bundle for faster read access to it */
	if( last_bundle != NULL ) {
		bundle_decrement(last_bundle);
	}

	last_bundle = bundlemem;
	last_bundle_number = n->bundle_num;

	return 1;
}

/**
 * \brief deletes a bundle from storage
 * \param bundle_number bundle number to be deleted
 * \param reason reason code
 * \return 1 on success or 0 on error
 */
uint8_t storage_flash_delete_bundle(uint32_t bundle_number, uint8_t reason)
{
	struct bundle_t * bundle = NULL;
	struct storage_flash_entry_t * n = NULL;
	struct mmem * bundlemem = NULL;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Deleting Bundle %lu with reason %u", bundle_number, reason);

	// Look for the bundle we are talking about
	for(n = list_head(bundle_list);
		n != NULL;
		n = list_item_next(n)) {

		if( n->bundle_num == bundle_number ) {
			break;
		}
	}

	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Could not find bundle %lu on storage_flash_delete_bundle", bundle_number);
		return 0;
	}

	if( bundle_number == last_bundle_number ) {
		bundle_decrement(last_bundle);
		last_bundle = NULL;
		last_bundle_number = 0;
	}

	if( n->storage_flags & STORAGE_FLASH_FLAGS_LOCKED ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Cannot delete %lu because it is locked", bundle_number);
	}

	if( reason != REASON_DELIVERED ) {
		if( (n->bundle_flags & BUNDLE_FLAG_CUST_REQ ) || (n->bundle_flags & BUNDLE_FLAG_REP_DELETE) ){
			bundlemem = storage_flash_read_bundle(bundle_number);

			if( bundlemem != NULL ) {
				bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

				if (bundle->src_node != dtn_node_id){
					STATUSREPORT.send(bundlemem, NODE_DELETED_BUNDLE, reason);
				}

				bundle_decrement(bundlemem);
				bundle = NULL;
				bundlemem = NULL;
			} else {
				LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Could not read bundle %lu to send report", bundle_number);
			}
		}
	}

	// Notified the agent, that a bundle has been deleted
	agent_delete_bundle(bundle_number);

	// Erase the flash page
	FLASH_ERASE_PAGE(n->page + FLASH_PAGE_OFFSET);

	// Remove the bundle from the list
	list_remove(bundle_list, n);

	bundles_in_storage--;

	// Free the storage struct
	memb_free(&bundle_mem, n);

	return 1;
}

/**
 * \brief reads a bundle from storage
 * \param bundle_num bundle number to read
 * \return pointer to the MMEM struct, NULL on error
 */
struct mmem *storage_flash_read_bundle(uint32_t bundle_number)
{
	struct storage_flash_entry_t * n = NULL;
	struct mmem * bundlemem = NULL;
	int ret = 0;

	if( last_bundle != NULL && bundle_number == last_bundle_number ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Reading bundle %lu from cache", bundle_number);
		bundle_increment(last_bundle);
		return last_bundle;
	}

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Reading bundle %lu from flash", bundle_number);

	// Look for the bundle we are talking about
	for(n = list_head(bundle_list);
		n != NULL;
		n = list_item_next(n)) {

		if( n->bundle_num == bundle_number ) {
			break;
		}
	}

	if( n == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Could not find bundle %lu in storage_flash_read_bundle", bundle_number);
		return NULL;
	}

	// Read bundle from flash+
	memset(&page_buffer, 0, sizeof(page_buffer));
	FLASH_READ_PAGE(n->page + FLASH_PAGE_OFFSET, 0, (uint8_t *) &page_buffer, sizeof(page_buffer));

	// Check if the page really contains a bundle
	if( page_buffer.tag != (STORAGE_FLASH_TAG + n->page) ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Bundle %lu should be on page %u but is not", bundle_number, n->page);
		return NULL;
	}

	// Allocate memory for the bundle
	bundlemem = bundle_create_bundle();
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "cannot allocate memory for bundle %lu", bundle_number);
		return NULL;
	}

	// Allocate the proper amount
	ret = mmem_realloc(bundlemem, page_buffer.size);
	if( !ret ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to realloc enough memory");
		bundle_decrement(bundlemem);
		return NULL;
	}

	// Copy data into MMEM
	memcpy(MMEM_PTR(bundlemem), page_buffer.bundle, page_buffer.size);

	/* Always keep a pointer to the last read bundle for faster re-read access to it */
	if( last_bundle != NULL ) {
		bundle_decrement(last_bundle);
	}

	last_bundle = bundlemem;
	last_bundle_number = n->bundle_num;
	bundle_increment(last_bundle);

	return bundlemem;
}

/**
 * \brief checks if there is space for a bundle
 * \param bundlemem pointer to a bundle struct (not used here)
 * \return number of free slots
 */
uint16_t storage_flash_get_free_space(struct mmem * bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
 * \brief Get the number of slots available in storage
 * \returns the number of free slots
 */
uint16_t storage_flash_get_bundle_numbers(void){
	return bundles_in_storage;
}

/**
 * \brief Get the bundle list
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * storage_flash_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
}

/**
 * \brief Mark a bundle as locked so that it will not be deleted even if we are running out of space
 *
 * \param bundle_num Bundle number
 * \return 1 on success or 0 on error
 */
uint8_t storage_flash_lock_bundle(uint32_t bundle_num)
{
	struct storage_flash_entry_t * entry = NULL;

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

	entry->storage_flags |= STORAGE_FLASH_FLAGS_LOCKED;

	return 1;
}

/**
 * \brief Mark a bundle as unlocked after being locked previously
 */
void storage_flash_unlock_bundle(uint32_t bundle_num)
{
	struct storage_flash_entry_t * entry = NULL;

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

	entry->storage_flags &= ~STORAGE_FLASH_FLAGS_LOCKED;
}


const struct storage_driver storage_flash = {
	"STORAGE_flash",
	storage_flash_init,
	storage_flash_reinit,
	storage_flash_save_bundle,
	storage_flash_delete_bundle,
	storage_flash_read_bundle,
	storage_flash_lock_bundle,
	storage_flash_unlock_bundle,
	storage_flash_get_free_space,
	storage_flash_get_bundle_numbers,
	storage_flash_get_bundles,
	storage_flash_format,
};

/** @} */
/** @} */

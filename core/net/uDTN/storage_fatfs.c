/**
 * \addtogroup bundle_storage
 * @{
 */

/**
 * \defgroup bundle_storage_fatfs FATFS-based persistent Storage
 *
 * @{
 */

/**
 * \file 
 * \author Timo Wischer <wischer@ibr.cs.tu-bs.de>
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "timers.h"
#include "ff.h"

#include "net/netstack.h"
#include "lib/mmem.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/logging.h"

#include "bundle.h"
#include "sdnv.h"
#include "statusreport.h"
#include "agent.h"
#include "hash.h"
#include "convergence_layer_dgram.h"

#include "storage.h"

/**
 * How long can a filename possibly be?
 */
#define STORAGE_FILE_NAME_LENGTH 	15

/**
 * Internal representation of a bundle
 *
 * The layout is quite fixed - the next pointer and the bundle_num have to go first because this struct
 * has to be compatible with the struct storage_entry_t in storage.h!
 */
struct file_list_entry_t {
	/** pointer to the next list element */
	struct file_list_entry_t * next;

	uint32_t bundle_num;

	/* local bundle creation time (in freeRTOS ticks) */
	TickType_t rec_time;
	uint32_t lifetime;
	uint32_t bundle_flags;

	// TODO use size_t
	uint16_t file_size;

	/** Flags */
	uint8_t flags;
};

/**
 * Flags for the storage
 */
#define STORAGE_COFFEE_FLAGS_LOCKED 	0x1

// List and memory blocks for the bundles
LIST(bundle_list);
MEMB(bundle_mem, struct file_list_entry_t, BUNDLE_STORAGE_SIZE);

// global, internal variables
/** Counts the number of bundles in storage */
static uint16_t bundles_in_storage;

/** Flag to indicate whether the bundle list has changed since last writing the list file */
static uint8_t bundle_list_changed = 0;

static SemaphoreHandle_t wait_for_changes_sem = NULL;
static SemaphoreHandle_t bundle_deleted_sem = NULL;

static FATFS fatfs;

///**
// * COFFEE is so slow, that we are loosing radio packets while using the flash. Unfortunately, the
// * radio is sending LL ACKs for these packets, so the other side does not know.
// * Therefore, we have to disable the radio while reading or writing COFFEE, to avoid sending
// * ACKs for packets that we cannot read out of the buffer.
// *
// * FIXME: This HACK is *very* ugly and poor design.
// */
//#define RADIO_SAFE_STATE_ON()		NETSTACK_MAC.off(0)
//#define RADIO_SAFE_STATE_OFF()		NETSTACK_MAC.on()

/**
 * "Internal" functions
 */
static void storage_fatfs_prune(const TimerHandle_t timer);
static uint8_t storage_fatfs_delete_bundle(uint32_t bundle_number, uint8_t reason);
static struct mmem * storage_fatfs_read_bundle(uint32_t bundle_number);

#if (BUNDLE_STORAGE_INIT == 0)
static void storage_fatfs_reconstruct_bundles();
#endif



static void storage_fatfs_file_close(FIL* const fd, const char* const filename)
{
	uint8_t tries = 5;

	FRESULT close_res = FR_OK;
	do {
		close_res = f_close(fd);
		if(close_res != FR_OK) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to close file %s (err %u)", filename, close_res);

			tries--;
			if (tries <= 0) {
				LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Closing file failed. Discard file contant and unlock file handle.");
				/* fp->lockid is invalid,
				 * if f_open failes before locking the file.
				 * So dec_lock will not decrement the file lock
				 * for a not locked file
				 */
				dec_lock(fd->lockid);
				break;
			}

			/* wait one ms to not blocking other tasks */
			vTaskDelay(pdMS_TO_TICKS(1));
		}
	} while (close_res != FR_OK);
}


static int storage_fatfs_format()
{
	//	RADIO_SAFE_STATE_ON();

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Formatting flash");

	const FRESULT res = f_mkfs("0:/", 0, 0);
	if (res != FR_OK) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Formatting failed with error code %u!", res);
		return -1;
	}

	//	RADIO_SAFE_STATE_OFF();

	return 0;
}


/**
 * \brief called by agent at startup
 */
static bool storage_fatfs_init(void)
{
	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "storage_fatfs init");

	/* Only execute the initalisation before the scheduler was started.
	 * So there exists only one thread and
	 * no locking is needed for the initialisation
	 */
	configASSERT(xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED);

	/* cancle, if initialisation was already done */
	if(wait_for_changes_sem != NULL || bundle_deleted_sem != NULL) {
		return false;
	}
	/* Do not use an recursive mutex,
	 * because mmem_check() will not work vital then.
	 */
	wait_for_changes_sem = xSemaphoreCreateCounting(1, 0);
	if(wait_for_changes_sem == NULL) {
		return false;
	}

	bundle_deleted_sem = xSemaphoreCreateCounting(1, 0);
	if(bundle_deleted_sem == NULL) {
		return false;
	}

	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	bundles_in_storage = 0;
	bundle_list_changed = 0;

	const FRESULT ret = f_mount(&fatfs, "0:/", 1);
	if (ret != FR_OK) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Mounting sd card failed with error code %u!", ret);
		return false;
	}

#if BUNDLE_STORAGE_INIT
	if (storage_fatfs_format() < 0) {
		return false;
	}
#else
	// Try to restore our bundle list from the file system
	storage_fatfs_reconstruct_bundles();
#endif

	// Set the timer to regularly prune expired bundles
	const TimerHandle_t store_timer = xTimerCreate("store timer", pdMS_TO_TICKS(5000), pdTRUE, NULL, storage_fatfs_prune);
	if (store_timer == NULL) {
		return false;
	}

	if ( !xTimerStart(store_timer, 0) ) {
		return false;
	}

	return true;
}

#if (BUNDLE_STORAGE_INIT == 0)
/**
 * \brief Restore bundles stored in CFS
 */
static void storage_fatfs_reconstruct_bundles()
{
	struct file_list_entry_t * entry = NULL;
	DIR directory_iterator;
	struct mmem * bundleptr = NULL;
	struct bundle_t * bundle = NULL;
	uint8_t found = 0;

//	RADIO_SAFE_STATE_ON();

	const FRESULT n = f_opendir(&directory_iterator, "/");
	if(n != FR_OK) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to list directory / (err %u)", n);
//		RADIO_SAFE_STATE_OFF();
		return;
	}

	FILINFO directory_entry;
#if _USE_LFN
	/* not using static buffer, becasue this is only needed on intialization */
	char lfn[_MAX_LFN + 1];
	directory_entry.lfname = lfn;
	directory_entry.lfsize = sizeof(lfn);
#endif

	while( f_readdir(&directory_iterator, &directory_entry) == FR_OK ) {
		if (directory_entry.fname[0] == 0) {
			/* end of directory */
			break;
		}

		if (directory_entry.fattrib & AM_DIR) {
			/* ignore directories */
			continue;
		}

		/* Check if there is a . in the filename */
		const char* const filename =  directory_entry.lfname[0] ? directory_entry.lfname : directory_entry.fname;

		if (f_size(&directory_entry) < sizeof(struct bundle_t)) {
			/* ignore files with a too small size */
			LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "filename %s is too small, skipping (size %lu)", filename, f_size(&directory_entry));
			continue;
		}

		char* const delimeter = strchr(filename, '.');
		if( delimeter == NULL ) {
			/* filename is invalid */
			LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "filename %s is invalid, skipping", filename);
			continue;
		}

		/* Check if the extension is b */
		if( *(delimeter+1) != 'b' ) {
			/* filename is invalid */
			LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "filename %s is invalid, skipping", filename);
			continue;
		}

		/* Get the bundle number from the filename */
		delimeter[0] = '\0';
		const uint32_t bundle_number = strtoul(filename, NULL, 10);

		/* Check if this bundle is in storage already */
		found = 0;
		for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {

			if( entry->bundle_num == bundle_number ) {
				found = 1;
				break;
			}
		}

		if( found == 1 ) {
			continue;
		}

		/* Allocate a directory entry for the bundle */
		entry = memb_alloc(&bundle_mem);
		if( entry == NULL ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot restore bundle (file %s, size %u)",
				filename, f_size(&directory_entry));
			continue;
		}

		/* Fill in the entry */
		entry->bundle_num = bundle_number;
		entry->file_size = f_size(&directory_entry);

		/* Add bundle to the list */
		list_add(bundle_list, entry);
		bundles_in_storage ++;

		/* Now read bundle from storage to update the rest of the entry */
//		RADIO_SAFE_STATE_OFF();
		bundleptr = storage_fatfs_read_bundle(entry->bundle_num);
//		RADIO_SAFE_STATE_ON();

		if( bundleptr == NULL ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to restore bundle %lu", entry->bundle_num);
			list_remove(bundle_list, entry);
			memb_free(&bundle_mem, entry);
			bundles_in_storage--;
			continue;
		}

		/* Get bundle struct */
		bundle = (struct bundle_t *) MMEM_PTR(bundleptr);

		/* Copy everything we need from the bundle */
		entry->rec_time = bundle->rec_time;
		entry->lifetime = bundle->lifetime;
		entry->file_size = bundleptr->size;
		entry->bundle_num = bundle->bundle_num;

		/* Deallocate memory */
		bundle_decrement(bundleptr);
		bundle = NULL;
		bundleptr = NULL;
		entry = NULL;
	}

	/* Close directory handle */
	f_closedir(&directory_iterator);

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Restored %u bundles from CFS", bundles_in_storage);

//	RADIO_SAFE_STATE_OFF();
}
#endif


/**
 * \brief deletes expired bundles from storage
 */
static void storage_fatfs_prune(const TimerHandle_t timer)
{
	struct file_list_entry_t * entry = NULL;

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		const TickType_t elapsed_time = (xTaskGetTickCount() - entry->rec_time) / portTICK_PERIOD_MS / 1000;

		if( entry->lifetime < elapsed_time ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "bundle lifetime expired of bundle %lu", entry->bundle_num);
			storage_fatfs_delete_bundle(entry->bundle_num, REASON_LIFETIME_EXPIRED);
		}
	}
}

/**
 * \brief Sets the storage to its initial state
 */
static void storage_fatfs_reinit(void)
{
	// Remove all bundles from storage
	while(bundles_in_storage > 0) {
		struct file_list_entry_t * entry = list_head(bundle_list);

		if( entry == NULL ) {
			// We do not have bundles in storage, stop deleting them
			break;
		}

		storage_fatfs_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}

	// And reset our counters
	bundles_in_storage = 0;
}

/**
 * \brief This function delete as many bundles from the storage as necessary to have at least one slot free
 * \param bundlemem Pointer to the MMEM struct containing the bundle
 * \return 1 on success, 0 if no room could be made free
 */
static uint8_t storage_fatfs_make_room(struct mmem * bundlemem)
{
	/* Delete expired bundles first */
	storage_fatfs_prune(NULL);

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
	struct file_list_entry_t * entry = NULL;

	/* Keep deleting bundles until we have enough slots */
	while( bundles_in_storage >= BUNDLE_STORAGE_SIZE) {
		unsigned long comparator = 0;
		struct file_list_entry_t * deletor = NULL;

		for( entry = list_head(bundle_list);
			 entry != NULL;
			 entry = list_item_next(entry) ) {

			/* Never delete locked bundles */
			if( entry->flags & STORAGE_COFFEE_FLAGS_LOCKED ) {
				continue;
			}

			/* Always keep bundles with higher priority */
			if( (entry->bundle_flags & BUNDLE_PRIORITY_MASK) > (bundle->flags & BUNDLE_PRIORITY_MASK ) ) {
				continue;
			}

#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDEST
			if( (clock_seconds() - entry->rec_time) > comparator || comparator == 0) {
				comparator = clock_seconds() - entry->rec_time;
				deletor = entry;
			}
#elif BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGEST
			if( (clock_seconds() - entry->rec_time) < comparator || comparator == 0) {
				comparator = clock_seconds() - entry->rec_time;
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
		storage_fatfs_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}
#elif (BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDER || BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGER )
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;

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
			if( entry->flags & STORAGE_COFFEE_FLAGS_LOCKED ) {
				continue;
			}

			/* Always keep bundles with higher priority */
			if( (entry->bundle_flags & BUNDLE_PRIORITY_MASK) > (bundle->flags & BUNDLE_PRIORITY_MASK ) ) {
				continue;
			}

			const uint32_t new_age = bundle->lifetime - ( (xTaskGetTickCount() - bundle->rec_time) / 1000 / portTICK_PERIOD_MS );
			const uint32_t old_age = entry->lifetime - ( (xTaskGetTickCount() - entry->rec_time) / 1000 / portTICK_PERIOD_MS );
#if BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_OLDER
			/* If the new bundle has a longer lifetime than the bundle in our storage,
			 * delete the bundle from storage to make room
			 */
			if (new_age >= old_age) {
				break;
			}
#elif BUNDLE_STORAGE_BEHAVIOUR == BUNDLE_STORAGE_BEHAVIOUR_DELETE_YOUNGER
			/* Delete youngest bundle in storage */
			// TODO possibly wrong comparsion
			if(new_age >= old_age) {
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
		storage_fatfs_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}
#else
#error No Bundle Deletion Strategy defined
#endif

	/* At least one slot is free now */
	return 1;
}

/**
 * \brief saves a bundle in storage
 * \param bundlemem pointer to the MMEM struct containing the bundle
 * \param bundle_number_ptr The pointer to the bundle number will be stored here
 * \return 1 on success, 0 otherwise
 */
static uint8_t storage_fatfs_save_bundle(struct mmem* const bundlemem, uint32_t* const bundle_number_ptr)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	char bundle_filename[STORAGE_FILE_NAME_LENGTH];
	int n;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "save_bundle with invalid pointer %p", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "save_bundle with invalid MMEM structure");
		bundle_decrement(bundlemem);
		return 0;
	}

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( bundle->bundle_num == entry->bundle_num ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "%lu is the same bundle", entry->bundle_num);
			*bundle_number_ptr = entry->bundle_num;
			bundle_decrement(bundlemem);
			return entry->bundle_num;
		}
	}

	while ( !storage_fatfs_make_room(bundlemem) ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Waiting for bundle deletion");
		if (xSemaphoreTake(bundle_deleted_sem, pdMS_TO_TICKS(CONVERGENCE_LAYER_RETRANSMIT_TIMEOUT)) == pdFALSE) {
			/* Timeout has expired.
			 * Do not acceped the bundle and
			 * send an temporare NACK
			 */

			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "Cannot store bundle, no room");

			/* Throw away bundle to not take up RAM */
			bundle_decrement(bundlemem);

			return 0;
		}
	}

	/* Update the pointer to the bundle, address may have changed due to storage_fatfs_make_room and MMEM reallocations */
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	// Allocate some memory for our bundle
	entry = memb_alloc(&bundle_mem);
	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot store bundle");
		bundle_decrement(bundlemem);
		return 0;
	}

	// Clear the memory area
	memset(entry, 0, sizeof(struct file_list_entry_t));

	// Copy necessary values from the bundle
	entry->rec_time = bundle->rec_time;
	entry->lifetime = bundle->lifetime;
	entry->file_size = bundlemem->size;
	entry->bundle_flags = bundle->flags;

	// Assign a unique bundle number
	entry->bundle_num = bundle->bundle_num;

	// determine the filename
	n = snprintf(bundle_filename, STORAGE_FILE_NAME_LENGTH, "%lu.b", entry->bundle_num);
	if( n == STORAGE_FILE_NAME_LENGTH ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "file name buffer is too short");
		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
		return 0;
	}

//	RADIO_SAFE_STATE_ON();

	// Store the bundle into the file
//	n = cfs_coffee_reserve(bundle_filename, bundlemem->size);
//	if( n < 0 ) {
//		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to reserve %u bytes for bundle", bundlemem->size);
//		memb_free(&bundle_mem, entry);
//		bundle_decrement(bundlemem);
//		RADIO_SAFE_STATE_OFF();
//		return 0;
//	}

	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "open file %s", bundle_filename);
	// Open the output file
	FIL fd_write;
	const FRESULT res = f_open(&fd_write, bundle_filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		// Unable to open file, abort here
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to open file %s, cannot save bundle (err %u)", bundle_filename, res);
		storage_fatfs_file_close(&fd_write, bundle_filename);
		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
//		RADIO_SAFE_STATE_OFF();
		return 0;
	}

	// Write our complete bundle
	UINT bytes_written = 0;
	const FRESULT ret = f_write(&fd_write, bundle, bundlemem->size, &bytes_written);
	if(ret != FR_OK || bytes_written != bundlemem->size) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to write %u bytes to file %s, aborting (err %u, written %u)",
			bundlemem->size, bundle_filename, res, bytes_written);
		storage_fatfs_file_close(&fd_write, bundle_filename);
		f_unlink(bundle_filename);

		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
//		RADIO_SAFE_STATE_OFF();
		return 0;
	}

	// And close the file
	storage_fatfs_file_close(&fd_write, bundle_filename);
	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "file %s closed", bundle_filename);

//	RADIO_SAFE_STATE_OFF();

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "New Bundle %lu (%lu), Src %lu.%lu, Dest %lu.%lu, Seq %lu",
		bundle->bundle_num, entry->bundle_num, bundle->src_node, ((uint32_t)bundle->src_srv),
		bundle->dst_node, ((uint32_t)bundle->dst_srv), bundle->tstamp_seq);

	// Add bundle to the list
	list_add(bundle_list, entry);

	// Mark the bundle list as changed
	bundle_list_changed = 1;
	bundles_in_storage++;

	// Now we have to free the incoming bundle slot
	bundle_decrement(bundlemem);

	// Now copy over the STATIC pointer to the bundle number, so that
	// the caller can stick it into an event
	*bundle_number_ptr = entry->bundle_num;

	/* storage status has changed */
	xSemaphoreGive(wait_for_changes_sem);

	return 1;
}

/**
 * \brief deletes a bundle form storage
 * \param bundle_number bundle number to be deleted
 * \param reason reason code
 * \return 1 on success or 0 on error
 */
static uint8_t storage_fatfs_delete_bundle(uint32_t bundle_number, uint8_t reason)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	char bundle_filename[STORAGE_FILE_NAME_LENGTH];
	int n;

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "Deleting Bundle %lu with reason %u", bundle_number, reason);

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( entry->bundle_num == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Could not find bundle %lu on del_bundle", bundle_number);
		return 0;
	}

	// Figure out the source to send status report
	if( reason != REASON_DELIVERED ) {
		if( (entry->bundle_flags & BUNDLE_FLAG_CUST_REQ ) || (entry->bundle_flags & BUNDLE_FLAG_REP_DELETE) ){
			bundlemem = storage_fatfs_read_bundle(bundle_number);
			if( bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to read back bundle %lu", bundle_number);
				return 0;
			}

			bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
			bundle->del_reason = reason;

			if (bundle->src_node != dtn_node_id){
				STATUSREPORT.send(bundlemem, 16, bundle->del_reason);
			}
		}

		bundle_decrement(bundlemem);
		bundle = NULL;
	}

	// Notified the agent, that a bundle has been deleted
	agent_delete_bundle(bundle_number);

	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	// determine the filename and remove the file
	n = snprintf(bundle_filename, STORAGE_FILE_NAME_LENGTH, "%lu.b", entry->bundle_num);
	if( n == STORAGE_FILE_NAME_LENGTH ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "file name buffer is too short");
		return 0;
	}

//	RADIO_SAFE_STATE_ON();

	f_unlink(bundle_filename);

//	RADIO_SAFE_STATE_OFF();

	// Mark the bundle list as changed
	bundle_list_changed = 1;
	bundles_in_storage--;

	// Free the storage struct
	memb_free(&bundle_mem, entry);

	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Bundle %lu deleted with reason %u", bundle_number, reason);

	/* there is one more slot free, now */
	xSemaphoreGive(bundle_deleted_sem);

	/* storage status has changed */
	xSemaphoreGive(wait_for_changes_sem);

	return 1;
}

/**
 * \brief reads a bundle from storage
 * \param bundle_number bundle number to read
 * \return pointer to the MMEM struct containing the bundle (caller has to free)
 */
static struct mmem * storage_fatfs_read_bundle(uint32_t bundle_number)
{
	struct file_list_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	char bundle_filename[STORAGE_FILE_NAME_LENGTH];
	int n;

	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "Reading Bundle %lu", bundle_number);

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( entry->bundle_num == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Could not find bundle %lu on read_bundle", bundle_number);
		return NULL;
	}

	bundlemem = bundle_create_bundle();
	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "cannot allocate memory for bundle %lu", bundle_number);
		return NULL;
	}

	n = mmem_realloc(bundlemem, entry->file_size);
	if( !n ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to realloc enough memory");
		bundle_decrement(bundlemem);
		return NULL;
	}

	// determine the filename
	n = snprintf(bundle_filename, STORAGE_FILE_NAME_LENGTH, "%lu.b", entry->bundle_num);
	if( n == STORAGE_FILE_NAME_LENGTH ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "file name buffer is too short");
		bundle_decrement(bundlemem);
		return NULL;
	}

//	RADIO_SAFE_STATE_ON();

	// Open the output file
	FIL fd_read;
	const FRESULT open_res = f_open(&fd_read, bundle_filename, FA_OPEN_EXISTING | FA_READ);
	if(open_res != FR_OK) {
		// Unable to open file, abort here
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to open file %s, cannot read bundle (err %u)", bundle_filename, open_res);
		bundle_decrement(bundlemem);
		storage_fatfs_file_close(&fd_read, bundle_filename);
		return NULL;
	}

	// Read our complete bundle
	UINT bytes_read = 0;
	const FRESULT ret = f_read(&fd_read, MMEM_PTR(bundlemem), bundlemem->size, &bytes_read);
	if(ret != FR_OK || bytes_read != bundlemem->size) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to read %u bytes from file %s, aborting", bundlemem->size, bundle_filename);
		bundle_decrement(bundlemem);
		storage_fatfs_file_close(&fd_read, bundle_filename);
		return NULL;
	}

	// And close the file
	storage_fatfs_file_close(&fd_read, bundle_filename);

//	RADIO_SAFE_STATE_OFF();

	return bundlemem;
}

/**
 * \brief checks if there is space for a bundle
 * \param bundlemem pointer to a bundle struct (not used here)
 * \return number of free slots
 */
static uint16_t storage_fatfs_get_free_space(struct mmem * bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
 * \brief Get the number of slots available in storage
 * \returns the number of free slots
 */
static uint16_t storage_fatfs_get_bundle_numbers(void){
	return bundles_in_storage;
}

/**
 * \brief Get the bundle list
 * \returns pointer to first bundle list entry
 */
static struct storage_entry_t * storage_fatfs_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
}

/**
 * \brief Mark a bundle as locked so that it will not be deleted even if we are running out of space
 *
 * \param bundle_num Bundle number
 * \return 1 on success or 0 on error
 */
static uint8_t storage_fatfs_lock_bundle(uint32_t bundle_num)
{
	struct file_list_entry_t * entry = NULL;

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

	entry->flags |= STORAGE_COFFEE_FLAGS_LOCKED;

	return 1;
}

/**
 * \brief Mark a bundle as unlocked after being locked previously
 */
static void storage_fatfs_unlock_bundle(uint32_t bundle_num)
{
	struct file_list_entry_t * entry = NULL;

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

	entry->flags &= ~STORAGE_COFFEE_FLAGS_LOCKED;
}


static void storage_fatfs_wait_for_changes(void)
{
	if ( !xSemaphoreTake(wait_for_changes_sem, portMAX_DELAY) ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_WRN, "Wait for changes failed");
	}
}


const struct storage_driver storage_fatfs = {
	"STORAGE_FATFS",
	storage_fatfs_init,
	storage_fatfs_reinit,
	storage_fatfs_save_bundle,
	storage_fatfs_delete_bundle,
	storage_fatfs_read_bundle,
	storage_fatfs_lock_bundle,
	storage_fatfs_unlock_bundle,
	storage_fatfs_get_free_space,
	storage_fatfs_get_bundle_numbers,
	storage_fatfs_get_bundles,
	storage_fatfs_wait_for_changes,
	storage_fatfs_format,
};
/** @} */
/** @} */

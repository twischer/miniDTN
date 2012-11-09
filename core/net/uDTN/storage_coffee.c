/**
 * \addtogroup bstorage
 * @{
 */

 /**
 * \defgroup g_storage flash storage modules
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
#include "mac.h"
#include "netstack.h"
#include "cfs/cfs.h"
#include "mmem.h"
#include "memb.h"
#include "cfs-coffee.h"
#include "watchdog.h"
#include "list.h"
#include "logging.h"

#include "bundle.h"
#include "sdnv.h"
#include "statusreport.h"
#include "agent.h"
#include "hash.h"

#include "storage.h"

/**
 * How long can a filename possibly be?
 */
#define STORAGE_FILE_NAME_LENGTH 	15

/**
 * How often should we save a bundle overview file to flash? [in seconds]
 */
#define STORAGE_WRITE_INTERVAL		120

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

	uint32_t rec_time;
	uint32_t lifetime;

	uint16_t file_size;
};

// List and memory blocks for the bundles
LIST(bundle_list);
MEMB(bundle_mem, struct file_list_entry_t, BUNDLE_STORAGE_SIZE);

// global, internal variables
/** Counts the number of bundles in storage */
static uint16_t bundles_in_storage;

/** Is used to periodically traverse all bundles and delete those that are expired */
static struct ctimer g_store_timer;

/** Is used to periodically write the list of bundles to flash */
static struct ctimer g_store_write_timer;

/** Flag to indicate whether the bundle list has changed since last writing the list file */
static uint8_t bundle_list_changed = 0;

/**
 * COFFEE is so slow, that we are loosing radio packets while using the flash. Unfortunately, the
 * radio is sending LL ACKs for these packets, so the other side does not know.
 * Therefore, we have to disable the radio while reading or writing COFFEE, to avoid sending
 * ACKs for packets that we cannot read out of the buffer.
 *
 * FIXME: This HACK is *very* ugly and poor design.
 */
#define RADIO_SAFE_STATE_ON()		NETSTACK_MAC.off(0)
#define RADIO_SAFE_STATE_OFF()		NETSTACK_MAC.on()

/**
 * "Internal" functions
 */
void storage_coffee_save_list_on_change();
void storage_coffee_store_prune();
uint16_t storage_coffee_delete_bundle(uint32_t bundle_number, uint8_t reason);
struct mmem * storage_coffee_read_bundle(uint32_t bundle_number);
void storage_coffee_read_list();
void storage_coffee_save_list();

/**
* /brief called by agent at startup
*/
void storage_mmem_init(void)
{
	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	bundles_in_storage = 0;
	bundle_list_changed = 0;

	// Try to restore our bundle list from the file system
	storage_coffee_read_list();

	// Set the timer to regularly prune expired bundles
	ctimer_set(&g_store_timer, CLOCK_SECOND*5, storage_coffee_store_prune, NULL);

	// Set the timer to regularly write the list of bundles to flash
	ctimer_set(&g_store_write_timer, CLOCK_SECOND * STORAGE_WRITE_INTERVAL, storage_coffee_save_list_on_change, NULL);
}


void storage_coffee_save_list_on_change()
{
	LOG(LOGD_DTN, LOG_STORE, LOGL_DBG, "g_store_save_list_on_change()");

	// In case the bundle list has changed, write the changes to the bundle list file
	if( bundle_list_changed ) {
		RADIO_SAFE_STATE_ON();

		// Remove the old list file
		cfs_remove(BUNDLE_STORAGE_FILE_NAME);

		RADIO_SAFE_STATE_OFF();

		// And create a new one
		storage_coffee_save_list();

		bundle_list_changed = 0;
	}

	// Restart the timer
	ctimer_restart(&g_store_write_timer);
}

/**
 * \brief Store the current bundle list into a file
 * FIXME: This function is actually not necessary, we could also list the directory on startup and restore bundles that way
 */
void storage_coffee_save_list()
{
	int fd_write;
	int n;
	struct file_list_entry_t * entry = NULL;

	RADIO_SAFE_STATE_ON();

	// Reserve memory for our list
	cfs_coffee_reserve(BUNDLE_STORAGE_FILE_NAME, sizeof(struct file_list_entry_t) * bundles_in_storage);

	// Open the file for writing
	fd_write = cfs_open(BUNDLE_STORAGE_FILE_NAME, CFS_WRITE);
	if( fd_write == -1 ) {
		// Unable to open file, abort here
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to open file %s, cannot save bundle list", BUNDLE_STORAGE_FILE_NAME);
		return;
	}

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		n = cfs_write(fd_write, entry, sizeof(struct file_list_entry_t));

		if( n != sizeof(struct file_list_entry_t) ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to append %u bytes to bundle list file, aborting", sizeof(struct file_list_entry_t));
			cfs_close(fd_write);
			cfs_remove(BUNDLE_STORAGE_FILE_NAME);
			return;
		}
	}

	cfs_close(fd_write);

	RADIO_SAFE_STATE_OFF();
}

/**
 * Restore the bundle list from a file
 */
void storage_coffee_read_list()
{
	int fd_read;
	int n;
	struct file_list_entry_t * entry = NULL;
	int eof = 0;

	RADIO_SAFE_STATE_ON();

	// Open the file for reading
	fd_read = cfs_open(BUNDLE_STORAGE_FILE_NAME, CFS_READ);
	if( fd_read == -1 ) {
		// Unable to open file, abort here
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to open file %s, cannot read bundle list", BUNDLE_STORAGE_FILE_NAME);
		RADIO_SAFE_STATE_OFF();
		return;
	}

	while( !eof ) {
		entry = memb_alloc(&bundle_mem);
		if( entry == NULL ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to allocate struct, cannot restore bundle list");
			cfs_close(fd_read);
			RADIO_SAFE_STATE_OFF();
			return;
		}

		// Read the struct
		n = cfs_read(fd_read, entry, sizeof(struct file_list_entry_t));

		if( n == 0 ) {
			// End of file reached
			memb_free(&bundle_mem, entry);
			RADIO_SAFE_STATE_OFF();
			break;
		}

		if( n != sizeof(struct file_list_entry_t) ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to read %u bytes from bundle list file", sizeof(struct file_list_entry_t));
			cfs_close(fd_read);
			memb_free(&bundle_mem, entry);
			RADIO_SAFE_STATE_OFF();
			return;
		}

		// Add bundle to the list
		list_add(bundle_list, entry);

		bundles_in_storage ++;

		// Now that we have the index entry, we have to try to read the bundle
		struct mmem * testbundle = NULL;
		testbundle = storage_coffee_read_bundle(entry->bundle_num);
		if( testbundle == NULL ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to restore bundle %lu", entry->bundle_num);
			list_remove(bundle_list, entry);
			memb_free(&bundle_mem, entry);
			continue;
		}

		bundle_decrement(testbundle);
	}

	cfs_close(fd_read);

	RADIO_SAFE_STATE_OFF();
}

/**
* \brief deletes expired bundles from storage
*/
void storage_coffee_store_prune()
{
	uint32_t elapsed_time;
	struct file_list_entry_t * entry = NULL;

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		elapsed_time = clock_seconds() - entry->rec_time;

		if( entry->lifetime < elapsed_time ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "bundle lifetime expired of bundle %lu", entry->bundle_num);
			storage_coffee_delete_bundle(entry->bundle_num, REASON_LIFETIME_EXPIRED);
		}
	}

	// Restart the timer
	ctimer_restart(&g_store_timer);
}

/**
 * \brief Sets the storage to its initial state
 */
void storage_mmem_reinit(void)
{
	// Remove all bundles from storage
	while(bundles_in_storage > 0) {
		struct file_list_entry_t * entry = list_head(bundle_list);

		if( entry == NULL ) {
			// We do not have bundles in storage, stop deleting them
			break;
		}

		storage_coffee_delete_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}

	RADIO_SAFE_STATE_ON();

	// Delete the bundle list file
	cfs_remove(BUNDLE_STORAGE_FILE_NAME);

	RADIO_SAFE_STATE_OFF();

	// And reset our counters
	bundles_in_storage = 0;
}

/**
* \brief saves a bundle in storage
* \param bundle pointer to the bundle
* \return the bundle number given to the bundle or <0 on errors
*/
uint8_t storage_coffee_save_bundle(struct mmem * bundlemem, uint32_t ** bundle_number_ptr)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	char bundle_filename[STORAGE_FILE_NAME_LENGTH];
	int fd_write;
	int n;
	uint32_t bundle_number;

	if( bundlemem == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "save_bundle with invalid pointer %p", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "save_bundle with invalid MMEM structure");
		return 0;
	}

	// Calculate the bundle number
	bundle_number = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs);

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( bundle_number == entry->bundle_num ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "%lu is the same bundle", entry->bundle_num);
			bundle_decrement(bundlemem);
			return entry->bundle_num;
		}
	}

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

	// Assign a unique bundle number
	bundle->bundle_num = bundle_number;
	entry->bundle_num = bundle_number;

	// determine the filename
	n = snprintf(bundle_filename, STORAGE_FILE_NAME_LENGTH, "%lu.b", entry->bundle_num);
	if( n == STORAGE_FILE_NAME_LENGTH ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "file name buffer is too short");
		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
		return 0;
	}

	RADIO_SAFE_STATE_ON();

	// Store the bundle into the file
	n = cfs_coffee_reserve(bundle_filename, bundlemem->size);
	if( n < 0 ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to reserve %u bytes for bundle", bundlemem->size);
		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
		RADIO_SAFE_STATE_OFF();
		return 0;
	}

	// Open the output file
	fd_write = cfs_open(bundle_filename, CFS_WRITE);
	if( fd_write == -1 ) {
		// Unable to open file, abort here
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to open file %s, cannot save bundle", bundle_filename);
		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
		RADIO_SAFE_STATE_OFF();
		return 0;
	}

	// Write our complete bundle
	n = cfs_write(fd_write, bundle, bundlemem->size);
	if( n != bundlemem->size ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to write %u bytes to file %s, aborting", bundlemem->size, bundle_filename);
		cfs_close(fd_write);
		cfs_remove(bundle_filename);
		memb_free(&bundle_mem, entry);
		bundle_decrement(bundlemem);
		RADIO_SAFE_STATE_OFF();
		return 0;
	}

	// And close the file
	cfs_close(fd_write);

	RADIO_SAFE_STATE_OFF();

	LOG(LOGD_DTN, LOG_STORE, LOGL_INF, "New Bundle %lu (%lu), Src %lu.%lu, Dest %lu.%lu, Seq %lu", bundle->bundle_num, entry->bundle_num, bundle->src_node, bundle->src_srv, bundle->dst_node, bundle->dst_srv, bundle->tstamp_seq);

	// Add bundle to the list
	list_add(bundle_list, entry);

	// Mark the bundle list as changed
	bundle_list_changed = 1;
	bundles_in_storage++;

	// Now we have to free the incoming bundle slot
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
uint16_t storage_coffee_delete_bundle(uint32_t bundle_number, uint8_t reason)
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
		// REASON_DELIVERED means "bundle delivered" and does not need a report
		// FIXME: really?

		bundlemem = storage_coffee_read_bundle(bundle_number);
		if( bundlemem == NULL ) {
			LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to read back bundle %lu", bundle_number);
			return 0;
		}

		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
		bundle->del_reason = reason;

		if( (bundle->flags & 8 ) || (bundle->flags & 0x40000) ){
			if (bundle->src_node != dtn_node_id){
				STATUS_REPORT.send(bundle, 16, bundle->del_reason);
			}
		}

		bundle_decrement(bundlemem);
	}

	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	// determine the filename and remove the file
	n = snprintf(bundle_filename, STORAGE_FILE_NAME_LENGTH, "%lu.b", entry->bundle_num);
	if( n == STORAGE_FILE_NAME_LENGTH ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "file name buffer is too short");
		return 0;
	}

	RADIO_SAFE_STATE_ON();

	cfs_remove(bundle_filename);

	RADIO_SAFE_STATE_OFF();

	// Mark the bundle list as changed
	bundle_list_changed = 1;
	bundles_in_storage--;

	// Notified the agent, that a bundle has been deleted
	agent_delete_bundle(bundle_number);

	// Free the storage struct
	memb_free(&bundle_mem, entry);

	return 1;
}

/**
* \brief reads a bundle from storage
* \param bundle_num bundle number to read
* \return 1 on success or 0 on error
*/
struct mmem * storage_coffee_read_bundle(uint32_t bundle_number)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	char bundle_filename[STORAGE_FILE_NAME_LENGTH];
	int fd_read;
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
		return 0;
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

	// Assign a unique bundle number
	bundle->bundle_num = bundle_number++;
	entry->bundle_num = bundle->bundle_num;

	// determine the filename
	n = snprintf(bundle_filename, STORAGE_FILE_NAME_LENGTH, "%lu.b", entry->bundle_num);
	if( n == STORAGE_FILE_NAME_LENGTH ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "file name buffer is too short");
		bundle_decrement(bundlemem);
		return NULL;
	}

	RADIO_SAFE_STATE_ON();

	// Open the output file
	fd_read = cfs_open(bundle_filename, CFS_READ);
	if( fd_read == -1 ) {
		// Unable to open file, abort here
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to open file %s, cannot read bundle", bundle_filename);
		bundle_decrement(bundlemem);
		return NULL;
	}

	// Read our complete bundle
	n = cfs_read(fd_read, MMEM_PTR(bundlemem), bundlemem->size);
	if( n != bundlemem->size ) {
		LOG(LOGD_DTN, LOG_STORE, LOGL_ERR, "unable to read %u bytes from file %s, aborting", bundlemem->size, bundle_filename);
		bundle_decrement(bundlemem);
		cfs_close(fd_read);
		return NULL;
	}

	// And close the file
	cfs_close(fd_read);

	RADIO_SAFE_STATE_OFF();

	return bundlemem;
}

/**
* \brief checks if there is space for a bundle
* \param bundle pointer to a bundle struct (not used here)
* \return number of free slots
*/
uint16_t storage_coffee_get_free_space(struct mmem * bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
* \returns the number of saved bundles
*/
uint16_t storage_coffee_get_bundle_numbers(void){
	return bundles_in_storage;
}

/**
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * storage_coffee_get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
}

const struct storage_driver storage_coffee = {
	"G_STORAGE",
	storage_mmem_init,
	storage_mmem_reinit,
	storage_coffee_save_bundle,
	storage_coffee_delete_bundle,
	storage_coffee_read_bundle,
	storage_coffee_get_free_space,
	storage_coffee_get_bundle_numbers,
	storage_coffee_get_bundles,
};
/** @} */
/** @} */

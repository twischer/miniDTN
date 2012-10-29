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
#include "contiki.h"
#include "storage.h"
#include "cfs/cfs.h"
#include "bundle.h"
#include "sdnv.h"
#include "dtn_config.h"
#include "status-report.h"
#include "agent.h"
#include "mmem.h"
#include "memb.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cfs-coffee.h"
#include "hash.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

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

/** Flag to indicate whether the bundle list has changed since last writing the list file */
static uint8_t bundle_list_changed = 0;

/**
 * "Internal" functions
 */
void g_store_prune();
uint16_t del_bundle(uint32_t bundle_number, uint8_t reason);
struct mmem * read_bundle(uint32_t bundle_number);
void g_store_read_list();

/**
* /brief called by agent at startup
*/
void init(void)
{
	// Initialize the bundle list
	list_init(bundle_list);

	// Initialize the bundle memory block
	memb_init(&bundle_mem);

	bundles_in_storage = 0;

	// Try to restore our bundle list from the file system
	g_store_read_list();

	// Set the timer to regularly prune expired bundles
	ctimer_set(&g_store_timer, CLOCK_SECOND*5, g_store_prune, NULL);
}

/**
 * \brief Store the current bundle list into a file
 */
void g_store_save_list()
{
	int fd_write;
	int n;
	struct file_list_entry_t * entry = NULL;

	// Reserve memory for our list
	cfs_coffee_reserve(BUNDLE_STORAGE_FILE_NAME, sizeof(struct file_list_entry_t) * bundles_in_storage);

	// Open the file for writing
	fd_write = cfs_open(BUNDLE_STORAGE_FILE_NAME, CFS_WRITE);
	if( fd_write == -1 ) {
		// Unable to open file, abort here
		printf("STORAGE: unable to open file %s, cannot save bundle list\n", BUNDLE_STORAGE_FILE_NAME);
		return;
	}

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		n = cfs_write(fd_write, entry, sizeof(struct file_list_entry_t));

		if( n != sizeof(struct file_list_entry_t) ) {
			printf("STORAGE: unable to append %u bytes to bundle list file, aborting\n", sizeof(struct file_list_entry_t));
			cfs_close(fd_write);
			cfs_remove(BUNDLE_STORAGE_FILE_NAME);
			return;
		}
	}

	cfs_close(fd_write);
}

/**
 * Restore the bundle list from a file
 */
void g_store_read_list()
{
	int fd_read;
	int n;
	struct file_list_entry_t * entry = NULL;
	int eof = 0;

	// Open the file for reading
	fd_read = cfs_open(BUNDLE_STORAGE_FILE_NAME, CFS_READ);
	if( fd_read == -1 ) {
		// Unable to open file, abort here
		PRINTF("STORAGE: unable to open file %s, cannot read bundle list\n", BUNDLE_STORAGE_FILE_NAME);
		return;
	}

	while( !eof ) {
		entry = memb_alloc(&bundle_mem);
		if( entry == NULL ) {
			printf("STORAGE: unable to allocate struct, cannot restore bundle list\n");
			cfs_close(fd_read);
			return;
		}

		// Read the struct
		n = cfs_read(fd_read, entry, sizeof(struct file_list_entry_t));

		if( n == 0 ) {
			// End of file reached
			memb_free(&bundle_mem, entry);
			break;
		}

		if( n != sizeof(struct file_list_entry_t) ) {
			printf("STORAGE: unable to read %u bytes from bundle list file\n", sizeof(struct file_list_entry_t));
			cfs_close(fd_read);
			memb_free(&bundle_mem, entry);
			return;
		}

		// Add bundle to the list
		list_add(bundle_list, entry);

		bundles_in_storage ++;
	}

	cfs_close(fd_read);
}

/**
* \brief deletes expired bundles from storage
*/
void g_store_prune()
{
	uint32_t elapsed_time;
	struct file_list_entry_t * entry = NULL;

	// Delete expired bundles from storage
	for(entry = list_head(bundle_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		elapsed_time = clock_seconds() - entry->rec_time;

		if( entry->lifetime < elapsed_time ) {
			PRINTF("STORAGE: bundle lifetime expired of bundle %lu\n", entry->bundle_num);
			del_bundle(entry->bundle_num, REASON_LIFETIME_EXPIRED);
		}
	}

	// In case the bundle list has changed, write the changes to the bundle list file
	if( bundle_list_changed ) {
		// Remove the old list file
		cfs_remove(BUNDLE_STORAGE_FILE_NAME);

		// And create a new one
		g_store_save_list();

		bundle_list_changed = 0;
	}

	// Restart the timer
	ctimer_restart(&g_store_timer);
}

/**
 * \brief Sets the storage to its initial state
 */
void reinit(void)
{
	// Remove all bundles from storage
	while(bundles_in_storage > 0) {
		struct file_list_entry_t * entry = list_head(bundle_list);

		if( entry == NULL ) {
			// We do not have bundles in storage, stop deleting them
			break;
		}

		del_bundle(entry->bundle_num, REASON_DEPLETED_STORAGE);
	}
	
	// Delete the bundle list file
	cfs_remove(BUNDLE_STORAGE_FILE_NAME);

	// And reset our counters
	bundles_in_storage = 0;
}

/**
* \brief saves a bundle in storage
* \param bundle pointer to the bundle
* \return the bundle number given to the bundle or <0 on errors
*/
uint8_t save_bundle(struct mmem * bundlemem, uint32_t * bundle_number)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	char bundle_filename[12];
	int fd_write;
	int n;

	if( bundlemem == NULL ) {
		printf("STORAGE: save_bundle with invalid pointer %p\n", bundlemem);
		return 0;
	}

	// Get the pointer to our bundle
	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	if( bundle == NULL ) {
		printf("STORAGE: save_bundle with invalid MMEM structure\n");
		return 0;
	}

	// Calculate the bundle number
	*bundle_number = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs);

	// Look for duplicates in the storage
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( *bundle_number == entry->bundle_num ) {
			PRINTF("STORAGE: %lu is the same bundle\n", entry->bundle_num);
			return entry->bundle_num;
		}
	}

	// Allocate some memory for our bundle
	entry = memb_alloc(&bundle_mem);
	if( entry == NULL ) {
		printf("STORAGE: unable to allocate struct, cannot store bundle\n");
		return 0;
	}

	// Clear the memory area
	memset(entry, 0, sizeof(struct file_list_entry_t));

	// Copy necessary values from the bundle
	entry->rec_time = bundle->rec_time;
	entry->lifetime = bundle->lifetime;
	entry->file_size = bundlemem->size;

	// Assign a unique bundle number
	bundle->bundle_num = *bundle_number;
	entry->bundle_num = bundle->bundle_num;

	// determine the filename
	sprintf(bundle_filename, "%lu.b", entry->bundle_num);

	// Store the bundle into the file
	cfs_coffee_reserve(bundle_filename, bundlemem->size);

	// Open the output file
	fd_write = cfs_open(bundle_filename, CFS_WRITE);
	if( fd_write == -1 ) {
		// Unable to open file, abort here
		printf("STORAGE: unable to open file %s, cannot save bundle\n", bundle_filename);
		memb_free(&bundle_mem, entry);
		bundle_dec(bundlemem);
		return 0;
	}

	// Write our complete bundle
	n = cfs_write(fd_write, bundle, bundlemem->size);
	if( n != bundlemem->size ) {
		printf("STORAGE: unable to write %u bytes to file %s, aborting\n", bundlemem->size, bundle_filename);
		cfs_close(fd_write);
		cfs_remove(bundle_filename);
		memb_free(&bundle_mem, entry);
		bundle_dec(bundlemem);
		return 0;
	}

	// And close the file
	cfs_close(fd_write);

	PRINTF("STORAGE: New Bundle %lu (%lu), Src %lu, Dest %lu, Seq %lu\n", bundle->bundle_num, entry->bundle_num, bundle->src_node, bundle->dst_node, bundle->tstamp_seq);

	// Add bundle to the list
	list_add(bundle_list, entry);

	// Mark the bundle list as changed
	bundle_list_changed = 1;
	bundles_in_storage++;

	// Now we have to free the incoming bundle slot
	bundle_dec(bundlemem);

	// Now we have to send an event to our daemon
	process_post(&agent_process, dtn_bundle_in_storage_event, &entry->bundle_num);

	return 1;
}

/**
* \brief deletes a bundle form storage
* \param bundle_num bundle number to be deleted
* \param reason reason code
* \return 1 on success or 0 on error
*/
uint16_t del_bundle(uint32_t bundle_number, uint8_t reason)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	char bundle_filename[12];

	PRINTF("STORAGE: Deleting Bundle %lu with reason %u\n", bundle_number, reason);

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( entry->bundle_num == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		printf("STORAGE: Could not find bundle %lu on del_bundle\n", bundle_number);
		return 0;
	}

	// Figure out the source to send status report
	if( reason != REASON_DELIVERED ) {
		// REASON_DELIVERED means "bundle delivered" and does not need a report
		// FIXME: really?

		bundlemem = read_bundle(bundle_number);
		if( bundlemem == NULL ) {
			printf("STORAGE: unable to read back bundle %lu\n", bundle_number);
			return 0;
		}

		bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
		bundle->del_reason = reason;

		if( (bundle->flags & 8 ) || (bundle->flags & 0x40000) ){
			if (bundle->src_node != dtn_node_id){
				STATUS_REPORT.send(bundle, 16, bundle->del_reason);
			}
		}

		bundle_dec(bundlemem);
	}

	// Remove the bundle from the list
	list_remove(bundle_list, entry);

	// determine the filename and remove the file
	sprintf(bundle_filename, "%lu.b", entry->bundle_num);
	cfs_remove(bundle_filename);

	// Mark the bundle list as changed
	bundle_list_changed = 1;
	bundles_in_storage--;

	// Notified the agent, that a bundle has been deleted
	agent_del_bundle(bundle_number);

	// Free the storage struct
	memb_free(&bundle_mem, entry);

	return 1;
}

/**
* \brief reads a bundle from storage
* \param bundle_num bundle number to read
* \return 1 on success or 0 on error
*/
struct mmem * read_bundle(uint32_t bundle_number)
{
	struct bundle_t * bundle = NULL;
	struct file_list_entry_t * entry = NULL;
	struct mmem * bundlemem = NULL;
	char bundle_filename[12];
	int fd_read;
	int n;

	PRINTF("STORAGE: Reading Bundle %lu\n", bundle_number);

	// Look for the bundle we are talking about
	for(entry = list_head(bundle_list);
		entry != NULL;
		entry = list_item_next(entry)) {

		if( entry->bundle_num == bundle_number ) {
			break;
		}
	}

	if( entry == NULL ) {
		PRINTF("STORAGE: Could not find bundle %lu on read_bundle\n", bundle_number);
		return 0;
	}

	bundlemem = create_bundle();
	if( bundlemem == NULL ) {
		printf("STORAGE: cannot allocate memory for bundle %lu\n", bundle_number);
		return NULL;
	}

	/* FIXME: Error case */
	mmem_realloc(bundlemem, entry->file_size);

	// Assign a unique bundle number
	bundle->bundle_num = bundle_number++;
	entry->bundle_num = bundle->bundle_num;

	// determine the filename
	sprintf(bundle_filename, "%lu.b", entry->bundle_num);

	// Store the bundle into the file
	cfs_coffee_reserve(bundle_filename, bundlemem->size);

	// Open the output file
	fd_read = cfs_open(bundle_filename, CFS_READ);
	if( fd_read == -1 ) {
		// Unable to open file, abort here
		printf("STORAGE: unable to open file %s, cannot read bundle\n", bundle_filename);
		bundle_dec(bundlemem);
		return NULL;
	}

	// Read our complete bundle
	n = cfs_read(fd_read, MMEM_PTR(bundlemem), bundlemem->size);
	if( n != bundlemem->size ) {
		printf("STORAGE: unable to read %u bytes from file %s, aborting\n", bundlemem->size, bundle_filename);
		bundle_dec(bundlemem);
		cfs_close(fd_read);
		return NULL;
	}

	// And close the file
	cfs_close(fd_read);
	
	return bundlemem;
}

/**
* \brief checks if there is space for a bundle
* \param bundle pointer to a bundle struct (not used here)
* \return number of free slots
*/
uint16_t free_space(struct mmem * bundlemem)
{
	return BUNDLE_STORAGE_SIZE - bundles_in_storage;
}

/**
* \returns the number of saved bundles
*/
uint16_t get_g_bundel_num(void){
	return bundles_in_storage;
}

/**
 * \returns pointer to first bundle list entry
 */
struct storage_entry_t * get_bundles(void)
{
	return (struct storage_entry_t *) list_head(bundle_list);
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
	get_bundles,
};
/** @} */
/** @} */

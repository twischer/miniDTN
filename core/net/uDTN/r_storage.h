#ifndef __R_STORAGE_H__
#define __R_STORAGE_H__

/**
 * How many bundles can possibly be stored in the data structures?
 */
#ifdef BUNDLE_CONF_STORAGE_SIZE
#define BUNDLE_STORAGE_SIZE BUNDLE_CONF_STORAGE_SIZE
#else
#define BUNDLE_STORAGE_SIZE 	10
#endif

/**
 * How much MMEM memory has to remain free after all bundles have been stored?
 */
#define STORAGE_HIGH_WATERMARK	150

#include "bundle.h"
#include "lib/mmem.h"
#include "net/rime/rimeaddr.h"
#include "routing.h"

extern const struct storage_driver g_storage_driver;

struct file_list_entry_t {
	/** pointer to the next list element */
	struct file_list_entry_t * next;

	/** copy of the bundle number - necessary to have
	 * a static address that we can pass on as an
	 * argument to an event
	 */
	uint16_t bundle_num;

	/** pointer to the actual bundle stored in MMEM */
	struct mmem *bundle;
};

void rs_init(void);
void rs_reinit(void);
/**
returns bundle_num
*/
int32_t rs_save_bundle(struct mmem *bundlemem);

uint16_t rs_del_bundle(uint16_t bundle_num,uint8_t reason);

struct mmem *rs_read_bundle(uint16_t bundle_num);
uint16_t rs_free_space(struct mmem *bundlemem);
#endif

/**
 * \addtogroup redundance
 * @{
 */

 /**
 * \defgroup b_redundance basic redundance check module
 *
 * @{
 */

/**
* \file
* implementation of basic redundance check module
* \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h> // memset

#include "sdnv.h"
#include "redundance.h"
#include "bundle.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "contiki.h"
#include "sys/ctimer.h"
#include "logging.h"

uint32_t redundance_bundle_list[REDUNDANCE_MAX];
uint8_t redundance_bundle_list_pointer;

/**
* \brief checks if bundle was delivered before
* \param bundle pointer to bundle
* \return 1 if bundle was delivered before or 0 if not
*/
uint8_t check(struct mmem *bundlemem)
{
	struct bundle_t * bundle = NULL;
	int i;

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "REDUNDANCE: Checking bundle %lu for redundancy", bundle->bundle_num);

	for(i=0; i<REDUNDANCE_MAX; i++) {
		if( redundance_bundle_list[i] ==  bundle->bundle_num) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "REDUNDANCE: bundle %lu is redundant", bundle->bundle_num);
			return 1;
		}
	}

	return 0;
}

/**
* \brief saves the bundle in a list of delivered bundles
* \param bundle pointer to bundle
* \return 1 on successor 0 on error
*/
uint8_t set(struct mmem *bundlemem)
{
	struct bundle_t * bundle = NULL;

	bundle = (struct bundle_t *) MMEM_PTR(bundlemem);

	redundance_bundle_list[redundance_bundle_list_pointer] = bundle->bundle_num;
	redundance_bundle_list_pointer = (redundance_bundle_list_pointer + 1) % REDUNDANCE_MAX;

	return 1;
}

		
/**
* \brief called by agent at startup
*/
void b_red_init(void)
{
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "REDUNDANCE: starting");

	// Initialize our bundle list
	memset(redundance_bundle_list, 0, sizeof(uint32_t) * REDUNDANCE_MAX);

	// And initialize our ring-buffer pointer
	redundance_bundle_list_pointer = 0;
}

const struct redundance_check b_redundance ={
	"B_REDUNDANCE",
	b_red_init,
	check,
	set,
};
/** @} */
/** @} */

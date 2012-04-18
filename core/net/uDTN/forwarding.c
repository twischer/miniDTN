
 /**
 * \addtogroup bprocess
 *
 * @{
 */

/**
 * \file 
 * implementation of forwarding
 * \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
 */
#include "bundle.h"
#include "agent.h"
#include "custody.h"
#include "storage.h"
#include "mmem.h"


#define DEBUG 0 
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint16_t * forwarding_bundle(struct bundle_t *bundle)
{
	int32_t saved;
	uint16_t * saved_as_num;

	PRINTF("FORWARDING: bundle->mem.ptr %p\n",bundle->mem.ptr);

	if (bundle->flags & 0x08){ // bundle is custody
		PRINTF("FORWARDING: Handing over to custody\n");
		saved = CUSTODY.decide(bundle);
	} else {
		PRINTF("FORWARDING: Handing over to storage\n");
		saved = BUNDLE_STORAGE.save_bundle(bundle);
	}

	if( saved < 0 ) {
		printf("FORWARDING: Bundle could not be saved\n");
		delete_bundle(bundle);
		return NULL;
	}

	PRINTF("FORWARDING: Saved as Bundle No %ld\n", saved);

	saved_as_num = memb_alloc(saved_as_mem);
	if(saved_as_num == NULL){
		printf("FORWARDING: out of MEMB space\n");
		delete_bundle(bundle);
		return NULL;
	}

	*saved_as_num = (uint16_t) saved;
	PRINTF("FORWARDING: bundle_num %u\n", *saved_as_num);
	delete_bundle(bundle);

	return saved_as_num;
}

#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "net/dtn/custody.h"
#include "net/dtn/storage.h"


#define DEBUG 0 
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void forwarding_bundle(struct bundle_t *bundle)
{
	PRINTF("FORWARDING:1 bundle->mem.ptr %p\n",bundle->mem.ptr);
	//uint32_t flags;
	int32_t saved;
	//sdnv_decode(bundle->mem.ptr + bundle->offset_tab[FLAGS][OFFSET], bundle->offset_tab[FLAGS][STATE], &flags);
	if (bundle->flags & 0x08){ // bundle is custody
		saved =CUSTODY.decide(bundle)
	}else{
		saved = BUNDLE_STORAGE.save_bundle(bundle);
	}
	PRINTF("FORWARDING:2 bundle->mem.ptr %p\n",bundle->mem.ptr);
	PRINTF("FORWARDING saved in %ld\n", saved);
	if( saved >=0){
		saved_as_num= (uint16_t)saved;
		PRINTF("FORWARDING: bundle_num %u\n",saved_as_num);
		delete_bundle(bundle);
		PRINTF("FORWARDING\n");
		process_post(&agent_process,dtn_bundle_in_storage_event, &saved_as_num);
	}else{
		delete_bundle(bundle);
		PRINTF("FORWARDING: bundle not saved\n");
		delete_bundle(bundle);
		return;
	}

}

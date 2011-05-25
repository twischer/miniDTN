#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "net/dtn/custody.h"
#include "net/dtn/storage.h"


#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void forwarding_bundle(struct bundle_t *bundle)
{
	if(CUSTODY.decide(bundle)){
		CUSTODY.manage(bundle);
	}
	g_bundle_num=(uint16_t)BUNDLE_STORAGE.save_bundle(bundle);
	if (g_bundle_num >=0){
		PRINTF("FORWARDING: bundle_num %u\n",g_bundle_num);
		process_post(&agent_process,dtn_bundle_in_storage_event, &g_bundle_num);
	}else{
		return;
	}

}

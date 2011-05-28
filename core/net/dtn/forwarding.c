#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "net/dtn/custody.h"
#include "net/dtn/storage.h"
uint16_t saved_as_num;
void forwarding_bundle(struct bundle_t *bundle)
{
	if(CUSTODY.decide(bundle)){
		CUSTODY.manage(bundle);
	}
	saved_as_num=(uint16_t)BUNDLE_STORAGE.save_bundle(bundle);
	if( saved_as_num >=0){
	//	PRINTF("FORWARDING: bundle_num %u\n",saved_as_num);
		process_post(&agent_process,dtn_bundle_in_storage_event, &saved_as_num);
	}else{
		return;
	}
	

}

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
	saved_as_num=BUNDLE_STORAGE.save_bundle(bundle);
	process_post(&agent_process,dtn_bundle_in_storage_event, &saved_as_num);

}

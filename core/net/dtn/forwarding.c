#include "net/dtn/bundle.h"
#include "net/dtn/storage.h"
#include "net/dtn/storage.h"
#include "net/dtn/dtn_config.h"
#include "net/dtn/agent.h"
#include "net/dtn/custody.h"

void forwarding_bundle(struct bundle_t *bundle)
{
	if(CUSTODY.deside(bundle)){
		CUSTODY.manage(bundle);
	}
	STORAGE.save_bundle(bundle);
	process_post(&agent_process,dtn_bundle_in_storage_event, NULL);

}

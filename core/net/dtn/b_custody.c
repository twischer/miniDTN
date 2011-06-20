#include "net/dtn/custody-signal.h"
#include "net/dtn/custody.h"
#include "bundle.h"

void b_cust_init(void)
{
	return;
}

uint8_t b_cust_manage(struct bundle_t *bundle)
{
	//TODO admin record senden, dass bundle abgelehnt wurde
	return 0;
}

uint8_t b_cust_set_state(custody_signal_t *signal)
{
	return 0;
}

int32_t b_cust_decide(struct bundle_t *bundle)
{
	if (BUNDLE_STORAGE.free_space(bundle) > 0){
		bundle->custody=1;
		return BUNDLE_STORAGE.save_bundle(bundle);
	}else{
		return -1;
	}
}


const struct custody_driver null_custody ={
	"B_CUSTODY",
	b_cust_init,
	b_cust_manage,
	b_cust_set_state,
	b_cust_decide,
};


#include "net/dtn/custody-signal.h"
#include "net/dtn/custody.h"

void null_cust_init(void)
{
	return;
}

uint8_t null_cust_manage(custody_signal_t *signal)
{
	//TODO admin record senden, dass bundle abgelehnt wurde
	return 0;
}

uint8_t null_cust_set_state(custody_signal_t *signal)
{
	return 0;
}

uint8_t null_cust_decide(custody_signal_t *signal)
{
	return 0;
}


const struct custody_driver null_custody ={
	"NULL_CUSTODY",
	null_cust_init,
	null_cust_manage,
	null_cust_set_state,
	null_cust_decide,
};


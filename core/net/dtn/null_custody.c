/**
* \addtogroup custody 
* @{
* \defgroup nullcust NULL custody
* @{
* \file
*
*/
#include "net/dtn/custody-signal.h"
#include "net/dtn/custody.h"

void null_cust_init(void)
{
	return;
}

uint8_t null_cust_manage(struct bundle_t *bundle)
{
	//TODO admin record senden, dass bundle abgelehnt wurde
	return 0;
}

uint8_t null_cust_set_state(custody_signal_t *signal)
{
	return 0;
}

int32_t null_cust_decide(struct bundle_t *bundle)
{
	return -1;
}


const struct custody_driver null_custody ={
	"NULL_CUSTODY",
	null_cust_init,
	null_cust_manage,
	null_cust_set_state,
	null_cust_decide,
};
/** @} */
/** @} */


/**
 * \addtogroup custody
 * @{
 */

/**
 * \defgroup custody_null NULL custody module
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 */

#include "custody.h"

void custody_null_init(void)
{
	return;
}

uint8_t custody_null_release(struct mmem *bundle)
{
	return 0;
}

uint8_t custody_null_report(struct mmem *bundle, uint8_t status)
{
	return 0;
}

uint8_t custody_null_decide(struct mmem *bundle, uint32_t ** bundle_number)
{
	return 0;
}

uint8_t custody_null_retransmit(struct mmem *bundle)
{
	return 0;
}

void custody_null_delete_from_list(uint32_t bundle_num)
{
	return;
}

const struct custody_driver custody_null ={
	"NULL_CUSTODY",
	custody_null_init,
	custody_null_release,
	custody_null_report,
	custody_null_decide,
	custody_null_retransmit,
	custody_null_delete_from_list
};
/** @} */
/** @} */


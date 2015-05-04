/**
 * \addtogroup redundancy
 * @{
 */

/**
 * \defgroup redundancy_null null redundancy check module
 *
 * @{
 */

/**
 * \file
 * \brief implementation an empty redundancy check module
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include "lib/memb.h"
#include "contiki.h"

#include "redundancy.h"

uint8_t redundancy_null_check(uint32_t bundle_number)
{
	return 0;
}

uint8_t redundancy_null_set(uint32_t bundle_number)
{
	return 1;
}
		
void redundancy_null_init(void)
{
}

const struct redundance_check redundancy_null = {
	"Redundancy NULL",
	redundancy_null_init,
	redundancy_null_check,
	redundancy_null_set,
};
/** @} */
/** @} */

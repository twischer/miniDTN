/** 
 * \addtogroup bundle_status
 * @{
 */

/**
 * \defgroup bundle_status_null Empty status report module
 * @{
 */

/**
 * \file
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#include "logging.h"

#include "statusreport.h"
#include "administrative_record.h"

int statusreport_null_dummy(status_report_t * report, uint8_t * buffer, uint8_t length)
{
	return 1;
}

uint8_t statusreport_null_send(struct mmem * bundlemem, uint8_t status, uint8_t reason)
{
	return 0;
}

const struct status_report_driver statusreport_null = {
		"STATUSREPORT_NULL",
		statusreport_null_send,
		statusreport_null_dummy,
		statusreport_null_dummy
};
/** @} */
/** @} */

#include "test.h"
#include "watchdog.h"
#include "clock.h"

uint32_t test_precise_timestamp(uint8_t * is_valid) {
	clock_time_t now;
	unsigned short now_fine;
	uint16_t cnt = 0;

	do {
		now_fine = clock_time();
		now = clock_seconds();
		cnt ++;

		/* In our experiments, this function sometimes takes so long,
		 * that the watchdog triggers a reboot
		 */
		if( cnt % 500 ) {
			watchdog_periodic();
		}
	} while (now_fine != clock_time() && cnt < 65000);

	if( is_valid != NULL ) {
		*is_valid = 1;
	}

	/* It may happen, that the loop was aborted due to the counter */
	if( cnt >= 65000 && is_valid != NULL ) {
		*is_valid = 0;
	}

	return ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;
}

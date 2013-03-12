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
	} while (now_fine != clock_time() && cnt < 65000);

	return ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;
}

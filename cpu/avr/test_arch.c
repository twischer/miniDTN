#include "test.h"
#include "watchdog.h"
#include "clock.h"

uint32_t test_precise_timestamp() {
	clock_time_t now;
	clock_time_t now_fine;

	do {
		now_fine = clock_time();
		now = clock_seconds();
	} while (now_fine != clock_time());

	return ((unsigned long)now)*CLOCK_SECOND + now_fine%CLOCK_SECOND;
}

#include "FreeRTOS.h"
#include "task.h"

#include "clock.h"

static unsigned long current_seconds = 0;

void vApplicationTickHook(void)
{
	static TickType_t ticks_over_second = 0;
	ticks_over_second++;

	if (ticks_over_second >= pdMS_TO_TICKS(1000)) {
		ticks_over_second = 0;
		current_seconds++;
	}
}

unsigned long clock_seconds()
{
	return current_seconds;
}
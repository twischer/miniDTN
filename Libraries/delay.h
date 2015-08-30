#include <stdint.h>
#include "stm32f4xx.h"

void delay_init();

static inline void delay_us(const uint16_t us)
{
	TIM2->CNT = 0;

	while (TIM2->CNT < us) { }
}

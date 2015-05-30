#include <stdint.h>
#include "stm32f4xx.h"

static inline void delay_us(const uint16_t us)
{
	TIM_SetCounter(TIM2, 0);

	while (TIM_GetCounter(TIM2) >= us) { }
}

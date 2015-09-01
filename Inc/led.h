/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __led_H
#define __led_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

typedef enum {
	LED_GREEN	= GPIO_PIN_12,
	LED_ORANGE	= GPIO_PIN_13,
	LED_RED		= GPIO_PIN_14,
	LED_BLUE	= GPIO_PIN_15,
} LED_Pin_t;


static inline void LED_Off(const LED_Pin_t led_pin)
{
	GPIOD->BSRR = led_pin << 16;
}


static inline void LED_On(const LED_Pin_t led_pin)
{
	GPIOD->BSRR = led_pin;
}


static inline void LED_Toggle(const LED_Pin_t led_pin)
{
	GPIOD->ODR ^= led_pin;
}

#ifdef __cplusplus
}
#endif
#endif /* __led_H */


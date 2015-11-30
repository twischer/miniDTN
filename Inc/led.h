/*
 * Used to access the four LEDs for debugging information
 * LED_GREEN indecates receiving a package over LOWPAN or ethernt
 * LED_BLUE sending a package over LOWPAN
 * LED_ORANGE sending a package over ethernet
 * LED_RED indectaes failers (starting FreeRTOS failes, LOWPAN FIFO buffer overflow)
 */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __led_H
#define __led_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#define ENABLE_LEDS 1

#if (ENABLE_LEDS == 1)
#define LED_PORT	GPIOD
typedef enum {
	LED_GREEN	= GPIO_PIN_12,
	LED_ORANGE	= GPIO_PIN_13,
	LED_RED		= GPIO_PIN_14,
	LED_BLUE	= GPIO_PIN_15,
} LED_Pin_t;
#else
/* if the LEDs where disabled use
 * other free pins for debugging output
 */
#define LED_PORT	GPIOA
typedef enum {
	LED_GREEN	= GPIO_PIN_8,
	LED_ORANGE	= GPIO_PIN_15,
	LED_RED		= GPIO_PIN_3,
	LED_BLUE	= GPIO_PIN_10,
} LED_Pin_t;
#endif

static inline void LED_Off(const LED_Pin_t led_pin)
{
	LED_PORT->BSRR = led_pin << 16;
}


static inline void LED_On(const LED_Pin_t led_pin)
{
	LED_PORT->BSRR = led_pin;
}


static inline void LED_Toggle(const LED_Pin_t led_pin)
{
	LED_PORT->ODR ^= led_pin;
}

#ifdef __cplusplus
}
#endif
#endif /* __led_H */


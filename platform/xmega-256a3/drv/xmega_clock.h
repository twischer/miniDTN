#ifndef __XMEGA_CLOCK_H__
#define __XMEGA_CLOCK_H__

#include <avr/io.h>
#include <stdint.h>
#include <avrdef.h>

#if F_CPU == 32000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC32MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC32MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x01
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	OSC_PLLFAC4_bm // 2^4 = 16 -> 2MHz * 16 = 32 MHz
	#define CLOCK_CONF_SECOND 	2000 /* 32 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#elif F_CPU == 8000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC2MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC2MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x00
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	OSC_PLLFAC2_bm // 2^2 = 4-> 2MHz * 4 = 8 MHz
	#define CLOCK_CONF_SECOND 	500 /* 8 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#elif F_CPU == 4000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC2MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC2MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x00
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	OSC_PLLFAC1_bm // 2^1 = 2-> 2MHz * 2 = 2 MHz
	#define CLOCK_CONF_SECOND 	250 /* 4 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#elif F_CPU == 2000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC2MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC2MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x00
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	OSC_PLLFAC0_bm // 2^0 = 1-> 2MHz * 1 = 2 MHz
	#define CLOCK_CONF_SECOND 	125 /* 2 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#else
	#error CPU Frequence Unknown
#endif

void xmega_clock_init(void);


#endif // #ifndef __XMEGA_CLOCK_H__

#ifndef __XMEGA_CLOCK_H__
#define __XMEGA_CLOCK_H__

#include <avr/io.h>
#include <stdint.h>

// we might even allow higher CPU frequencies here - NOT TESTED

#if F_CPU == 32000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	16
#elif F_CPU == 30000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	15
#elif F_CPU == 28000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	14
#elif F_CPU == 26000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	13
#elif F_CPU == 24000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	12
#elif F_CPU == 22000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	11
#elif F_CPU == 20000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	10
#elif F_CPU == 18000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	9
#elif F_CPU == 16000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	8
#elif F_CPU == 14000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	7
#elif F_CPU == 12000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	6
#elif F_CPU == 10000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	5
#elif F_CPU == 8000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	4
#elif F_CPU == 6000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	3
#elif F_CPU == 4000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	2
#elif F_CPU == 2000000UL
	#define XMEGA_CLOCK_CONF_PLL_FACTOR	1
#else
	#error CPU Frequence Unknown
#endif

void xmega_clock_output(void);
void xmega_clock_init(void);


#endif // #ifndef __XMEGA_CLOCK_H__

//#define __AVR_ATxmega256A3__

#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

#include <avr/io.h>
#include <stdint.h>
#include <avrdef.h>


/* MCU and clock rate */
//#define MCU_MHZ 2
//#define MCU_HZ 2000000
#define PLATFORM PLATFORM_AVR

//#define __STDC_HOSTED__ 1

/* Timer setup, need 125 ticks (32MHz / 1024 / 250 = 125). */

/*
#define XMEGA_OSC_SOURCE OSC_RC32MEN_bm // OSC_RC2MEN_bm 
#define XMEGA_CLOCK_SOURCE CLK_SCLKSEL_RC32M_gc // CLK_SCLKSEL_RC2M_gc
#define CLOCK_CONF_SECOND 125
#define XMEGA_TIMER_TOP 250
#define XMEGA_TIMER_PRE TC_CLKSEL_DIV1024_gc
*/

/* 2MHz / 1024 /   */

/*#define XMEGA_OSC_SOURCE  
#define XMEGA_CLOCK_SOURCE 
#define CLOCK_CONF_SECOND 125
#define XMEGA_TIMER_TOP 250
#define XMEGA_TIMER_PRE TC_CLKSEL_DIV64_gc
*/

#if F_CPU == 32000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC32MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC32MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x01
	#define XMEGA_PLL_FACTOR	OSC_PLLFAC4_bm // 2^4 = 16 -> 2MHz * 16 = 32 MHz
	#define CLOCK_CONF_SECOND 	2000 /* 32 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#elif F_CPU == 8000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC2MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC2MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x00
	#define XMEGA_PLL_FACTOR	OSC_PLLFAC2_bm // 2^2 = 4-> 2MHz * 4 = 8 MHz
	#define CLOCK_CONF_SECOND 	500 /* 8 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#elif F_CPU == 4000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC2MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC2MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x00
	#define XMEGA_PLL_FACTOR	OSC_PLLFAC1_bm // 2^1 = 2-> 2MHz * 2 = 2 MHz
	#define CLOCK_CONF_SECOND 	250 /* 4 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#elif F_CPU == 2000000UL
	//#define XMEGA_OSC_SOURCE	OSC_RC2MEN_bm
	//#define XMEGA_OSC_READY		OSC_RC2MRDY_bm
	//#define XMEGA_CLK_SOURCE	0x00
	#define XMEGA_PLL_FACTOR	OSC_PLLFAC0_bm // 2^0 = 1-> 2MHz * 1 = 2 MHz
	#define CLOCK_CONF_SECOND 	125 /* 2 MHz / TIMER_PRESCALE / TIMER_TOP */
	#define TIMER_TOP      250
	#define TIMER_PRESCALE TC_CLKSEL_DIV64_gc
#else
	#error CPU Frequence Unknown
#endif

// PLL
// #define XMEGA_OSC_SOURCE		OSC_PLL_bm (100)

/* Maximum time interval (used for timers) */
//#define INFINITE_TIME 0xffff

/* Pre-allocated memory for loadable modules heap space (in bytes)*/
//#define MMEM_CONF_SIZE 256

/* LED's */

/*
#define __USE_LEDS__
#define LEDPORT PORTF
#define LED1 PIN7_bm
#define LED2 PIN6_bm
#define LED3 PIN5_bm
#define LED4 PIN4_bm
#define LEDS_CONF_ALL (LED1 | LED2 | LED3 | LED4)
#define LED_STATUS LED1
#define LED_ALERT LED2
*/
/* The following defs satisfy the LEDS api, and do nothing. */
/*
#define LEDS_GREEN PIN3_bm
#define LEDS_YELLOW PIN3_bm
#define LEDS_RED PIN3_bm
#define LEDS_BLUE PIN3_bm
*/

/* Watchdog */
//#define WATCHDOG_CONF_TIMEOUT WDT_PER_8KCLK_gc

/* ? */

#define CCIF
#define CLIF

//#define BOARD XMEGA_A3BU_XPLAINED
//#define CONFIG_NVM_IGNORE_XMEGA_A3_D3_REVB_ERRATA
//#define IOPORT_XMEGA_COMPAT

// #define RTIMER_TIMER_NUMBER 0

// Simply define this - dont know what SLIP is used for!
#define SLIP_PORT RS232_PORT_0

//typedef uint8_t irqflags_t;

#endif /* __CONTIKI_CONF_H__ */

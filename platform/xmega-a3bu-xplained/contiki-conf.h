#ifndef __CONTIKI_CONF_H__
#define __CONTIKI_CONF_H__

#include <avr/io.h>
#include <stdint.h>
#include <avrdef.h>

// include drivers for this platform here because there are #ifdef's in the .h files that we need immedeately
#include "xmega_clock.h"
#include "xmega_interrupt.h"

#define PLATFORM PLATFORM_AVR

//#define __STDC_HOSTED__ 1

#define CCIF
#define CLIF

// Simply define this
#define SLIP_PORT RS232_PORT_0

#define PLATFORM_HAS_LEDS	1

// @TODO this is only for XMega Xplained 
#define LEDS_PxDIR PORTR.DIRSET
#define LEDS_PxOUT PORTR.OUT
#define LEDS_CONF_GREEN PIN0_bm
#define LEDS_CONF_YELLOW PIN1_bm
#define LEDS_CONF_ALL (PIN0_bm | PIN1_bm)


#endif /* __CONTIKI_CONF_H__ */

/*
 * Copyright (c) 2013, TU Braunschweig
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *        Plattform config for INGA
 * \author
 *        Enrico Joerns <e.joerns@tu-bs.de>
 */

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

/*
 * Definitions below are dictated by the hardware and not really
 * changeable!
 */

/** Inga revision 1.2  */
#define INGA_V12  12
/** Inga revision 1.4  */
#define INGA_V14  14
/** Inga revision 1.5  */
#define INGA_V15  15
/** Inga revision 2.0  */
#define INGA_V20  20

/** Set default INGA revision if nothing else set 
 * Possible values are INGA_V12, INGA_V15, INGA_V20
 */
#ifndef INGA_CONF_REVISION
#define INGA_REVISION INGA_V12
#else
#define INGA_REVISION INGA_CONF_REVISION
#endif

#define PLATFORM       PLATFORM_AVR

/** Currently all INGA revisions use same HAL */
#define RF230_HAL = INGA_12

#define PLATFORM_HAS_BUTTON 1

/* CPU target speed in Hz */
#ifndef F_CPU
#define F_CPU          8000000UL
#endif

/* Our clock resolution, this is the same as Unix HZ. Depends on F_CPU */
#ifndef CLOCK_CONF_SECOND
#define CLOCK_CONF_SECOND 128UL
#endif

/* Types for clocks and uip_stats */
typedef unsigned short uip_stats_t;
typedef unsigned long clock_time_t;
typedef unsigned long off_t;

/* LED ports */
#define PLATFORM_HAS_LEDS       1
#define LEDS_PxDIR              DDRD
#define LEDS_PxOUT              PORTD
#define LEDS_CONF_GREEN         0x20
#define LEDS_CONF_YELLOW        0x80

/* micro SD ports */
#define MICRO_SD_PWR_PORT       PORTA
#define MICRO_SD_PWR_PORT_DDR   DDRA
#define MICRO_SD_PWR_PIN        PORTA4

#include <util/delay.h>

/**
 * Powers the microSD card on.
 *
 * @note With INGA 1.4 powering on the SD card will unfortunately block SPI MISO.
 */
#define SDCARD_POWER_ON()   {MICRO_SD_PWR_PORT |= (1 << MICRO_SD_PWR_PIN); _delay_ms(2);}

/**
 * Powers the microSD card off.
 */
#define SDCARD_POWER_OFF()  MICRO_SD_PWR_PORT &= ~(1 << MICRO_SD_PWR_PIN)

#endif /* __PLATFORM_CONF_H__ */

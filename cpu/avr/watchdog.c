/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 *
 * This file is part of the Contiki operating system.
 *
 */

/* Watchdog routines for the AVR */

/* The default timeout of 2 seconds works on most MCUs.
 * It should be disabled during sleep (unless used for wakeup) since
 * it draws significant current (~5 uamp on 1284p, 20x the MCU consumption).
 *
 * Note the wdt is not properly simulated in AVR Studio 4 Simulator 1:
 *   On many devices calls to wdt_reset will have no effect, and a wdt reboot will occur.
 *   The MCUSR will not show the cause of a wdt reboot.
 *   A 1MHz clock is assumed; at 8MHz timeout occurs 8x faster than it should.
 * Simulator 2 is supposed to work on supported devices (not atmega128rfa1),
 * but neither it nor Studio 5 beta do any resets on the 1284p.
 *
 * Setting WATCHDOG_CONF_TIMEOUT -1 will disable the WDT.
 */
//#define WATCHDOG_CONF_TIMEOUT -1

#ifndef WATCHDOG_CONF_TIMEOUT
#define WATCHDOG_CONF_TIMEOUT WDTO_2S
#endif

/* While balancing start and stop calls is a good idea, an imbalance will cause
 * resets that can take a lot of time to track down.
 * Some low power protocols may cause this.
 * The default is no balance; define WATCHDOG_CONF_BALANCE 1 to override.
 */
#ifndef WATCHDOG_CONF_BALANCE
#define WATCHDOG_CONF_BALANCE 0
#endif

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <dev/watchdog.h>

// This is needed because AVR libc does not support WDT yet
#ifdef __AVR_XMEGA__
#ifndef wdt_disable
#define wdt_disable() \
  CCP = CCP_IOREG_gc; \
  WDT.CTRL = (WDT.CTRL & ~WDT_ENABLE_bm) | WDT_CEN_bm;
#endif
#endif


/* Keep address over reboots */
void *watchdog_return_addr __attribute__ ((section (".noinit")));

//Not all AVR toolchains alias MCUSR to the older MSUSCR name
//#if defined (__AVR_ATmega8__) || defined (__AVR_ATmega8515__) || defined (__AVR_ATmega16__)
#if !defined (MCUSR) && defined (MCUCSR)
#warning *** MCUSR not defined, using MCUCSR instead ***
#define MCUSR MCUCSR
#endif

#if WATCHDOG_CONF_BALANCE && WATCHDOG_CONF_TIMEOUT >= 0
static int stopped = 0;
#endif

/* Only required if we want to examine reset source later on. */
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

/* 
 * Watchdog may remain activated with cleared prescaler (fastest) after reset.
 * Thus disabling the watchdog during init is required!!
 */
void get_mcusr(void) \
       __attribute__((naked)) \
       __attribute__((section(".init3")));
void get_mcusr(void)
{
  mcusr_mirror = MCUSR;
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
}

/*---------------------------------------------------------------------------*/
  void
watchdog_init(void)
{
  watchdog_return_addr = 0;

#if WATCHDOG_CONF_BALANCE && WATCHDOG_CONF_TIMEOUT >= 0
  stopped = 1;
#endif
}
/*---------------------------------------------------------------------------*/
  void
watchdog_start(void)
{
#if WATCHDOG_CONF_TIMEOUT >= 0
#if WATCHDOG_CONF_BALANCE
  stopped--;
  if(!stopped)
#endif
  {
#ifdef __AVR_XMEGA__	
    // enable watchdog	
    CCP = CCP_IOREG_gc;
    WDT.CTRL = WATCHDOG_CONF_TIMEOUT |  WDT_CEN_bm | WDT_ENABLE_bm;
#else
    wdt_enable(WATCHDOG_CONF_TIMEOUT);
#endif

    // Enable WDT Interrupt
#if defined (__AVR_ATmega1284P__)
    WDTCSR |= _BV(WDIE);	
#endif
  }
#endif  
}
/*---------------------------------------------------------------------------*/
  void
watchdog_periodic(void)
{
#if WATCHDOG_CONF_TIMEOUT >= 0
#if WATCHDOG_CONF_BALANCE
  if(!stopped)
#endif

#ifdef __AVR_XMEGA__
  // reset watchdog
  CCP = CCP_IOREG_gc;
  WDT.CTRL = (RST.CTRL & ~WDT_ENABLE_bm) | WDT_CEN_bm;
  CCP = CCP_IOREG_gc;
  WDT.CTRL = WATCHDOG_CONF_TIMEOUT |  WDT_CEN_bm | WDT_ENABLE_bm;
#else
  wdt_reset();
#endif

#endif
}
/*---------------------------------------------------------------------------*/
  void
watchdog_stop(void)
{
#if WATCHDOG_CONF_TIMEOUT >= 0
#if WATCHDOG_CONF_BALANCE
  stopped++;
#endif
  wdt_disable();
  // Disable WDT Interrupt
#if defined (__AVR_ATmega1284P__)
  WDTCSR &= ~_BV(WDIE);
#endif
#endif
}
/*---------------------------------------------------------------------------*/
void
watchdog_reboot(void)
{
  cli();
  wdt_enable(WDTO_15MS); //wd on,250ms 
  while(1); //loop until watchdog resets
}
/*---------------------------------------------------------------------------*/
/* Not all AVRs implement the wdt interrupt */
#if defined (__AVR_ATmega1284P__)
ISR(WDT_vect)
{
  /* The address is given in words (16-bit), but all GNU tools use bytes
   * so we need to multiply with 2 here. */
  watchdog_return_addr = (void *)((unsigned int)__builtin_return_address(0)<<1);
}
#endif

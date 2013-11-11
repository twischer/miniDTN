/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 */

/**
 * \file EEPROM read/write routines for AVR
 *
 * \author
 *        Adam Dunkels <adam@sics.se>
 *        Enrico Joerns <e.joerns@tu-bs.de>
 */

#include "dev/eeprom.h"
#include "dev/watchdog.h"

#include <avr/eeprom.h>
#include <stdio.h>
#include <util/delay.h>

#define EEPROM_WRITE_MAX_TRIES  100

/*---------------------------------------------------------------------------*/
void
eeprom_write(eeprom_addr_t addr, unsigned char *buf, int size)
{
  uint8_t tries = 0;

  // nonblocking wait
  while(!eeprom_is_ready()) {
    watchdog_periodic();
    _delay_ms(1);
    tries++;

    if (tries > EEPROM_WRITE_MAX_TRIES) {
      printf("Error: EEPROM write failed (%d,%u,%d)\n", 0, addr, size);
      return;
    }
  }

  eeprom_write_block(buf, (unsigned short *)addr, size);
}
/*---------------------------------------------------------------------------*/
void
eeprom_read(eeprom_addr_t addr, unsigned char *buf, int size)
{
  uint8_t tries = 0;

  // nonblocking wait
  while(!eeprom_is_ready()) {
    watchdog_periodic();
    _delay_ms(1);
    tries++;
    if (tries > EEPROM_WRITE_MAX_TRIES) {
      printf("Error: EEPROM read failed\n");
      return;
    }
  }

  eeprom_read_block(buf, (unsigned short *)addr, size);
}
/*---------------------------------------------------------------------------*/

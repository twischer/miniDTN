/*
 * Copyright (c) 2012, TU Braunschweig.
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
 *      ADC driver implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors_driver
 * @{
 */

/**
 * \addtogroup adc_driver
 * @{
 */
#include "adc.h"
#include <stdio.h>
#include <avr/pgmspace.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...)
#endif

static uint8_t aref_connected;

void
adc_init(uint8_t mode, uint8_t ref)
{
  ADCSRA = ((ADC_ENABLE) | (ADC_PRESCALE_64));
  ADCSRB = 0x00;

  /* Safety check:
   * Test AREF pin by starting a single conversion with AREF as reference
   * and 0V channel input.
   * If this results in a value of 1023, AREF pin seems to be unconnected.*/
  ADMUX = 0x1F; /* AREF input, 0V reference */
  ADCSRA |= (1 << ADSC); /* Start conversion */
  while (ADCSRA & (1 << ADSC)); /* Wait till done */

  if (ADC == 1023) {
    PRINTF("adc: AREF unconnected\n");
    aref_connected = 0;
  } else {
    PRINTF("adc: AREF connected\n");
    aref_connected = 1;
  }

  /* Do not allow to set internal reference with AREF connected */
  if ((aref_connected) && (ref != ADC_REF_AREF)) {
    PRINTF("adc: Internal reference voltage not allowed!\n");
    return 1;
  }

  ADMUX = ref;

  if (mode != ADC_SINGLE_CONVERSION) {
    ADCSRB |= (0x07 & mode);
    ADCSRA |= ((ADC_TRIGGER_ENABLE) | (ADC_INTERRUPT_ENABLE));
  }
}
/*----------------------------------------------------------------------------*/
void
adc_set_mux(uint8_t mux)
{
  static uint8_t used_adcs = 0;
  /*save energy by disabling the i/o input buffer*/
  if (mux < 8) {
    used_adcs |= (1 << mux);
    DIDR0 |= used_adcs;
  }
  ADMUX &= (0xE0);
  ADMUX |= mux;
  ADCSRA |= ADC_START;
}
/*----------------------------------------------------------------------------*/
uint16_t
adc_get_value(void)
{
  if (ADCSRA & ADC_TRIGGER_ENABLE) {
    /*just read the ADC data register*/
    return ADCW;
  } else {
    /*start single conversion*/
    while (ADCSRA & (1 << ADSC)) {
      ;
    }
    return ADCW;
  }
}
/*----------------------------------------------------------------------------*/
uint16_t
adc_get_value_from(uint8_t chn)
{
  adc_set_mux(chn);
  return adc_get_value();
}
/*----------------------------------------------------------------------------*/
void
adc_deinit(void)
{
  ADCSRA = ADC_STOP;
  ADCSRB = ADC_STOP;
  ADMUX = ADC_STOP;
}
/*----------------------------------------------------------------------------*/

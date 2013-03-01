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
 *      Battery sensor implementation
 * \author
 *      Enrico Joerns <e.joerns@tu-bs.de>
 */

#include "contiki.h"
#include "lib/sensors.h"
#include "adc.h"
#include "battery-sensor.h"

/* VCC ADC channel */
#define PWR_MONITOR_VCC_ADC             2
/* ICC ADC channel */
#define PWR_MONITOR_ICC_ADC             3

/* @2.56Vref: 2^10 => 4267mV; */
#define BATTERY_SENSOR_V_SCALE 25 / 6
/* @2.56Vref: 2^10 => 128mA; */
#define BATTERY_SENSOR_I_SCALE 1 / 4

static uint8_t initialized = 0;

const struct sensors_sensor battery_sensor;
/*----------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
    case BATTERY_VOLTAGE:
      return adc_get_value_from(PWR_MONITOR_VCC_ADC) * BATTERY_SENSOR_V_SCALE;
    case BATTERY_CURRENT:
      return adc_get_value_from(PWR_MONITOR_ICC_ADC) * BATTERY_SENSOR_I_SCALE;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static int
status(int type)
{
  switch (type) {
    case SENSORS_ACTIVE:
    case SENSORS_READY:
      return initialized;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch (type) {
    case SENSORS_ACTIVE:
      if (c) {
        adc_init(ADC_SINGLE_CONVERSION, ADC_REF_2560MV_INT);
        initialized = 1;
        return 1;
      } else {
        adc_deinit();
        initialized = 0;
        return 1;
      }
      break;
    default:
      return 0;
      break;
  }
}
/*----------------------------------------------------------------------------*/
SENSORS_SENSOR(battery_sensor, "PRESSURE", value, configure, status);


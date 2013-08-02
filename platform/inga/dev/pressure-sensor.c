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
 *      Pressure sensor interface implemention
 * \author
 *      Georg von Zengen
 */

#include "contiki.h"
#include "lib/sensors.h"
#include "dev/bmp085.h"
#include "dev/pressure-sensor.h"
const struct sensors_sensor pressure_sensor;
static uint8_t ready = 0;
static uint8_t active = 0;
static uint32_t pressure;
/*----------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
    case PRESS_H:
      pressure = bmp085_read_pressure(0);
      return (uint16_t) ((pressure >> 16) & 0xFFFF);
    case PRESS_L:
      return (uint16_t) (pressure & 0xFFFF);
    case TEMP:
      return (int16_t) bmp085_read_temperature();
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static int
status(int type)
{
  switch (type) {
    case SENSORS_READY:
      return ready;
      break;
    case SENSORS_ACTIVE:
      return active;
      break;
  }
  return active;
}
/*----------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch (type) {
    case SENSORS_HW_INIT:
      if (bmp085_available()) {
        ready = 1;
      } else {
        ready = 0;
      }
      break;
    case SENSORS_ACTIVE:
      if (c) {
        active = 1;
        return (bmp085_init() == 0) ? 1 : 0;
      } else {
        active = 0;
        return (bmp085_deinit() == 0) ? 1 : 0;
      }
      break;
    default:
      break;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
SENSORS_SENSOR(pressure_sensor, PRESSURE_SENSOR, value, configure, status);

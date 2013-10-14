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

#define CFG_READY_  0
#define CFG_ACTIVE_ 1
#define CFG_MODE_   3

const struct sensors_sensor pressure_sensor;
static uint8_t config = 0x00;
static uint32_t pressure;
/*----------------------------------------------------------------------------*/
static int
value(int type)
{
  // if not activated, do not query data
  if (!(config & (1 << CFG_ACTIVE_))) return -1;

  switch (type) {
    case PRESS_H:
      // read with selected mode
      pressure = bmp085_read_pressure((config & (0x3 << CFG_MODE_)) >> CFG_MODE_);
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
      return (config & (1 << CFG_READY_)) >> CFG_READY_;
      break;
    case SENSORS_ACTIVE:
      return (config & (1 << CFG_ACTIVE_)) >> CFG_ACTIVE_;
      break;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch (type) {
    case SENSORS_HW_INIT:
      if (bmp085_available()) {
        config |= (1 << CFG_READY_);
      } else {
        config &= (1 << CFG_READY_);
      }
      break;
    case SENSORS_ACTIVE:
      if (c) {
        config |= (1 << CFG_ACTIVE_);
        return (bmp085_init() == 0) ? 1 : 0;
      } else {
        config &= ~(1 << CFG_ACTIVE_);
        return (bmp085_deinit() == 0) ? 1 : 0;
      }
      break;
    default:
      break;

    case PRESSURE_CONF_OPERATION_MODE:
      switch (c) {
        case PRESSURE_MODE_ULTRA_LOW_POWER:
          config &= ~(0x3 << CFG_MODE_);
          config |= (BMP085_ULTRA_LOW_POWER << CFG_MODE_);
          break;
        case PRESSURE_MODE_STANDARD:
          config &= ~(0x3 << CFG_MODE_);
          config |= (BMP085_STANDARD << CFG_MODE_);
          break;
        case PRESSURE_MODE_HIGH_RES:
          config &= ~(0x3 << CFG_MODE_);
          config |= (BMP085_HIGH_RESOLUTION << CFG_MODE_);
          break;
        case PRESSURE_MODE_ULTRA_HIGH_RES:
          config &= ~(0x3 << CFG_MODE_);
          config |= (BMP085_ULTRA_HIGH_RES << CFG_MODE_);
          break;
      }
      break;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
SENSORS_SENSOR(pressure_sensor, PRESSURE_SENSOR, value, configure, status);

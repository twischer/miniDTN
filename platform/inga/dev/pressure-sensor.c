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
uint8_t press_state = 0;

/*----------------------------------------------------------------------------*/
static int
value(int type) {
  switch (type) {
    case PRESS:
      return (uint16_t) bmp085_read_pressure(0);
    case TEMP:
      //return (uint16_t) bmp085_read_temperature();
      return (int16_t) bmp085_read_temperature();
  }
  return 0;
}

/*----------------------------------------------------------------------------*/
static int
status(int type) {
  return press_state;
}

/*----------------------------------------------------------------------------*/
static int
configure(int type, int c) {
  switch (type) {
    case SENSORS_ACTIVE:
      if (c) {
        press_state = 1;
        return bmp085_init();
      } else {

      }
      break;
    default:
      break;
  }
}
/*----------------------------------------------------------------------------*/
SENSORS_SENSOR(pressure_sensor, "PRESSURE", value, configure, status);

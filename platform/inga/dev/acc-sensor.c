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
 *      Accelerometer sensor implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/* Acceleration sensor interface 
 * Author  : Georg von Zengen
 * Created : 2011/10/17
 */
#include "contiki.h"
#include "lib/sensors.h"
#include "adxl345.h"
#include "acc-sensor.h"

#define FALSE 0
#define TRUE  1
const struct sensors_sensor acc_sensor;
uint8_t acc_state = 0;
/*---------------------------------------------------------------------------*/
static int
value(int type) {
  switch (type) {
    case ACC_X:
      return adxl345_get_x_acceleration();

    case ACC_Y:
      return adxl345_get_y_acceleration();

    case ACC_Z:
      return adxl345_get_z_acceleration();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type) {
  switch (type) {
    case SENSORS_ACTIVE:
      return 0; // TODO: do check
      break;
    case SENSORS_READY:
      return adxl345_ready() == 0 ? TRUE : FALSE;
      break;
  }
  return acc_state;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c) {
  switch (type) {
    case SENSORS_ACTIVE:
      if (c) {
        if (!status(SENSORS_ACTIVE)) {
          return adxl345_init();
        }
        return FALSE;
      } else {
        // deactivate
        return FALSE;
      }
      break;
    case ACC_SENSOR_SENSITIVITY:
      // TODO: check if initialized?
      adxl345_set_g_range(c);
      break;
    case ACC_SENSOR_FIFOMODE:
      adxl345_set_fifomode(c);
      break;
    case ACC_SENSOR_POWERMODE:
      adxl345_set_powermode(c);
      break;
    default:
      break;
  }
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(acc_sensor, "ACCELERATION", value, configure, status);

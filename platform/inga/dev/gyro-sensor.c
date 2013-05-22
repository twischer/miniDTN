/*
 * Copyright (c) 2013, TU Braunschweig.
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
 *      Gyroscope sensor implementation
 * \author
 *      Georg von Zengen
 *      Enrico Joerns <e.joerns@tu-bs.de>
 */

#include "contiki.h"
#include "lib/sensors.h"
#include "l3g4200d.h"
#include "gyro-sensor.h"
const struct sensors_sensor gyro_sensor;
static uint8_t initialized = 0;
#define raw_to_dps(raw) TODO
static angle_data_t gyro_data;
/*---------------------------------------------------------------------------*/
static void
cond_update_gyro_data(int ch)
{
  /* bit positions set to one indicate that data is obsolete and a new readout will
   * needs to be performed. */
  static uint8_t gyro_data_obsolete_vec;
  /* A real readout is performed only when the channels obsolete flag is already
   * set. I.e. the channels value has been used already.
   * Then all obsolete flags are reset to zero. */
  if (gyro_data_obsolete_vec & (1 << ch)) {
    gyro_data = l3g4200d_get_angle();
    gyro_data_obsolete_vec = 0x00;
  }
  /* set obsolete flag for current channel */
  gyro_data_obsolete_vec |= (1 << ch);
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
    case GYRO_X:
      cond_update_gyro_data(GYRO_X);
      return l3g4200d_raw_to_dps(gyro_data.x);

    case GYRO_Y:
      cond_update_gyro_data(GYRO_Y);
      return l3g4200d_raw_to_dps(gyro_data.y);

    case GYRO_Z:
      cond_update_gyro_data(GYRO_Z);
      return l3g4200d_raw_to_dps(gyro_data.z);

    case GYRO_X_RAW:
      cond_update_gyro_data(GYRO_X_RAW);
      return gyro_data.x;

    case GYRO_Y_RAW:
      cond_update_gyro_data(GYRO_Y_RAW);
      return gyro_data.y;

    case GYRO_Z_RAW:
      cond_update_gyro_data(GYRO_Z_RAW);
      return gyro_data.z;

    case GYRO_TEMP:
      return l3g4200d_get_temp();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
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
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  uint8_t value = 0;
  switch (type) {

    case SENSORS_ACTIVE:
      if (c) {
        if (l3g4200d_init() == 0) {
          initialized = 1;
          return 1;
        }
      }
      break;

    case GYRO_CONF_SENSITIVITY:
      switch (c) {
        case GYRO_250DPS:
          value = L3G4200D_250DPS;
          break;
        case GYRO_500DPS:
          value = L3G4200D_500DPS;
          break;
        case GYRO_2000DPS:
          value = L3G4200D_2000DPS;
          break;
        default:
          // invalid value
          return 0;
      }
      return (l3g4200d_set_dps(value) == 0) ? 1 : 0;

    case GYRO_CONF_DATA_RATE:
      if (c <= GYRO_100HZ) {
        value = L3G4200D_ODR_100HZ;
      } else if (c <= GYRO_200HZ) {
        value = L3G4200D_ODR_200HZ;
      } else if (c <= GYRO_400HZ) {
        value = L3G4200D_ODR_400HZ;
      } else if (c <= GYRO_800HZ) {
        value = L3G4200D_ODR_800HZ;
      } else {
        // invalid value
        return 0;
      }
      return (l3g4200d_set_data_rate(value) == 0) ? 1 : 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(gyro_sensor, "GYROSCOPE", value, configure, status);

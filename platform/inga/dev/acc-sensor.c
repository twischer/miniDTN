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
#include <stdbool.h>

#define FALSE 0
#define TRUE  1

const struct sensors_sensor acc_sensor;
uint8_t acc_state = 0;
bool interrupt_mode = false; // TODO: needed for possible later implementations with interrupts/events
static uint8_t prescaler;
static bool acc_active = false;
#define raw_to_mg(raw) (raw * 63) / prescaler;
static acc_data_t acc_data;
/*---------------------------------------------------------------------------*/
static int
cond_update_acc_data(int ch)
{
  /* bit positions set to one indicate that data is obsolete and a new readout will
   * needs to be performed. */
  static uint8_t acc_data_obsolete_vec;
  /* A real readout is performed only when the channels obsolete flag is already
   * set. I.e. the channels value has been used already.
   * Then all obsolete flags are reset to zero. */
  if (acc_data_obsolete_vec & (1 << ch)) {
    acc_data = adxl345_get_acceleration();
    acc_data_obsolete_vec = 0x00;
  }
  /* set obsolete flag for current channel */
  acc_data_obsolete_vec |= (1 << ch);
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch (type) {
    case ACC_X:
      cond_update_acc_data(ACC_X);
      return acc_data.x;

    case ACC_Y:
      cond_update_acc_data(ACC_Y);
      return acc_data.y;

    case ACC_Z:
      cond_update_acc_data(ACC_Z);
      return acc_data.z;

    case ACC_X_MG:
      cond_update_acc_data(ACC_X_MG);
      return raw_to_mg(acc_data.x);

    case ACC_Y_MG:
      cond_update_acc_data(ACC_Y_MG);
      return raw_to_mg(acc_data.y);

    case ACC_Z_MG:
      cond_update_acc_data(ACC_Z_MG);
      return raw_to_mg(acc_data.z);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch (type) {
    case SENSORS_ACTIVE:
      return acc_active; // TODO: do check
      break;
    case SENSORS_READY:
      return adxl345_ready();
      break;
  }
  return acc_state;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  uint8_t value;
  switch (type) {

    case SENSORS_ACTIVE:
      if (c) {
        if (!status(SENSORS_ACTIVE)) {
          prescaler = 16;
          return acc_active = (adxl345_init() == 0) ? 1 : 0;
        }
      }
      break;

    case ACC_CONF_SENSITIVITY:
      // map to register settings
      switch (c) {
        case ACC_2G:
          prescaler = 16;
          value = ADXL345_MODE_2G;
          break;
        case ACC_4G:
          prescaler = 8;
          value = ADXL345_MODE_4G;
          break;
        case ACC_8G:
          prescaler = 4;
          value = ADXL345_MODE_8G;
          break;
        case ACC_16G:
          prescaler = 2;
          value = ADXL345_MODE_16G;
          break;
        default:
          return FALSE;
          break;
      }
      adxl345_set_g_range(value);
      return TRUE;
      break;

    case ACC_CONF_FIFOMODE:
      // map to register settings
      switch (c) {
        case ACC_BYPASS:
          value = ADXL345_MODE_BYPASS;
          break;
        case ACC_FIFO:
          value = ADXL345_MODE_FIFO;
          break;
        case ACC_STREAM:
          value = ADXL345_MODE_STREAM;
          break;
        case ACC_TRIGGER:
          value = ADXL345_MODE_TRIGGER;
          break;
        default:
          return FALSE;
          break;
      }
      adxl345_set_fifomode(value);
      return TRUE;
      break;

    case ACC_CONF_POWERMODE:
      // TODO: implementation needed
      return FALSE;
      break;

    case ACC_CONF_DATA_RATE:
      if (!interrupt_mode) {
        c *= 2; // remember nyquist.
      }
      if (c <= ACC_0HZ10) {
        value = ADXL345_ODR_0HZ10;
      } else if (c <= ACC_0HZ20) {
        value = ADXL345_ODR_0HZ20;
      } else if (c <= ACC_0HZ39) {
        value = ADXL345_ODR_0HZ39;
      } else if (c <= ACC_0HZ78) {
        value = ADXL345_ODR_0HZ78;
      } else if (c <= ACC_1HZ56) {
        value = ADXL345_ODR_1HZ56;
      } else if (c <= ACC_3HZ13) {
        value = ADXL345_ODR_3HZ13;
      } else if (c <= ACC_6HZ25) {
        value = ADXL345_ODR_6HZ25;
      } else if (c <= ACC_12HZ5) {
        value = ADXL345_ODR_12HZ5;
      } else if (c <= ACC_25HZ) {
        value = ADXL345_ODR_25HZ;
      } else if (c <= ACC_50HZ) {
        value = ADXL345_ODR_50HZ;
      } else if (c <= ACC_100HZ) {
        value = ADXL345_ODR_100HZ;
      } else if (c <= ACC_200HZ) {
        value = ADXL345_ODR_200HZ;
      } else if (c <= ACC_400HZ) {
        value = ADXL345_ODR_400HZ;
      } else if (c <= ACC_800HZ) {
        value = ADXL345_ODR_800HZ;
      } else if (c <= ACC_1600HZ) {
        value = ADXL345_ODR_1600HZ;
      } else if (c <= ACC_3200HZ) {
        value = ADXL345_ODR_3200HZ;
      } else {
        return FALSE;
      }
      adxl345_set_data_rate(value);
      return TRUE;
      break;

  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(acc_sensor, "Accelerometer", value, configure, status);

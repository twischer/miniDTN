/*
 * Copyright (c) 2013, TU Braunschweig
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
 * @file gyro-example.c
 * @author Enrico Joerns <e.joerns@tu-bs.de>
 */

#include <stdio.h>
#include "contiki.h"
#include "gyro-sensor.h"

/*---------------------------------------------------------------------------*/
PROCESS(gyro_process, "Gyroscope process");
AUTOSTART_PROCESSES(&gyro_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(gyro_process, ev, data)
{
  PROCESS_BEGIN();

  // just wait shortly to be sure sensor is available
  etimer_set(&timer, CLOCK_SECOND * 0.05);
  PROCESS_YIELD();

  // get pointer to sensor
  static const struct sensors_sensor *gyro_sensor;
  gyro_sensor = sensors_find("Gyro");

  // activate
  SENSORS_ACTIVATE(*gyro_sensor);
  
  // configure
  gyro_sensor->configure(GYRO_CONF_SENSITIVITY, GYRO_250DPS);
  gyro_sensor->configure(GYRO_CONF_DATA_RATE, GYRO_100HZ);

  while (1) {

    // read and output values
    printf("x: %d, y: %d, z: %d\n",
        gyro_sensor->value(GYRO_X),
        gyro_sensor->value(GYRO_Y),
        gyro_sensor->value(GYRO_Z));
  
    PROCESS_PAUSE();

  }

  // deactivate
  SENSORS_DEACTIVATE(*gyro_sensor);

  PROCESS_END();
}


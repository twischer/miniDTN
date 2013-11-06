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
 * @file button-example.c
 * @author Enrico Joerns <e.joerns@tu-bs.de>
 */

#include <stdio.h>
#include "contiki.h"
#include "button-sensor.h"

/*---------------------------------------------------------------------------*/
PROCESS(button_process, "button process");
AUTOSTART_PROCESSES(&button_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(button_process, ev, data)
{
  PROCESS_BEGIN();

  // just wait shortly to be sure sensor is available
  etimer_set(&timer, CLOCK_SECOND * 0.05);
  PROCESS_YIELD();

  // get pointer to sensor
  static const struct sensors_sensor *button_sensor;
  button_sensor = sensors_find("Button");

  // activate and check status
  uint8_t status = SENSORS_ACTIVATE(*button_sensor);
  if (status == 0) {
    printf("Error: Failed to init button sensor, aborting...\n");
    PROCESS_EXIT();
  }
  
  while (1) {

    PROCESS_YIELD();

    if (ev == sensors_event && data == button_sensor) {
      printf("Button %s\n", button_sensor->value(0) ? "pressed" : "released");
    }

  }

  // deactivate (never reached here)
  SENSORS_DEACTIVATE(*button_sensor);

  PROCESS_END();
}




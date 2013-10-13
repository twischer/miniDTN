/*
 * Copyright (c) 2009, Swedish Institute of Computer Science
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

#ifndef __SENSORS_H__
#define __SENSORS_H__

#include "contiki.h"

/** \defgroup sensor-interface Sensor interface
 *
 * \section using_sensor Using a sensor
 * 
 * Each sensor has a set of functions for controlling it and query it for its state.
 * Some sensors also generate an events when the sensors change.
 * A sensor must be activated before it generates events or relevant values.
 * 
 * - SENSORS_ACTIVATE(sensor) - activate the sensor
 * - SENSORS_DEACTIVATE(sensor) - deactivate the sensor
 * - sensor.value(0) - query the sensor for its last value
 * 
 * - sensors_event - event sent when a sensor has changed (the data argument will referer to the actual sensor)
 * 
 * Example for querying the button:
\code
SENSORS_ACTIVATE(button_sensor) // activate the button sensor
button_sensor::value(0) // the button has been pressed or not
\endcode
 * 
 * \section defining_sensor Defining new sensors
 * 
 * Platform designers may need to define new sensors. This sections describes
 * the steps required.
 * 
 * Each sensor requires 3 functions; one for quering data,
 * one for configuring the sensor, and one for requesting status.
 * 
\code
static int my_value(int type) {...}
static int my_configure(int type, int value) {...}
static int my_status(int type) {...}
\endcode
 * 
 * These functions have to adapt the interface described in \ref sensors_sensor.
 * 
 * To create a contiki sensor of them use the SENSORS_SENSOR() macro.
\code
SENSORS_SENSOR(my_sensor, "MY_TYPE", my_value, my_configure, my_status);
\endcode
 * 
 * Finally add the sensor to your platforms sensor list.
\code
SENSORS(..., &my_sensor, ...);
\endcode
 * 
 * @{ */

/** \name Default constants for the configure/status API
 * @{ */
/** internal - used only for (hardware) initialization. */
#define SENSORS_HW_INIT 128
/** Configuration: activates/deactives sensor: 0 -> turn off, 1 -> turn on.
 * Status query: Returns 1 if sensor is active.
 */
#define SENSORS_ACTIVE 129
/** Status query: Returns 1 if sensor is ready */
#define SENSORS_READY 130
/** @} */

/** Activates sensor */
#define SENSORS_ACTIVATE(sensor) (sensor).configure(SENSORS_ACTIVE, 1)
/** Deactivates sensor */
#define SENSORS_DEACTIVATE(sensor) (sensor).configure(SENSORS_ACTIVE, 0)

/** Defines new sensor
 * @param name of the sensor
 * @param String type name of the sensor
 * @param pointer to sensors value function
 * @param pointer to sensors configure function
 * @param pointer to sensors status function
 */
#define SENSORS_SENSOR(name, type, value, configure, status)        \
const struct sensors_sensor name = { type, value, configure, status }

/** Provides the number of available sensors. */
#define SENSORS_NUM (sizeof(sensors) / sizeof(struct sensors_sensor *))

/** Setup defined sensors.
 * @param comma-separated list of pointers to defined sensors.
 *
\code
SENSORS_SENSOR(acc_sensor, "ACC", acc_val, acc_cfg, acc_status);
SENSORS_SENSOR(battery_sensor, "ACC", battery_val, battery_cfg, battery_status);
SENSORS(accsensor, battery_sensor);
\endcode
 */
#define SENSORS(...) \
const struct sensors_sensor *sensors[] = {__VA_ARGS__, NULL};       \
unsigned char sensors_flags[SENSORS_NUM]

/**
 * Sensor defintion structure.
 */
struct sensors_sensor {
  /** String name associated with sensor class. */
  char *       type;
  /** Get sensor value.
   * 
   * @param type Determines which value to read.
   *             Available types depend on the respective sensor implementation.
   * @return Integer sensor value associated with given type
   */
  int          (* value)     (int type);
  /** Configure sensor.
   * 
   * @param type  Configuration type
   *              Beside standard types respective sensor implementations
   *              may define additional types.
   * @param value Value to set for configuration type.
   * @retval 1 if configuration succeeded
   * @retval 0 if configuration failed or type is unknown
   */
  int          (* configure) (int type, int value);
  /** Get status of sensor.
   * 
   * @param type One of (\ref SENSORS_ACTIVE, \ref SENSORS_READY) or sensor-specific ones.
   * @return Return value defined for particular type
   * \retval 0 if type is unknown
   */
  int          (* status)    (int type);
};

/** Returns sensor associated with type name if existing
 * @param type Type name (string)
 * @return pointer to sensor
 */
const struct sensors_sensor *sensors_find(const char *type);
/** Returns next sensor in sensors list
 * @return pointer to sensor
 */
const struct sensors_sensor *sensors_next(const struct sensors_sensor *s);
/** Returns first sensor in sensors list
 * @return pointer to sensor
 */
const struct sensors_sensor *sensors_first(void);

/** Waits for updates from sensor?
 * @param s
 */
void sensors_changed(const struct sensors_sensor *s);

extern process_event_t sensors_event;

PROCESS_NAME(sensors_process);

/** @} */

#endif /* __SENSORS_H__ */

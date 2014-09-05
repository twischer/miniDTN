/*
 * Copyright (c) 2014, TU Braunschweig.
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
 *      Magnetometer sensor implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *      Georg von Zengen
 *      Enrico Jörns <e.joerns@tu-bs.de>
 *      Yannic Schröder <yschroed@ibr.cs.tu-bs.de>
 */

#include "contiki.h"
#include "lib/sensors.h"
#include "mag-sensor.h"
#include "mag3110.h"
#include <stdbool.h>

#define FALSE 0
#define TRUE  1

const struct sensors_sensor mag_sensor;
static uint8_t ready = 0;
static bool mag_active = false;
static mag_data_t mag_data;

/*---------------------------------------------------------------------------*/
static void
cond_update_acc_data(int ch)
{
	/* bit positions set to one indicate that data is obsolete and a new readout will
	 * needs to be performed. */
	static uint8_t mag_data_obsolete_vec = 0xFF;
	/* A real readout is performed only when the channels obsolete flag is already
	 * set. I.e. the channels value has been used already.
	 * Then all obsolete flags are reset to zero. */
	if (mag_data_obsolete_vec & (1 << ch)) {
		mag_data = mag3110_get();
		mag_data_obsolete_vec = 0x00;
	}
	/* set obsolete flag for current channel */
	mag_data_obsolete_vec |= (1 << ch);
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
	switch (type) {
	case MAG_X_RAW:
		cond_update_acc_data(MAG_X_RAW);
		return mag_data.x;

	case MAG_Y_RAW:
		cond_update_acc_data(MAG_Y_RAW);
		return mag_data.y;

	case MAG_Z_RAW:
		cond_update_acc_data(MAG_Z_RAW);
		return mag_data.z;

	case MAG_TEMP:
		return mag3110_get_temp();

	}
	return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
	switch (type) {
	case SENSORS_ACTIVE:
		return mag_active; // TODO: do check
		break;
	case SENSORS_READY:
		return ready;
		break;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
	switch (type) {

	case SENSORS_HW_INIT:
		if (mag3110_available()) {
			ready = 1;
			return 1;
		} else {
			ready = 0;
			return 0;
		}
		break;

	case SENSORS_ACTIVE:
		if (c) {
			return mag_active = (mag3110_init() == 0) ? 1 : 0;
		} else {
			mag3110_deinit();
			return 1;
		}
		break;

	}
	return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(mag_sensor, MAG_SENSOR, value, configure, status);

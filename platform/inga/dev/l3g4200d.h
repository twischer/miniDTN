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
 *      ST L3G4200D 3-axis Gyroscope interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors_driver
 * @{
 */

/** \defgroup l3g4200d_interface ST L3G4200D 3-axis Gyroscope interface
 * @{
 */


#ifndef GYROL3G4200D_H_
#define GYROL3G4200D_H_

#include "../dev/i2c.h"
#include <util/delay.h>

/*Pressure Sensor BMP085 device address*/
#define L3G4200D_DEV_ADDR_R		0xD3
#define L3G4200D_DEV_ADDR_W		0xD2

#define L3G4000D_WHO_I_AM_REG	0x0F
#define L3G4000D_CTRL_REG1 		0x20
#define L3G4000D_CTRL_REG2 		0x21
#define L3G4000D_CTRL_REG3 		0x22
#define L3G4000D_CTRL_REG4 		0x23
#define L3G4000D_CTRL_REG5 		0x24

#define L3G4000D_REFERENCE 		0x25

#define L3G4000D_OUT_TEMP 		0x26

#define L3G4000D_STATUS_REG 	0x27

#define L3G4000D_OUT_X_L 		0x28
#define L3G4000D_OUT_X_H 		0x29
#define L3G4000D_OUT_Y_L 		0x2A
#define L3G4000D_OUT_Y_H 		0x2B
#define L3G4000D_OUT_Z_L 		0x2C
#define L3G4000D_OUT_Z_H 		0x2D

typedef struct {
	uint16_t angle_x_value;
	uint16_t angle_y_value;
	uint16_t angle_z_value;
} angle_data_t;

int8_t l3g4200d_init(void);
angle_data_t l3g4200d_get_angle(void);
uint16_t l3g4200d_get_x_angle(void);
uint16_t l3g4200d_get_y_angle(void);
uint16_t l3g4200d_get_z_angle(void);
uint8_t l3g4200d_get_temp(void);
uint16_t l3g4200d_read16bit(uint8_t addr);
uint8_t l3g4200d_read8bit(uint8_t addr);
void l3g4200d_write8bit(uint8_t addr, uint8_t data);

/** @} */ // l3g4200d_interface
/** @} */ // inga_sensors_driver

#endif /* GYROL3G4200D_H_ */

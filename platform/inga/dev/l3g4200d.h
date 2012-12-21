/* Copyright (c) 2010, Ulf Kulau
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \defgroup L3G4200D_interface ST L3G4200D 3-axis Gyroscope interface
 *
 * <p>
 * </p>
 *
 *
 * @{
 *
 */

/**
 * \file
 *		ST L3G4200D 3-axis Gyroscope interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#ifndef GYROL3G4200D_H_
#define GYROL3G4200D_H_

#include "../drv/i2c-drv.h"
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
#endif /* GYROL3G4200D_H_ */

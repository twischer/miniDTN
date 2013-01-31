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

/* Gyroscope device address*/
#define L3G4200D_DEV_ADDR_R		0xD3
#define L3G4200D_DEV_ADDR_W		0xD2

#define L3G4200D_FIFO_SIZE		32

/** \name Register addresses.
 * 
 * Naming convention of the datasheet with prefix L3G4200D_ is used.
 * 
 * @{
 */
/// Device identification register
#define L3G4200D_WHO_I_AM_REG 0x0F
/// Control register 1
#define L3G4200D_CTRL_REG1    0x20
/// Control register 2
#define L3G4200D_CTRL_REG2    0x21
/// Control register 3
#define L3G4200D_CTRL_REG3    0x22
/// Control register 4
#define L3G4200D_CTRL_REG4    0x23
/// Control register 5
#define L3G4200D_CTRL_REG5    0x24
/// Reference value for Interrupt generation
#define L3G4200D_REFERENCE    0x25
/// Temperature data
#define L3G4200D_OUT_TEMP     0x26
/// Status register
#define L3G4200D_STATUS_REG   0x27
/// X-axis angular rate data. (low)
#define L3G4200D_OUT_X_L 		  0x28
/// X-axis angular rate data. (high)
#define L3G4200D_OUT_X_H 		  0x29
/// Y-axis angular rate data. (low)
#define L3G4200D_OUT_Y_L 		  0x2A
/// Y-axis angular rate data. (high)
#define L3G4200D_OUT_Y_H 		  0x2B
/// Z-axis angular rate data. (low)
#define L3G4200D_OUT_Z_L 		  0x2C
/// Z-axis angular rate data. (high)
#define L3G4200D_OUT_Z_H 		  0x2D
/// Fifo control register
#define L3G4200D_FIFO_CTRL_REG  0x2E
/// Fifo source register
#define L3G4200D_FIFO_SRC_REG   0x2F
/** @} */

/** \name Bit addresses for CTRL_REG1
 * @{
 */
/// Output Data Rate selection (higher)
#define L3G4200D_DR1        7
/// Output Data Rate selection (lower)
#define L3G4200D_DR0        6
/// Bandwidth selection (higher)
#define L3G4200D_BW1        5
/// Bandwidth selection (lower)
#define L3G4200D_BW0        4
/// Power down mode enable.
#define L3G4200D_PD         3
/// Z axis enable.
#define L3G4200D_ZEN        2
/// Y axis enable.
#define L3G4200D_YEN        1
/// X axis enable
#define L3G4200D_XEN        0
/** @} */

/** \name Bit addresses for CTRL_REG2.
 * @{
 */
/// High Pass filter Mode Selection (higher)
#define L3G4200D_HPM1       5
/// High Pass filter Mode Selection (lower)
#define L3G4200D_HPM0       4
/// High Pass filter Cutt Off frequency selection (highest)
#define L3G4200D_HPCF3      3
/// High Pass filter Cutt Off frequency selection
#define L3G4200D_HPCF2      2
/// High Pass filter Cutt Off frequency selection
#define L3G4200D_HPCF1      1
/// High Pass filter Cutt Off frequency selection (lowest)
#define L3G4200D_HPCF0      0
/** @} */

/** \name Bit addresses for CTRL_REG3.
 * @{
 */
/// Interrupt enable on INT1 pin
#define L3G4200D_I1_INT1    7
/// Boot status available on INT1
#define L3G4200D_I1_BOOT    6
/// Interrupt active configuration on INT1
#define L3G4200D_H_LACTIVE  5
/// Push-Pull / Open drain
#define L3G4200D_PP_OD      4
/// Data Ready on DRDY/INT2
#define L3G4200D_I2_DRDY    3
/// FIFO Watermark interrupt on DRDY/INT2
#define L3G4200D_I2_WTM     2
/// FIFO Overrun interrupt on DRDY/INT2
#define L3G4200D_I2_ORUN    1
/// FIFO Empty interrupt on DRDY/INT2
#define L3G4200D_I2_EMPTY   0
/** @} */

/** \name Bit addresses for CTRL_REG4.
 * @{
 */
/// Block Data Update
#define L3G4200D_BDU        7
/// Big/Little Endian Data Selection
#define L3G4200D_BLE        6
/// Full Scale Selection (higher)
#define L3G4200D_FS1        5
/// Full Scale Selection (lower)
#define L3G4200D_FS0        4
/// Self Test Enable (higher)
#define L3G4200D_ST1        2
/// Self Test Enable (lower)
#define L3G4200D_ST0        1
/// SPI Serial Interface mode selection
#define L3G4200D_SIM        0
/** @} */

/** \name Bit addresses for CTRL_REG5.
 * @{
 */
/// Reboot memory content
#define L3G4200D_BOOT       7
/// FIFO enable
#define L3G4200D_FIFO_EN    6
/// High Pass filter Enable
#define L3G4200D_HPEN       4
/// INT1 selection configuration (higher)
#define L3G4200D_INT1_SEL1  3
/// INT1 selection configuration (lower)
#define L3G4200D_INT1_SEL0  2
/// Out selection configuration (higher)
#define L3G4200D_OUT_SEL1   1
/// Out selection configuration (lower)
#define L3G4200D_OUT_SEL0   0
/** @} */

/** \name Bit addresses for STATUS_REG.
 * @{
 */
/// X, Y, Z-axis data overrun
#define L3G4200D_ZYXOR      7
/// Z axis data overrun
#define L3G4200D_ZOR        6
/// Y axis data overrun
#define L3G4200D_YOR        5
/// X axis data overrun
#define L3G4200D_XOR        4
/// X, Y, Z-axis new data available
#define L3G4200D_ZYXDA      3
/// Z axis new data available
#define L3G4200D_ZDA        2
/// Y axis new data available
#define L3G4200D_YDA        1
/// X axis new data available
#define L3G4200D_XDA        0
/** @} */

/** \name Bit addresses for FIFO_CTRL_REG.
 * @{
 */
/// FIFO mode selection
#define L3G4200D_FM2        7
/// FIFO mode selection
#define L3G4200D_FM1        6
/// FIFO mode selection
#define L3G4200D_FM0        5
/// FIFO threshold. Watermark level setting (highest)
#define L3G4200D_WTM4       4
/// FIFO threshold. Watermark level setting
#define L3G4200D_WTM3       3
/// FIFO threshold. Watermark level setting
#define L3G4200D_WTM2       2
/// FIFO threshold. Watermark level setting
#define L3G4200D_WTM1       1
/// FIFO threshold. Watermark level setting (lowest)
#define L3G4200D_WTM0       0
/** @} */

/** \name Bit addresses for FIFO_SRC_REG.
 * @{
 */
/// Watermark status
#define L3G4200D_WTM        7
/// Overrun bit status
#define L3G4200D_OVRN       6
/// FIFO empty bit
#define L3G4200D_EMPTY      5
/// FIFO stored data level (highest)
#define L3G4200D_FSS4       4
/// FIFO stored data level
#define L3G4200D_FSS3       3
/// FIFO stored data level
#define L3G4200D_FSS2       2
/// FIFO stored data level
#define L3G4200D_FSS1       1
/// FIFO stored data level (lowest)
#define L3G4200D_FSS0       0
/** @} */


/** \name Resolution Settings.
 * @{
 */
#define L3G4200D_250DPS           (0x00 << L3G4200D_FS0)
#define L3G4200D_500DPS           (0x01 << L3G4200D_FS0)
#define L3G4200D_2000DPS          (0x02 << L3G4200D_FS0)
/** @} */

/** \name Data rate Settings.
 * @{
 */
#define L3G4200D_100HZ      (0x0 << L3G4200D_DR0)
#define L3G4200D_200HZ      (0x1 << L3G4200D_DR0)
#define L3G4200D_400HZ      (0x2 << L3G4200D_DR0)
#define L3G4200D_800HZ      (0x3 << L3G4200D_DR0)
/** @} */

/** \name Mode Settings.
 * @{
 */
#define L3G4200D_BYPASS           (0x00 << L3G4200D_FM0)
#define L3G4200D_FIFO             (0x01 << L3G4200D_FM0)
#define L3G4200D_STREAM           (0x02 << L3G4200D_FM0)
#define L3G4200D_STREAM_TO_FIFO   (0x03 << L3G4200D_FM0)
#define L3G4200D_BYPASS_TO_STREAM (0x04 << L3G4200D_FM0)
/** @} */


// types

/** Angle data type. */
typedef struct {
#ifdef L3G4200D_FLOAT_EN
  float angle_x_value;
  float angle_y_value;
  float angle_z_value;
#else
  int16_t angle_x_value;
  int16_t angle_y_value;
  int16_t angle_z_value;
#endif
} angle_data_t;


// functions

/** Inits the gyroscope. */
int8_t l3g4200d_init(void);


/** Sets the sensitivity value [dps]
 * 
 * @param set One of L3G4200D_250DPS, L3G4200D_500DPS or L3G4200D_2000DBS
 */
extern inline void l3g4200d_set_dps(uint8_t set);


/** Sets the fifo mode.
 * 
 * @note To use stream or fifo mode, the fifo has to be enabled
 * 	with l3g4200d_fifo_enable().
 * 
 * @param set One of L3G4200D_BYPASS, L3G4200D_FIFO, L3G4200D_STREAM,
 * L3G4200D_STREAM_TO_FIFO or L3G4200D_BYPASS_TO_STREAM
 */
extern inline void l3g4200d_set_fifomode(uint8_t set);


/** Enables fifo mode.
 */
extern inline void l3g4200d_fifo_enable(void);


/** Checks for fifo overrun.
 * 
 * @return 0 = no overrun, else overrun
 */
extern inline int8_t l3g4200d_fifo_overrun(void);


/** Reads data for x,y and z angle.
 * 
 * @note This function is faster than l3g4200d_get_x_angle (etc.) if all values are required!
 */
angle_data_t l3g4200d_get_angle(void);

/** Same as l3g4200d_get_angle() but returns data in deci dps l*/
angle_data_t l3g4200d_get_angle_ddps(void);
angle_data_t l3g4200d_get_dps(void);

/** Reads angle values from fifo.
 *
 * @note This will only work if fifo/stream mode is enabled.
 *
 * @param ret Output data is written to this address
 * @return Number of data sets read (max. L3G4200D_FIFO_SIZE)
 */
int8_t l3g4200d_get_angle_fifo(angle_data_t* ret);


/** Reads x angle value
 * @return x angle value
 */
int16_t l3g4200d_get_x_angle(void);
int16_t l3g4200d_get_x_angle_ddps(void);

/** Reads y angle value
 * @return y angle value
 */
int16_t l3g4200d_get_y_angle(void);
int16_t l3g4200d_get_y_angle_ddps(void);


/** Reads z angle value
 * @return z angle value
 */
int16_t l3g4200d_get_z_angle(void);
int16_t l3g4200d_get_z_angle_ddps(void);


/** Reads temperature value
 * @return temperature
 */
uint8_t l3g4200d_get_temp(void);


/** Reads 2x8 bit register from gyroscopes
 * @param addr address to read from (lower byte)
 * @return register content
 */
uint16_t l3g4200d_read16bit(uint8_t addr);


/** Reads 8 bit register from gyroscopes
 * @param addr address to read from
 * @return register content
 */
uint8_t l3g4200d_read8bit(uint8_t addr);


/** Writes 8 bit to gyroscope register
 * @param addr address to write to
 * @param data data to write
 */
void l3g4200d_write8bit(uint8_t addr, uint8_t data);

#endif /* GYROL3G4200D_H_ */

/** @} */
/** @} */

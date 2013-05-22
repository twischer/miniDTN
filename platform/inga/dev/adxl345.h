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
 *		ADXL345 Accelerometer interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *      Enrico Jörns <joerns@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_sensors_driver
 * @{
 */

/**
 * \defgroup adxl345_interface ADXL345 Accelerometer Interface
 * 
 * \section Registers and Bits
 * 
 * All registers are named \c ADXL345_name_REG where \em name is the name
 * of the register as described in the sensors datasheet.
 * 
 * All bit positions are named \c ADXL345_name for single bit entries
 * and ADXL345_name_L for multiple bit entries, where the position always
 * means the entries LSB.
 * @{
 */

#ifndef ADXL345_H_
#define ADXL345_H_

#include "../dev/mspi.h"
#include <stdio.h>
#include <util/delay.h>

/*!
 * SPI device order.
 * The chip select number where the
 * ADXL345 is connected to the BCD-decimal decoder
 */
#define ADXL345_CS 					2

/** Device ID Register */
#define ADXL345_DEVICE_ID_REG     0x00

/** \name BW_RATE register/bits
 * \{ */
/** ADXL Data Rate and Power Mode Control Register. */
#define ADXL345_BW_RATE_REG       0x2C
/** Low power bit pos. */
#define ADXL345_LOW_POWER     4
/** Rate bits [4] pos. */
#define ADXL345_RATE_L        0
/** \} */

/** \name POWER_CTL register/bits
 * \{ */
/** ADXL Power Control Register. */
#define ADXL345_POWER_CTL_REG     0x2D
/** Link bit pos.*/
#define ADXL345_LINK          5
/** AUTO_SLEEP bit pos.*/
#define ADXL345_AUTO_SLEEP    4
/** Measure bit pos.*/
#define ADXL345_MEASURE       3
/** Sleep bit pos.*/
#define ADXL345_SLEEP         2
/** Wakeup bits [2] LSB pos. */
#define ADXL345_WAKEUP_L      0
/** \} */

/** \name DATA_FORMAT register/bits
 * \{ */
/** ADXL Data Format Register Register. */
#define ADXL345_DATA_FORMAT_REG		0x31
/** SELF_TEST bit pos. */
#define ADXL345_SELF_TEST     7
/** SPI bit pos. */
#define ADXL345_SPI           6
/** INT_INVERT bit pos. */
#define ADXL345_INT_INVERT		5
/** FULL_RES bit pos. */
#define ADXL345_FULL_RES      3
/** Justify bit pos. */
#define ADXL345_JUSTIFY       2
/** Range bits [2] LSB pos. */
#define ADXL345_RANGE_L       0
/** \} */

/** \name FIFO_CTL register/bits
 * \{ */
/** FIFO control register */
#define ADXL345_FIFO_CTL_REG      0x38
/** FIFO_MODE bits [2] LSB pos. */
#define ADXL345_FIFO_MODE_L		6
/** Trigger bit pos. */
#define ADXL345_TRIGGER       5
/** Samples bits [5] LSB pos. */
#define ADXL345_SAMPLES_L     0
/** \} */

/** \name FIFO_STATUS register/bits
 * \{ */
/** FIFO status register */
#define ADXL345_FIFO_STATUS_REG		0x39
/** FIFO_TRIG bit pos. */
#define ADXL345_FIFO_TRIG     7
/** Entries bits [6] LSB pos. */
#define ADXL345_ENTRIES_L     0
/** \} */

/** x Acceleration Data register (high) */
#define ADXL345_OUTX_LOW_REG      0x32
/** x Acceleration Data register (low) */
#define ADXL345_OUTX_HIGH_REG  		0x33
/** y Acceleration Data register (high) */
#define ADXL345_OUTY_LOW_REG      0x34
/** y Acceleration Data register (low) */
#define ADXL345_OUTY_HIGH_REG     0x35
/** z Acceleration Data register (high) */
#define ADXL345_OUTZ_LOW_REG      0x36
/** z Acceleration Data register (low) */
#define ADXL345_OUTZ_HIGH_REG     0x37


/**
 * \name g range settings
 * \{
 */
/** +/- 2g, 256 LSB/g */
#define ADXL345_MODE_2G				(0x0 << ADXL345_RANGE_L)
/** +/- 4g, 128 LSB/g */
#define ADXL345_MODE_4G				(0x1 << ADXL345_RANGE_L)
/** +/- 8g, 64 LSB/g */
#define ADXL345_MODE_8G				(0x2 << ADXL345_RANGE_L)
/** +/- 16g, 32 LSB/g */
#define ADXL345_MODE_16G			(0x3 << ADXL345_RANGE_L)
/** \} */

/**
 * \name FIFO mode settings
 * \{
 */
#define ADXL345_MODE_BYPASS		(0x0 << ADXL345_FIFO_MODE_L)
#define ADXL345_MODE_FIFO			(0x1 << ADXL345_FIFO_MODE_L)
#define ADXL345_MODE_STREAM		(0x2 << ADXL345_FIFO_MODE_L)
#define ADXL345_MODE_TRIGGER	(0x3 << ADXL345_FIFO_MODE_L)
/** \} */

/**
 * \name Power mode settings
 * \{ */
#define ADXL345_PMODE_SLEEP       1
#define ADXL345_PMODE_WAKEUP      3
#define ADXL345_PMODE_STANDBY     4
/** \} */

/**
 * \name Values for output data rate
 * \see ADXL345_BW_RATE_REG
 * \{
 */
/* ODR: 0.1 Hz, bandwith: 0.05Hz, I_DD: 23 µA */
#define ADXL345_ODR_0HZ10			(0x0 << ADXL345_RATE_L)
/* ODR: 0.2 Hz, bandwith: 0.1Hz, I_DD: 23 µA */
#define ADXL345_ODR_0HZ20			(0x1 << ADXL345_RATE_L)
/* ODR: 0.39 Hz, bandwith: 0.2Hz, I_DD: 23 µA */
#define ADXL345_ODR_0HZ39			(0x2 << ADXL345_RATE_L)
/* ODR: 0.78 Hz, bandwith: 0.39Hz, I_DD: 23 µA */
#define ADXL345_ODR_0HZ78			(0x3 << ADXL345_RATE_L)
/* ODR: 1.56 Hz, bandwith: x.xxHz, I_DD: 34 µA */
#define ADXL345_ODR_1HZ56			(0x4 << ADXL345_RATE_L)
/* ODR: 3.13 Hz, bandwith: x.xxHz, I_DD: 40 µA */
#define ADXL345_ODR_3HZ13			(0x5 << ADXL345_RATE_L)
/* ODR: 6.25 Hz, bandwith: x.xxHz, I_DD: 45 µA */
#define ADXL345_ODR_6HZ25			(0x6 << ADXL345_RATE_L)
/* ODR: 12.5 Hz, bandwith: x.xxHz, I_DD: 50 µA */
#define ADXL345_ODR_12HZ5			(0x7 << ADXL345_RATE_L)
/* ODR: 25 Hz, bandwith: x.xxHz, I_DD: 60 µA */
#define ADXL345_ODR_25HZ			(0x8 << ADXL345_RATE_L)
/* ODR: 50 Hz, bandwith: x.xxHz, I_DD: 90 µA */
#define ADXL345_ODR_50HZ			(0x9 << ADXL345_RATE_L)
/* ODR: 100 Hz, bandwith: x.xxHz, I_DD: 140 µA */
#define ADXL345_ODR_100HZ			(0xA << ADXL345_RATE_L)
/* ODR: 200 Hz, bandwith: x.xxHz, I_DD: 140 µA */
#define ADXL345_ODR_200HZ			(0xB << ADXL345_RATE_L)
/* ODR: 400 Hz, bandwith: x.xxHz, I_DD: 140 µA */
#define ADXL345_ODR_400HZ			(0xC << ADXL345_RATE_L)
/* ODR: 800 Hz, bandwith: x.xxHz, I_DD: 140 µA */
#define ADXL345_ODR_800HZ			(0xD << ADXL345_RATE_L)
/* ODR: 1600 Hz, bandwith: x.xxHz, I_DD: 90 µA */
#define ADXL345_ODR_1600HZ		(0xE << ADXL345_RATE_L)
/* ODR: 3200 Hz, bandwith: x.xxHz, I_DD: 140 µA */
#define ADXL345_ODR_3200HZ		(0xF << ADXL345_RATE_L)

/** \} */

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;
} acc_data_t;

/**
 * \brief Initialize the ADXL345 accelerometer
 * 
 * Default settings are: TODO
 *
 * \retval 0 Initialization succeeded
 * \retval -1 Initialization failed
 */
int8_t adxl345_init(void);

/**
 * Deinitilizes the ADX345 accelerometer.
 * 
 * I.e. sets it in standby mode.
 */
void adxl345_deinit(void);


/**
 * Checks whether the device is ready or not by reading the ID.
 * \retval 1 ready
 * \retval 0 not ready
 */
int8_t adxl345_ready(void);

/**
 * 
 * @param range
 */
inline void adxl345_set_g_range(uint8_t range);

/**
 * 
 * @param rate
 */
void adxl345_set_data_rate(uint8_t rate);

/**
 * 
 * @param mode One of ADXL345_MODE_BYPASS, ADXL345_MODE_FIFO, ADXL345_MODE_STREAM, ADXL345_MODE_TRIGGER
 */
void adxl345_set_fifomode(uint8_t mode);

/**
 * 
 * @param mode One of ADXL345_PMODE_SLEEP, ADXL345_PMODE_WAKEUP, ADXL345_PMODE_STANDBY
 */
void adxl345_set_powermode(uint8_t mode);

/**
 * Returns the current fill level of the internal FIFO (when operating in FIFO/Stream mode).
 * @return Fill level [0 - 33]
 */
uint8_t adxl345_get_fifo_level();

/**
 * \brief This function returns the current measured acceleration
 * at the x-axis of the adxl345
 *
 * \return current x-axis acceleration value
 */
int16_t adxl345_get_x_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * at the y-axis of the adxl345
 *
 * \return current y-axis acceleration value
 */
int16_t adxl345_get_y_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * at the z-axis of the adxl345
 *
 * \return current z-axis acceleration value
 */
int16_t adxl345_get_z_acceleration(void);

/**
 * \brief This function returns the current measured acceleration
 * of all axis (x,y,z)
 * \note This is more efficient than reading each axis individually
 *
 * \return current acceleration value of all axis
 */
acc_data_t adxl345_get_acceleration(void);

/** Convert raw value to mg value */
#define adxl345_raw_to_mg(raw) (raw * 62) / 16; // approx 3.9 scale factor


/**
 * \brief This function writes data to the given register
 *        of the ADXL345
 * \param reg  The register address
 * \param data The data value
 */
void adxl345_write(uint8_t reg, uint8_t data);

/**
 * \brief This function reads from the given register
 *        of the ADXL345
 * \param reg   The register address
 * \return      The data value
 */
uint8_t adxl345_read(uint8_t reg);

/** @} */
/** @} */

#endif /* ADXL345_H_ */

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
 *		Bosch BMP085 Digital Pressure Sensor driver implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup bmp085_interface BMP085 Digital Pressure Sensor
 */

#include "bmp085.h"

/*!
 * I2C address to read data
 */
#define BMP085_DEV_ADDR_R		0xEF
/*!
 * I2C address to write data
 */
#define BMP085_DEV_ADDR_W		0xEE
/*Control register address*/
/*!
 * Basic control register address
 */
#define BMP085_CTRL_REG_ADDR	0xF4
/*!
 * Control register value for temperature
 */
#define BMP085_CTRL_REG_TEMP	0x2E
/*!
 * Control register value for oversampling mode 0
 */
#define BMP085_CTRL_REG_PRESS_0	0x34
/*!
 * Control register value for oversampling mode 1
 */
#define BMP085_CTRL_REG_PRESS_1	0x74
/*!
 * Control register value for oversampling mode 2
 */
#define BMP085_CTRL_REG_PRESS_2	0xB4
/*!
 * Control register value for oversampling mode 3
 */
#define BMP085_CTRL_REG_PRESS_3	0xF4
/*Data register*/
/*!
 * Main data register address
 */
#define BMP085_DATA_REG_N		0xF6
/*!
 * Extended data register address for 19bit
 * resolution
 */
#define BMP085_DATA_REG_X		0xF8
/*EEPROM Register addresses for calibration data*/
/*!
 * Coefficient AC1 eeprom address
 */
#define BMP085_AC1_ADDR 		0xAA
/*!
 * Coefficient AC2 eeprom address
 */
#define BMP085_AC2_ADDR			0xAC
/*!
 * Coefficient AC3 eeprom address
 */
#define BMP085_AC3_ADDR			0xAE
/*!
 * Coefficient AC4 eeprom address
 */
#define BMP085_AC4_ADDR			0xB0
/*!
 * Coefficient AC5 eeprom address
 */
#define BMP085_AC5_ADDR			0xB2
/*!
 * Coefficient AC6 eeprom address
 */
#define BMP085_AC6_ADDR			0xB4
/*!
 * Coefficient B1 eeprom address
 */
#define BMP085_B1_ADDR			0xB6
/*!
 * Coefficient B2 eeprom address
 */
#define BMP085_B2_ADDR			0xB8
/*!
 * Coefficient MB eeprom address
 */
#define BMP085_MB_ADDR			0xBA
/*!
 * Coefficient MC eeprom address
 */
#define BMP085_MC_ADDR			0xBC
/*!
 * Coefficient MD eeprom address
 */
#define BMP085_MD_ADDR			0xBE

/*!
 * Hold all coefficients
 */
typedef struct {
  volatile int16_t ac1;
  volatile int16_t ac2;
  volatile int16_t ac3;
  volatile uint16_t ac4;
  volatile uint16_t ac5;
  volatile uint16_t ac6;
  volatile int16_t b1;
  volatile int16_t b2;
  volatile int16_t mb;
  volatile int16_t mc;
  volatile int16_t md;
} bmp085_calib_data;

/*!
 * Hold all coefficients
 */
static bmp085_calib_data bmp085_coeff;

static uint8_t bmp085_read8bit_data(uint8_t addr);
static uint16_t bmp085_read16bit_data(uint8_t addr);
static void bmp085_read_calib_data(void);
static int32_t bmp085_read_uncomp_pressure(uint8_t mode);
static int32_t bmp085_read_uncomp_temperature(void);
/*---------------------------------------------------------------------------*/
int8_t
bmp085_available(void)
{
  uint8_t i;

  i2c_init();
  while (bmp085_read16bit_data(BMP085_AC1_ADDR) == 0x00) {
    _delay_ms(10);
    if (i++ > 10) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
int8_t
bmp085_init(void)
{
  if (!bmp085_available()) {
    return -1;
  }
  bmp085_read_calib_data();
  return 0;
}
/*---------------------------------------------------------------------------*/
int8_t
bmp085_deinit(void)
{
  // todo: needs implementation?
  return 0;
}
/*---------------------------------------------------------------------------*/
static int32_t
bmp085_read_uncomp_temperature(void)
{
  i2c_start(BMP085_DEV_ADDR_W);
  i2c_write(BMP085_CTRL_REG_ADDR);
  i2c_write(BMP085_CTRL_REG_TEMP);
  i2c_stop();
  _delay_ms(5);
  return (int32_t) (bmp085_read16bit_data(BMP085_DATA_REG_N));
}
/*---------------------------------------------------------------------------*/
static int32_t
bmp085_read_uncomp_pressure(uint8_t mode)
{
  int32_t pressure;
  i2c_start(BMP085_DEV_ADDR_W);
  i2c_write(BMP085_CTRL_REG_ADDR);
  switch (mode) {
    case 0:
      i2c_write(BMP085_CTRL_REG_PRESS_0);
      i2c_stop();
      _delay_ms(5);
      break;
    case 1:
      i2c_write(BMP085_CTRL_REG_PRESS_1);
      i2c_stop();
      _delay_ms(8);
      break;
    case 2:
      i2c_write(BMP085_CTRL_REG_PRESS_2);
      i2c_stop();
      _delay_ms(14);
      break;
    case 3:
      i2c_write(BMP085_CTRL_REG_PRESS_3);
      i2c_stop();
      _delay_ms(26);
      break;
  }
  pressure = bmp085_read16bit_data(BMP085_DATA_REG_N);
  pressure = pressure << 8;
  pressure += bmp085_read8bit_data(BMP085_DATA_REG_X);
  return (pressure >> (8 - mode));
}
/*---------------------------------------------------------------------------*/
int16_t
bmp085_read_temperature(void)
{
  int32_t ut = 0;
  int32_t x1, x2, b5;

  ut = bmp085_read_uncomp_temperature();

  x1 = ((int32_t) ut - (int32_t) bmp085_coeff.ac6)
          * (int32_t) bmp085_coeff.ac5 >> 15;
  x2 = ((int32_t) bmp085_coeff.mc << 11) / (x1 + bmp085_coeff.md);
  b5 = x1 + x2;

  return (int16_t) ((b5 + 8) >> 4);
}
/*---------------------------------------------------------------------------*/
int32_t
bmp085_read_pressure(uint8_t mode)
{
  int32_t ut = 0, compt = 0;
  int32_t up = 0, compp = 0;

  int32_t x1, x2, b5, b6, x3, b3, p;
  uint32_t b4, b7;

  ut = bmp085_read_uncomp_temperature();
  up = bmp085_read_uncomp_pressure(mode);

  x1 = ((int32_t) ut - (int32_t) bmp085_coeff.ac6)
          * (int32_t) bmp085_coeff.ac5 >> 15;
  x2 = ((int32_t) bmp085_coeff.mc << 11) / (x1 + bmp085_coeff.md);
  b5 = x1 + x2;
  compt = (b5 + 8) >> 4;

  b6 = b5 - 4000;
  x1 = (bmp085_coeff.b2 * ((b6 * b6) >> 12)) >> 11;
  x2 = (bmp085_coeff.ac2 * b6) >> 11;
  x3 = x1 + x2;
  b3 = (((((int32_t) bmp085_coeff.ac1) * 4 + x3) << mode) + 2) >> 2;
  x1 = (bmp085_coeff.ac3 * b6) >> 13;
  x2 = (bmp085_coeff.b1 * ((b6 * b6) >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  b4 = (bmp085_coeff.ac4 * (uint32_t) (x3 + 32768)) >> 15;
  b7 = ((uint32_t) (up - b3) * (50000 >> mode));

  if (b7 < 0x80000000) {
    p = (b7 << 1) / b4;
  } else {
    p = (b7 / b4) << 1;
  }

  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  compp = p + ((x1 + x2 + 3791) >> 4);
  return compp;
}
/*---------------------------------------------------------------------------*/
static void
bmp085_read_calib_data(void)
{
  bmp085_coeff.ac1 = bmp085_read16bit_data(BMP085_AC1_ADDR);
  bmp085_coeff.ac2 = bmp085_read16bit_data(BMP085_AC2_ADDR);
  bmp085_coeff.ac3 = bmp085_read16bit_data(BMP085_AC3_ADDR);
  bmp085_coeff.ac4 = bmp085_read16bit_data(BMP085_AC4_ADDR);
  bmp085_coeff.ac5 = bmp085_read16bit_data(BMP085_AC5_ADDR);
  bmp085_coeff.ac6 = bmp085_read16bit_data(BMP085_AC6_ADDR);
  bmp085_coeff.b1 = bmp085_read16bit_data(BMP085_B1_ADDR);
  bmp085_coeff.b2 = bmp085_read16bit_data(BMP085_B2_ADDR);
  bmp085_coeff.mb = bmp085_read16bit_data(BMP085_MB_ADDR);
  bmp085_coeff.mc = bmp085_read16bit_data(BMP085_MC_ADDR);
  bmp085_coeff.md = bmp085_read16bit_data(BMP085_MD_ADDR);
}
/*---------------------------------------------------------------------------*/
static uint16_t
bmp085_read16bit_data(uint8_t addr)
{
  uint8_t msb = 0, lsb = 0;
  i2c_start(BMP085_DEV_ADDR_W);
  i2c_write(addr);
  i2c_rep_start(BMP085_DEV_ADDR_R);
  i2c_read_ack(&msb);
  i2c_read_nack(&lsb);
  i2c_stop();
  return (uint16_t) ((msb << 8) | lsb);
}
/*---------------------------------------------------------------------------*/
static uint8_t
bmp085_read8bit_data(uint8_t addr)
{
  uint8_t lsb = 0;
  i2c_start(BMP085_DEV_ADDR_W);
  i2c_write(addr);
  i2c_rep_start(BMP085_DEV_ADDR_R);
  i2c_read_nack(&lsb);
  i2c_stop();
  return lsb;
}

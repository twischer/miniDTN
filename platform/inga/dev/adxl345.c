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
 *      ADXL345 Accelerometer interface implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *      Enrico Jörns <joerns@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup adxl345_interface
 * @{
 */

#include "adxl345.h"

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define ADXL345_DEVICE_ID_DATA            0xE5
/*----------------------------------------------------------------------------*/

uint8_t bustype = 0;
int8_t
adxl345_available(void)
{
  uint8_t i, i2c_data = 0, mpsi_data = 0;
  
  mspi_chip_release(ADXL345_CS);
  mspi_init(ADXL345_CS, MSPI_MODE_3, MSPI_BAUD_MAX);
  
  for(i = 0; i <= 10; i++) {
    mspi_chip_select(ADXL345_CS);
    mspi_transceive(ADXL345_DEVICE_ID_REG);
    mpsi_data = mspi_transceive(MSPI_DUMMY_BYTE);
    mspi_chip_release(ADXL345_CS);

    i2c_init();
    i2c_start(ADXL345_I2C_ADDR_R);
    i2c_write(ADXL345_DEVICE_ID_REG);
    i2c_rep_start(ADXL345_I2C_ADDR_R);
    i2c_read_nack(&i2c_data);
    i2c_stop();
    printf("i2c_data = %d\n", i2c_data); 
    if((mpsi_data == ADXL345_DEVICE_ID_DATA) && (i2c_data != ADXL345_DEVICE_ID_DATA)) {
      bustype = MSPI;
      PRINTF("adxl345: is available via MSPI\n");
      return 1;
    }
    if((mpsi_data != ADXL345_DEVICE_ID_DATA) && (i2c_data == ADXL345_DEVICE_ID_DATA)) {
      bustype = I2C;
      PRINTF("adxl345: is available via I2C\n");
      return 1;
    }
 
    _delay_ms(10);
 }
  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
adxl345_init(void)
{

  if (!adxl345_available()) {
    return -1;
  }
  // full resolution, 2g range
  adxl345_write(ADXL345_DATA_FORMAT_REG, (1 << ADXL345_FULL_RES));
  // measuring enabled
  adxl345_write(ADXL345_POWER_CTL_REG, (1 << ADXL345_MEASURE));
  // output data rate: 100Hz
  adxl345_write(ADXL345_BW_RATE_REG, ADXL345_ODR_100HZ);

  PRINTF("adxl345: initialized\n");

  return 0;
}
/*----------------------------------------------------------------------------*/
void
adxl345_deinit(void)
{
  adxl345_set_powermode(ADXL345_PMODE_STANDBY);
}

/*----------------------------------------------------------------------------*/
void
adxl345_set_g_range(uint8_t range)
{
  uint8_t tmp_reg = adxl345_read(ADXL345_DATA_FORMAT_REG);
  adxl345_write(ADXL345_DATA_FORMAT_REG, (tmp_reg & 0xFC) | (range & 0x03));
}
/*----------------------------------------------------------------------------*/
void
adxl345_set_data_rate(uint8_t rate)
{
  uint8_t tmp_reg = adxl345_read(ADXL345_BW_RATE_REG);
  adxl345_write(ADXL345_BW_RATE_REG, (tmp_reg & 0xF0) | (rate & 0x0F));
}
/*----------------------------------------------------------------------------*/
void
adxl345_set_fifomode(uint8_t mode)
{
  uint8_t tmp_reg = adxl345_read(ADXL345_FIFO_CTL_REG);
  adxl345_write(ADXL345_FIFO_CTL_REG, (tmp_reg & 0x3F) | (mode & 0xC0));
}
/*----------------------------------------------------------------------------*/
void
adxl345_set_powermode(uint8_t mode)
{
  uint8_t tmp_reg = adxl345_read(ADXL345_POWER_CTL_REG);
  switch (mode) {
      // sleep mode with 1Hz readings
    case ADXL345_PMODE_SLEEP:
      tmp_reg &= 0xF8;
      tmp_reg |= (1 << ADXL345_SLEEP) | (0x3 << ADXL345_WAKEUP_L);
      break;
    case ADXL345_PMODE_WAKEUP:
      tmp_reg &= 0xF9;
      tmp_reg |= (1 << ADXL345_MEASURE);
      tmp_reg &= ~(1 << ADXL345_SLEEP);
      break;
    case ADXL345_PMODE_STANDBY:
      tmp_reg &= ~(1 << ADXL345_MEASURE);
      break;
  }
  adxl345_write(ADXL345_POWER_CTL_REG, tmp_reg);
}
/*----------------------------------------------------------------------------*/
uint8_t
adxl345_get_fifo_level()
{
  return adxl345_read(ADXL345_FIFO_STATUS_REG);
}
/*----------------------------------------------------------------------------*/
int16_t
adxl345_get_x(void)
{
  uint8_t byteLow;
  uint8_t byteHigh;
  byteLow = adxl345_read(ADXL345_OUTX_LOW_REG);
  byteHigh = adxl345_read(ADXL345_OUTX_HIGH_REG);

  return ((byteHigh << 8) + byteLow);
}
/*----------------------------------------------------------------------------*/
int16_t
adxl345_get_y(void)
{
  uint8_t byteLow;
  uint8_t byteHigh;
  byteLow = adxl345_read(ADXL345_OUTY_LOW_REG);
  byteHigh = adxl345_read(ADXL345_OUTY_HIGH_REG);
  return (byteHigh << 8) +byteLow;
}
/*----------------------------------------------------------------------------*/
int16_t
adxl345_get_z(void)
{
  uint8_t byteLow;
  uint8_t byteHigh;
  byteLow = adxl345_read(ADXL345_OUTZ_LOW_REG);
  byteHigh = adxl345_read(ADXL345_OUTZ_HIGH_REG);
  return (byteHigh << 8) +byteLow;
}
/*----------------------------------------------------------------------------*/
acc_data_t
adxl345_get(void)
{
  //TODO: if(bus=
  acc_data_t adxl345_data;
  uint8_t lsb = 0, msb = 0;
  mspi_chip_select(ADXL345_CS);
  mspi_transceive(ADXL345_OUTX_LOW_REG | 0xC0); // read, multiple
  lsb = mspi_transceive(MSPI_DUMMY_BYTE);
  msb = mspi_transceive(MSPI_DUMMY_BYTE);
  adxl345_data.x = (int16_t) ((msb << 8) + lsb);
  lsb = mspi_transceive(MSPI_DUMMY_BYTE);
  msb = mspi_transceive(MSPI_DUMMY_BYTE);
  adxl345_data.y = (int16_t) ((msb << 8) + lsb);
  lsb = mspi_transceive(MSPI_DUMMY_BYTE);
  msb = mspi_transceive(MSPI_DUMMY_BYTE);
  adxl345_data.z = (int16_t) ((msb << 8) + lsb);
  mspi_chip_release(ADXL345_CS);
  PRINTF("adxl345: x: %d, y: %d, z: %d\n", adxl345_data.x, adxl345_data.y, adxl345_data.z);
  return adxl345_data;
}
/*----------------------------------------------------------------------------*/
int8_t
adxl345_get_acceleration_fifo(acc_data_t* ret)
{
  uint8_t idx = 0;
  uint8_t fifolevel = adxl345_read(ADXL345_FIFO_STATUS_REG) & 0x3F;

  for (idx = 0; idx < fifolevel; idx++) {
    ret[idx] = adxl345_get();
  }

  return fifolevel;
}
/*----------------------------------------------------------------------------*/
void
adxl345_write(uint8_t reg, uint8_t data)
{
  reg &= 0x7F;

  if(bustype == MSPI) {
    mspi_chip_select(ADXL345_CS);
    mspi_transceive(reg);
    mspi_transceive(data);
    mspi_chip_release(ADXL345_CS);
  }
  else if(bustype == I2C) {
    i2c_start(ADXL345_I2C_ADDR_W);
    i2c_write(reg);
    i2c_rep_start(ADXL345_I2C_ADDR_W);
    i2c_write(data);
    i2c_stop();
  }
}
/*----------------------------------------------------------------------------*/
uint8_t
adxl345_read(uint8_t reg)
{
  uint8_t data;
  reg |= 0x80;
  
  if(bustype == MSPI) {
    mspi_chip_select(ADXL345_CS);
    mspi_transceive(reg);
    data = mspi_transceive(MSPI_DUMMY_BYTE);
    mspi_chip_release(ADXL345_CS);
  }
  else if(bustype == I2C) {
    i2c_start(ADXL345_I2C_ADDR_R);
    i2c_write(reg);
    i2c_rep_start(ADXL345_I2C_ADDR_R);
    i2c_read_nack(&data);
    i2c_stop();
  }

  return data;
}
/*----------------------------------------------------------------------------*/

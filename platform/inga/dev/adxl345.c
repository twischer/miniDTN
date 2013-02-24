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
#define ADXL345_BW_RATE_DEFAULT_DATA      0x0A
#define ADXL345_POWER_CTL_DEFAULT_DATA		0x08
#define ADXL345_DATA_FORMAT_DEFAULT_DATA  0x00
#define ADXL345_DEVICE_ID_DATA            0xE5
/*----------------------------------------------------------------------------*/
int8_t
adxl345_init(void)
{
  mspi_chip_release(ADXL345_CS);
  mspi_init(ADXL345_CS, MSPI_MODE_3, MSPI_BAUD_MAX);

  adxl345_write(ADXL345_DATA_FORMAT_REG, ADXL345_DATA_FORMAT_DEFAULT_DATA);
  adxl345_write(ADXL345_POWER_CTL_REG, ADXL345_POWER_CTL_DEFAULT_DATA);
  adxl345_write(ADXL345_BW_RATE_REG, ADXL345_BW_RATE_DEFAULT_DATA);

  return adxl345_ready() ? 0 : -1;
}
/*----------------------------------------------------------------------------*/
int8_t
adxl345_ready(void)
{
  uint8_t i = 0;
  printf("adxl345_ready: %u\n", adxl345_read(ADXL345_DEVICE_ID_REG));
  while (adxl345_read(ADXL345_DEVICE_ID_REG) != ADXL345_DEVICE_ID_DATA) {
    _delay_ms(10);
    if (i++ > 10) {
      return 0;
    }
  }
  return 1;
}
/*----------------------------------------------------------------------------*/
inline void
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
  //  adxl345_write(ADXL345_POWER_CTL_REG, (tmp_reg & 0x??) | (mode & 0x??));
}
/*----------------------------------------------------------------------------*/
int16_t
adxl345_get_x_acceleration(void)
{
  uint8_t byteLow;
  uint8_t byteHigh;
  byteLow = adxl345_read(ADXL345_OUTX_LOW_REG);
  byteHigh = adxl345_read(ADXL345_OUTX_HIGH_REG);

  return ((byteHigh << 8) + byteLow);
}
/*----------------------------------------------------------------------------*/
int16_t
adxl345_get_y_acceleration(void)
{
  uint8_t byteLow;
  uint8_t byteHigh;
  byteLow = adxl345_read(ADXL345_OUTY_LOW_REG);
  byteHigh = adxl345_read(ADXL345_OUTY_HIGH_REG);
  return (byteHigh << 8) +byteLow;
}
/*----------------------------------------------------------------------------*/
int16_t
adxl345_get_z_acceleration(void)
{
  uint8_t byteLow;
  uint8_t byteHigh;
  byteLow = adxl345_read(ADXL345_OUTZ_LOW_REG);
  byteHigh = adxl345_read(ADXL345_OUTZ_HIGH_REG);
  return (byteHigh << 8) +byteLow;
}
/*----------------------------------------------------------------------------*/
acc_data_t
adxl345_get_acceleration(void)
{
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
  return adxl345_data;
}
/*----------------------------------------------------------------------------*/
int8_t
adxl345_get_acceleration_fifo(acc_data_t* ret)
{
  uint8_t idx = 0;
  uint8_t fifolevel = adxl345_read(ADXL345_FIFO_STATUS_REG) & 0x3F;

  for (idx = 0; idx < fifolevel; idx++) {
    ret[idx] = adxl345_get_acceleration();
  }

  return fifolevel;
}
/*----------------------------------------------------------------------------*/
void
adxl345_write(uint8_t reg, uint8_t data)
{
  mspi_chip_select(ADXL345_CS);
  reg &= 0x7F;
  mspi_transceive(reg);
  mspi_transceive(data);
  mspi_chip_release(ADXL345_CS);
}
/*----------------------------------------------------------------------------*/
uint8_t
adxl345_read(uint8_t reg)
{
  uint8_t data;
  mspi_chip_select(ADXL345_CS);
  reg |= 0x80;
  mspi_transceive(reg);
  data = mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_chip_release(ADXL345_CS);
  return data;
}
/*----------------------------------------------------------------------------*/

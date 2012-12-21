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
 * \addtogroup L3G4200D_interface
 * @{
 */

/**
 * \file
 *      ST L3G4200D 3-axis Gyroscope interface implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#include "l3g4200d.h"
int8_t
l3g4200d_init(void) {
  uint8_t i = 0;

  i2c_init();
  while (l3g4200d_read8bit(L3G4000D_WHO_I_AM_REG) != 0xD3) {
    _delay_ms(10);
    if (i++ > 10) {
      return -1;
    }
  }
  l3g4200d_write8bit(L3G4000D_CTRL_REG1, 0x0F);
  return 0;
}
/*---------------------------------------------------------------------------*/
angle_data_t
l3g4200d_get_angle(void) {
  angle_data_t this_angle;

  this_angle.angle_x_value = l3g4200d_get_x_angle();
  this_angle.angle_y_value = l3g4200d_get_y_angle();
  this_angle.angle_z_value = l3g4200d_get_z_angle();
  return this_angle;
}
/*---------------------------------------------------------------------------*/
uint16_t
l3g4200d_get_x_angle(void) {
  return l3g4200d_read16bit(L3G4000D_OUT_X_L);
}
/*---------------------------------------------------------------------------*/
uint16_t
l3g4200d_get_y_angle(void) {
  return l3g4200d_read16bit(L3G4000D_OUT_Y_L);
}
/*---------------------------------------------------------------------------*/
uint16_t
l3g4200d_get_z_angle(void) {
  return l3g4200d_read16bit(L3G4000D_OUT_Z_L);
}
/*---------------------------------------------------------------------------*/
uint8_t
l3g4200d_get_temp(void) {
  return l3g4200d_read8bit(L3G4000D_OUT_TEMP);
}
/*---------------------------------------------------------------------------*/
uint8_t
l3g4200d_read8bit(uint8_t addr) {
  uint8_t lsb = 0;
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write(addr);
  i2c_rep_start(L3G4200D_DEV_ADDR_R);
  i2c_read_nack(&lsb);
  i2c_stop();
  return lsb;
}
/*---------------------------------------------------------------------------*/
uint16_t
l3g4200d_read16bit(uint8_t addr) {
  uint8_t lsb = 0, msb = 0;
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write((addr | 0x80));
  i2c_rep_start(L3G4200D_DEV_ADDR_R);
  i2c_read_ack(&lsb);
  i2c_read_nack(&msb);
  i2c_stop();
  return (uint16_t) ((msb << 8) + lsb);
}
/*---------------------------------------------------------------------------*/
void
l3g4200d_write8bit(uint8_t addr, uint8_t data) {
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write(addr);
  i2c_write(data);
  i2c_stop();
}

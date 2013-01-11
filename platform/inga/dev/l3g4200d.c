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
 *      ST L3G4200D 3-axis Gyroscope interface implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup l3g4200d_interface
 * @{
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

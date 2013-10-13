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
 *      Enrico Jörns <e.joerns@tu-bs.de>
 * 
 */

/**
 * \addtogroup l3g4200d_interface
 * @{
 */

#include "l3g4200d.h"

#define L3G4200D_DPSDIV_250G	35
#define L3G4200D_DPSDIV_500G	70
#define L3G4200D_DPSDIV_2000G	280

uint16_t l3g4200d_dps_scale;

#define TEMP_OFFSET     0
// converts raw value to tempeartue, offset might need to be adjusted
#define raw_to_temp(a)  (25 - TEMP_OFFSET - ((int8_t) a))
//*----------------------------------------------------------------------------*/
int8_t
l3g4200d_available(void)
{
  uint8_t tries = 0;

  i2c_init();
  while (l3g4200d_read8bit(L3G4200D_WHO_AM_I_REG) != L3G4200D_WHO_AM_I) {
    _delay_ms(10);
    if (tries++ > 10) {
      return 0;
    }
  }
  return 1;
}
/*----------------------------------------------------------------------------*/
int8_t
l3g4200d_init(void)
{
  
  if (!l3g4200d_available()) {
    return -1;
  }
  // disable power down mode, enable axis
  l3g4200d_write8bit(L3G4200D_CTRL_REG1, 
      (1 << L3G4200D_PD) 
      | (1 << L3G4200D_ZEN) 
      | (1 << L3G4200D_YEN) 
      | (1 << L3G4200D_XEN));
  l3g4200d_set_dps(L3G4200D_250DPS);
  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
l3g4200d_deinit(void)
{
  // enable power down mode, disable axis
  l3g4200d_write8bit(L3G4200D_CTRL_REG1, 0x00);
  return 0;
}
/*----------------------------------------------------------------------------*/
uint8_t
l3g4200d_set_dps(uint8_t set)
{
  uint8_t tmpref = l3g4200d_read8bit(L3G4200D_CTRL_REG4);
  l3g4200d_write8bit(L3G4200D_CTRL_REG4, (tmpref & 0xCF) | set);
  // set dpsdiv to 4*(datasheet_value)
  switch (set) {
    case L3G4200D_250DPS:
      l3g4200d_dps_scale = L3G4200D_DPSDIV_250G;
      break;
    case L3G4200D_500DPS:
      l3g4200d_dps_scale = L3G4200D_DPSDIV_500G;
      break;
    case L3G4200D_2000DPS:
      l3g4200d_dps_scale = L3G4200D_DPSDIV_2000G;
      break;
  }
  // verify
  tmpref = l3g4200d_read8bit(L3G4200D_CTRL_REG4);
  return (tmpref & 0x30) ^ (set);
}
/*----------------------------------------------------------------------------*/
uint8_t
l3g4200d_set_data_rate(uint8_t set)
{
  // read
  uint8_t tmpref = l3g4200d_read8bit(L3G4200D_CTRL_REG1);
  // write
  l3g4200d_write8bit(L3G4200D_CTRL_REG1, (tmpref & 0x3F) | set);
  // verify
  tmpref = l3g4200d_read8bit(L3G4200D_CTRL_REG1);
  return (tmpref & 0xC0) ^ (set);
}
/*----------------------------------------------------------------------------*/
inline void
l3g4200d_set_fifomode(uint8_t set)
{
  uint8_t tmpref = l3g4200d_read8bit(L3G4200D_FIFO_CTRL_REG);
  l3g4200d_write8bit(L3G4200D_FIFO_CTRL_REG, (tmpref & 0x1F) | set);
}
/*----------------------------------------------------------------------------*/
inline void
l3g4200d_fifo_enable()
{
  l3g4200d_write8bit(L3G4200D_CTRL_REG5, (1 << L3G4200D_FIFO_EN));
}
/*----------------------------------------------------------------------------*/
int8_t
l3g4200d_fifo_overrun(void)
{
  return (l3g4200d_read8bit(L3G4200D_FIFO_SRC_REG) & (1 << L3G4200D_OVRN));
}
/*----------------------------------------------------------------------------*/
angle_data_t
l3g4200d_get_angle(void)
{
  angle_data_t ret_data;
  uint8_t lsb = 0, msb = 0;
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write((L3G4200D_OUT_X_L | 0x80));
  i2c_rep_start(L3G4200D_DEV_ADDR_R);
  i2c_read_ack(&lsb);
  i2c_read_ack(&msb);
  ret_data.x = (uint16_t) ((msb << 8) + lsb);
  i2c_read_ack(&lsb);
  i2c_read_ack(&msb);
  ret_data.y = (uint16_t) ((msb << 8) + lsb);
  i2c_read_ack(&lsb);
  i2c_read_nack(&msb);
  ret_data.z = (uint16_t) ((msb << 8) + lsb);
  i2c_stop();
  return ret_data;
}
/*----------------------------------------------------------------------------*/
int8_t
l3g4200d_get_angle_fifo(angle_data_t* ret)
{
  uint8_t idx = 0;
  uint8_t fifolevel = l3g4200d_read8bit(L3G4200D_FIFO_SRC_REG) & 0x1F;

  for (idx = 0; idx < fifolevel; idx++) {
    ret[idx] = l3g4200d_get_angle();
  }

  return fifolevel;
}
/*----------------------------------------------------------------------------*/
int16_t
l3g4200d_get_x_angle(void)
{
  return l3g4200d_read16bit(L3G4200D_OUT_X_L);
}
/*----------------------------------------------------------------------------*/
int16_t
l3g4200d_get_y_angle(void)
{
  return l3g4200d_read16bit(L3G4200D_OUT_Y_L);
}
/*----------------------------------------------------------------------------*/
int16_t
l3g4200d_get_z_angle(void)
{
  return l3g4200d_read16bit(L3G4200D_OUT_Z_L);
}
/*----------------------------------------------------------------------------*/

int8_t
l3g4200d_get_temp(void)
{
  return raw_to_temp(l3g4200d_read8bit(L3G4200D_OUT_TEMP));
}
/*----------------------------------------------------------------------------*/
uint8_t
l3g4200d_read8bit(uint8_t addr)
{
  uint8_t lsb = 0;
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write(addr);
  i2c_rep_start(L3G4200D_DEV_ADDR_R);
  i2c_read_nack(&lsb);
  i2c_stop();
  return lsb;
}
/*----------------------------------------------------------------------------*/
uint16_t
l3g4200d_read16bit(uint8_t addr)
{
  uint8_t lsb = 0, msb = 0;
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write((addr | 0x80));
  i2c_rep_start(L3G4200D_DEV_ADDR_R);
  i2c_read_ack(&lsb);
  i2c_read_nack(&msb);
  i2c_stop();
  return (uint16_t) ((msb << 8) + lsb);
}
/*----------------------------------------------------------------------------*/
void
l3g4200d_write8bit(uint8_t addr, uint8_t data)
{
  i2c_start(L3G4200D_DEV_ADDR_W);
  i2c_write(addr);
  i2c_write(data);
  i2c_stop();
}

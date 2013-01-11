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
 *      I2C driver implementation
 * \author
 *      Stephan Rottman <rottmann@ibr.cs.tu-bs.de>
 *      modified by Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_bus_driver
 * @{
 */

/**
 * \addtogroup i2c_driver
 * @{
 */

#include "i2c.h"

#ifndef PRR
#define PRR PRR0
#endif
void
i2c_init(void) {
  TWSR &= ~((1 << TWPS1) | (1 << TWPS0));
#if I2C_HIGH_SPEED
  TWBR = 2; //400KHz
#else
  TWBR = 32; //100KHz
#endif

}
/*----------------------------------------------------------------------------*/
int8_t
_i2c_start(uint8_t addr, uint8_t rep) {
  PRR &= ~(1 << PRTWI);
  i2c_init();
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)));
  if ((TWSR & 0xF8) != ((!rep) ? I2C_START : I2C_REP_START))
    return -1;

  TWDR = addr;
  TWCR = (1 << TWINT) | (1 << TWEN);

  while (!(TWCR & (1 << TWINT)));
  if (!(((TWSR & 0xF8) == I2C_MT_SLA_ACK) || ((TWSR & 0xF8) == I2C_MR_SLA_ACK)))
    return -2;

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
i2c_start(uint8_t addr) {
  return _i2c_start(addr, 0);
}
/*----------------------------------------------------------------------------*/
int8_t
i2c_rep_start(uint8_t addr) {
  return _i2c_start(addr, 1);
}
/*----------------------------------------------------------------------------*/
int8_t
i2c_write(uint8_t data) {
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);

  while (!(TWCR & (1 << TWINT)));
  if ((TWSR & 0xF8) != I2C_MT_DATA_ACK)
    return -1;

  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
i2c_read(uint8_t *data, uint8_t ack) {
  uint16_t i = 0;
  TWCR = (1 << TWINT) | (1 << TWEN) | (ack ? (1 << TWEA) : 0);
  while (!(TWCR & (1 << TWINT))) {
    if (i++ > 800) {
      return -1;
    }
  }
  if ((TWSR & 0xF8) != (ack ? I2C_MR_DATA_ACK : I2C_MR_DATA_NACK))
    return -1;
  *data = TWDR;
  return 0;
}
/*----------------------------------------------------------------------------*/
int8_t
i2c_read_ack(uint8_t *data) {
  return i2c_read(data, 1);
}
/*----------------------------------------------------------------------------*/
int8_t
i2c_read_nack(uint8_t *data) {
  return i2c_read(data, 0);
}
/*----------------------------------------------------------------------------*/
void
i2c_stop(void) {

  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
  while (TWCR & (1 << TWSTO));

  TWCR &= ~(TWEN);

  PRR |= (1 << PRTWI);
  DDRC &= ~((1 << PC0) | (1 << PC1));
  PORTC |= ((1 << PC0) | (1 << PC1));

}

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
 *		I2C driver definitions
 * \author
 *      Stephan Rottman <rottmann@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_bus_driver
 * @{
 */

/**
 * \defgroup i2c_driver I2C-Bus Driver
 * @{
 */

#include <avr/io.h>

#ifndef I2CDRV_H_
#define I2CDRV_H_


#define I2C_HIGH_SPEED	 0


#define I2C_START        0x08
#define I2C_REP_START    0x10
#define I2C_MT_SLA_ACK   0x18
#define I2C_MT_DATA_ACK  0x28
#define I2C_MR_SLA_ACK   0x40
#define I2C_MR_DATA_ACK  0x50
#define I2C_MR_DATA_NACK 0x58


void i2c_init(void);
void i2c_stop(void);

int8_t i2c_start(uint8_t addr);
int8_t i2c_rep_start(uint8_t addr);
int8_t i2c_write(uint8_t data);
int8_t i2c_read(uint8_t *data, uint8_t ack);
int8_t i2c_read_ack(uint8_t *data);
int8_t i2c_read_nack(uint8_t *data);


#endif /* I2CDRV_H_ */

/** @} */
/** @} */

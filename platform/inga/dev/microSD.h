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
 *		MicroSD Card interface definitions
 * \author
 * 		Original Source Code:
 * 		Ulrich Radig
 * 		Modified by:
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_device_driver
 * @{
 */

/**
 * \defgroup microSD_interface MicroSD Card Interface
 *
 *	\note This sd-card interface is based on the work of Ulrich Radig
 *	      "Connect AVR to MMC/SD" and was a little bit modified plus
 *	      adapted to the mspi-drv
 *
 * <p> Huge memory space is always nice to have, because you don't have to
 * decide which data has to be stored, just store everything! Furthermore you
 * can buffer your data, whenever wireless connection breaks down.
 * An easy and modular way is a SD card. For smaller applications (like
 * sensor nodes), the tiny version "microSD" would be the best choice. Both, the
 * "big" SD-Cards and the microSD-Cards can be interfaced by SPI and use the same
 * Instruction set, so this interface can be used for both SD-Card types.</p>
 * @{
 */

#ifndef FLASH_MICROSD_H_
#define FLASH_MICROSD_H_

#include "../dev/mspi.h"
#include <stdio.h>
#include <util/delay.h>

/*!
 * SPI device order. The chip select number where the
 * microSD-Card is connected to the BCD-decimal decoder
 */
#define MICRO_SD_CS 					5

#define MICRO_SD_PWR_PORT				PORTA
#define MICRO_SD_PWR_PORT_DDR			DDRA
#define MICRO_SD_PWR_PIN				PORTA4

/**
 * \brief Powers on and initialize the microSD / SD-Card
 *
 * \return <ul>
 *  		<li> 0 : SD-Card was initialized without an error
 *  		<li> 1 : CMD0 failure!
 *  		<li> 2 : CMD1 failure!
 * 		   </ul>
 */
uint8_t microSD_init(void);

uint8_t microSD_deinit(void);

/**
 * \brief This function will read one block (512Byte) of the SD-Card.
 *
 * \param addr Block address
 * \param *buffer Pointer to a block buffer
 *
 * \return <ul>
 *  		<li> 0 : SD-Card block read was successful
 *  		<li> 1 : CMD17 failure!
 * 		   </ul>
 */
uint8_t microSD_read_block(uint32_t addr, uint8_t *buffer);

/**
 * \brief This function will write one block (512Byte) of the SD-Card.
 *
 * \param addr Block address
 * \param *buffer Pointer to a block buffer
 *
 * \return <ul>
 *  		<li> 0 : SD-Card block write was successful
 *  		<li> 1 : CMD24 failure!
 * 		   </ul>
 */
uint8_t microSD_write_block(uint32_t addr, uint8_t *buffer);

/**
 * \brief This function sends a command via SPI to the SD-Card. An SPI
 * command consists off 6 bytes
 *
 * \param *cmd Pointer to the command array
 *
 * \return <ul>
 *  		<li> 1 : Acknowledge
 *  		<li> other failure code
 * 		   </ul>
 */
uint8_t microSD_write_cmd(uint8_t *cmd);

/** @} */ // l3g4200d_interface
/** @} */ // inga_device_driver

#endif /* FLASH_MICROSD_H_ */

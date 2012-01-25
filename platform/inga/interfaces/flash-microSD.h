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
 * \addtogroup Device Interfaces
 * @{
 *
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
 *
 * @{
 *
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

#ifndef FLASH_MICROSD_H_
#define FLASH_MICROSD_H_

#include "../drv/mspi-drv.h"
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
 *			<li> 3 : Failure reading the CSD!
 *			<li> 4 : CMD8 failure!
 *			<li> 5 : CMD16 failure!
 *			<li> 6 : ACMD41 failure!
 *			<li> 7 : CMD58 failure!
 * 		   </ul>
 */
uint8_t microSD_init(void);

/**
 * \brief This function will read the CSD (16 Bytes) of the SD-Card.
 *
 * \param *buffer Pointer to a block buffer, MUST hold at least 16 Bytes.
 *
 * \return <ul>
 *  		<li> 0 : SD-Card CSD read was successful
 *  		<li> 1 : CMD9 failure!
 * 		   </ul>
 */
uint8_t microSD_read_csd( uint8_t *buffer );

/**
 * \brief This function returns the number of bytes in one block.
 *
 * Mainly used to calculated size of the SD-Card together with
 * microSD_get_card_block_count().
 *
 *
 * \return Number of bytes per block on the SD-Card
 */
uint16_t microSD_get_block_size();

/**
 * \brief This function returns the number of blocks on the SD-Card.
 *
 *
 * \return Number of blocks on the SD-Card
 */
uint32_t microSD_get_card_block_count();

/**
 * \brief This function indicates if a card is a SDSC or SDHC/SDXC card.
 *
 * microSD_init() must be called beforehand and be successful before this
 * functions return value has any meaning.
 *
 * \return Not 0 if the card is SDSC and 0 if SDHC/SDXC
 */
uint8_t microSD_is_SDSC();

uint8_t microSD_deinit(void);

/**
 * \brief This function will read one block (512, 1024, 2048 or 4096Byte) of the SD-Card.
 *
 * \param addr Block address
 * \param *buffer Pointer to a block buffer (needs to be as long es microSD_get_block_size()).
 *
 * \return <ul>
 *  		<li> 0 : SD-Card block read was successful
 *  		<li> 1 : CMD17 failure!
 * 		   </ul>
 */
uint8_t microSD_read_block(uint32_t addr, uint8_t *buffer);

/**
 * \brief This function will write one block (512, 1024, 2048 or 4096Byte) of the SD-Card.
 *
 * \param addr Block address
 * \param *buffer Pointer to a block buffer (needs to be as long es microSD_get_block_size()).
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
 * \param *resp Pointer to the response array. Only needed for responses other than R1. May me NULL if response is R1. Otherwise resp must be long enough for the response (only R3 and R7 are supported yet) and the first byte of the response array must indicate the response that is expected. For Example the first byte should be 0x07 if response type R7 is expected.
 *
 * \return R1 response byte or 0xFF in case of read/write timeout
 */
uint8_t microSD_write_cmd(uint8_t *cmd, uint8_t *resp );
uint16_t microSD_data_crc( uint8_t *data );
uint8_t microSD_set_CRC( uint8_t enable );





#endif /* FLASH_MICROSD_H_ */

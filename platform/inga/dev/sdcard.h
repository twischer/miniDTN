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
 *		SD Card interface definitions
 * \author
 *              Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *		Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 *              Enrico Joerns <joerns@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_device_driver
 * @{
 */

/**
 * \defgroup sdcard_interface MicroSD Card Interface
 *
 * This driver provides the following main features:
 *
 * - single block read
 * - single block write
 * - multi block write
 *
 * Note that multiple bock write is faster than single block write
 * but only writes sequential block numbers
 *
 * \note CRC functionality is not fully implemented thus it sould not be used yet.
 *
 * \author
 *              Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *		Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 *		Enrico Joerns <joerns@ibr.cs.tu-bs.de>
 * @{
 */

#ifndef INGA_SDCARD_H
#define INGA_SDCARD_H

#include "mspi.h"
#include <stdio.h>

/*!
 * SPI device order. The chip select number where the
 * sdcard-Card is connected to the BCD-decimal decoder
 */
#define MICRO_SD_CS 					5

/**
 * \name Interface return codes
 * \{ */
/** Successfully completed operation */
#define SDCARD_SUCCESS            0
/** Indicates the host cannot handle this card,
 * maybe due to rejected voltage range */
#define SDCARD_REJECTED           2
/** Timeout while trying to send command */
#define SDCARD_CMD_TIMEOUT        4
/** Card sent an error response */
#define SDCARD_CMD_ERROR          3
/** Card did not send a data start byte to indicate beginning of a data block */
#define SDCARD_DATA_TIMEOUT       6
/** Card returned error when trying to read or write data.
 * Obtained from data response (write) or data error response (read) token */
#define SDCARD_DATA_ERROR         7
/** Busy waiting timed out (card held down data line too long) */
#define SDCARD_BUSY_TIMEOUT       8
/** Failed reading CSD register */
#define SDCARD_CSD_ERROR          10
/** \} */

#define SDCARD_WRITE_COMMAND_ERROR  1
#define SDCARD_WRITE_DATA_ERROR     2

#define SDCARD_ERASE_START_ERROR    1
#define SDCARD_ERASE_END_ERR        2

/**
 * \brief Initializes the SD Card
 *
 * \retval SDCARD_SUCCESS SD-Card was initialized without an error
 * \retval SDCARD_CMD_ERROR
 * \retval SDCARD_CMD_TIMEOUT 
 * \retval SDCARD_REJECTED
 * \retval SDCARD_CSD_ERROR
 */
uint8_t sdcard_init(void);

/**
 * \brief This function will read the CSD (16 Bytes) of the SD-Card.
 *
 * \param *buffer Pointer to a block buffer, MUST hold at least 16 Bytes.
 *
 * \retval SDCARD_SUCCESS SD-Card CSD read was successful
 * \retval SDCARD_CMD_ERROR CMD9 failure.
 * \retval SDCARD_DATA_TIMEOUT
 * \retval SDCARD_DATA_ERROR
 */
uint8_t sdcard_read_csd(uint8_t *buffer);

/**
 * Returns the size of one block in bytes.
 *
 * Mainly used to calculated size of the SD-Card together with
 * sdcard_get_card_block_count().
 *
 * \note It's fixed at 512 Bytes.
 *
 * \return Number of Bytes in one Block.
 */
uint16_t sdcard_get_block_size();

/**
 * \brief This function indicates if a card is a SDSC or SDHC/SDXC card.
 *
 * \note sdcard_init() must be called beforehand and be successful before this
 * functions return value has any meaning.
 *
 * \return Not 0 if the card is SDSC and 0 if SDHC/SDXC
 */
uint8_t sdcard_is_SDSC();

/**
 * Turns crc capabilities of the card on or off.
 *
 * \note Does not work properly at this time. (FIXME)
 * \param enable 0 if CRC should be disabled, 1 if it should be enabled
 * \return 0 on success, !=0 otherwise
 */
uint8_t sdcard_set_CRC(uint8_t enable);

/**
 * Returns card size
 * @return card size in bytes
 */
uint64_t sdcard_get_card_size();

/**
 * @return number of blocks
 */
uint32_t sdcard_get_block_num();

/**
 * @note Not implemented
 */
uint8_t sdcard_erase_blocks(uint32_t startaddr, uint32_t endaddr);

/**
 * \brief This function will read one block (512, 1024, 2048 or 4096Byte) of the SD-Card.
 *
 * \param addr Block address
 * \param *buffer Pointer to a block buffer (needs to be as long as sdcard_get_block_size()).
 *
 * \retval SDCARD_SUCCESS SD-Card block read was successful
 * \retval SDCARD_CMD_ERROR CMD17 failure
 * \retval SDCARD_BUSY_TIMEOUT
 * \retval SDCARD_DATA_TIMEOUT
 * \retval SDCARD_DATA_ERROR
 */
uint8_t sdcard_read_block(uint32_t addr, uint8_t *buffer);

/**
 * \brief This function will write one block (512, 1024, 2048 or 4096Byte) of the SD-Card.
 *
 * \param addr Block address
 * \param *buffer Pointer to a block buffer (needs to be as long as sdcard_get_block_size()).
 *
 * \retval SDCARD_SUCCESS SD-Card block write was successful
 * \retval SDCARD_CMD_ERROR CMD24 failure
 * \retval SDCARD_BUSY_TIMEOUT
 * \retval SDCARD_DATA_ERROR
 */
uint8_t sdcard_write_block(uint32_t addr, uint8_t *buffer);

/**
 * \brief Prepares to write multiple blocks sequentially.
 *
 * \param addr Address of first block
 * \param num_blocks Number of blocks that should be written (0 means not known yet).
 *        Givin a number here could speed up writing due to possible sector pre-erase
 * \retval SDCARD_SUCCESS Starting mutli lock write was successful
 * \retval SDCARD_CMD_ERROR CMD25 failure
 * \retval SDCARD_BUSY_TIMEOUT
 */
uint8_t sdcard_write_multi_block_start(uint32_t addr, uint32_t num_blocks);

/**
 * \brief Writes single of multiple sequental blocks.
 *
 * \param buffer
 * \retval SDCARD_SUCCESS Successfully wrote block
 * \retval SDCARD_BUSY_TIMEOUT
 * \retval SDCARD_DATA_ERROR
 */
uint8_t sdcard_write_multi_block_next(uint8_t *buffer);

/**
 * \brief Stops multiple block write.
 * 
 * \retval SDCARD_SUCCESS successfull
 * \retval SDCARD_BUSY_TIMEOUT
 */
uint8_t sdcard_write_multi_block_stop();


/**
 * \brief This function sends a command via SPI to the SD-Card. An SPI
 * command consists off 6 bytes
 *
 * \param cmd Command number to send
 * \param *arg pointer to 32bit argument (addresses etc.)
 * \param *resp Pointer to the response array. Only needed for responses other than R1. May be NULL if response is R1. Otherwise resp must be long enough for the response (only R3 and R7 are supported yet) and the first byte of the response array must indicate the response that is expected. For Example the first byte should be 0x07 if response type R7 is expected.
 *
 * \return R1 response byte or 0xFF in case of read/write timeout
 */
uint8_t sdcard_write_cmd(uint8_t cmd, uint32_t *arg, uint8_t *resp);

/**
 * \brief This function calculates the CRC16 for a 512 Byte data block.
 *
 * \param *data The array containing the data to be send. Must be 512 Bytes long.
 * \return The CRC16 code for the given data.
 */
uint16_t sdcard_data_crc(uint8_t *data);


/** @} */ // sdcard_interface
/** @} */ // inga_device_driver

#endif /* INGA_SDCARD_H */

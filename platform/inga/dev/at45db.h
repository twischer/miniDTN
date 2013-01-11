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
 *		Atmel Flash EEPROM AT45DB interface definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_device_driver
 * @{
 */

/**
 * \defgroup AT45DB_interface Atmel Flash EEPROM AT45DB interface
 *
 * <p>A fixed Flash EEPROM is always good to store some data. Furthermore
 * in this project environment, the AT45DBxx1 will be used as a hardware
 * interface between the boot section and the application section.</p>
 *
 * \note
 * The function of the AT45DBxx1 is a little bit different compared to
 * an SD-Card. The basic idea of the SD-Card is, to store one block
 * (512Byte) into a local buffer an transfer the whole block via SPI
 * to the SD-Card.
 * The AT45DBxx1 has two page buffer, where the data can be collected by
 * sending a byte via SPI to one of these buffers. If a buffer is filled,
 * only a command is necessary and the AT45DBxx1 will copy the buffer into
 * the flash section. To avoid latency, it is possible (and implemented)
 * to switch between the page buffers.
 * @{
 *
 */

#ifndef FLASHAT45DB_H_
#define FLASHAT45DB_H_

#include "../dev/mspi.h"
#include <stdio.h>
#include <util/delay.h>

/*!
 * SPI device order. The chip select number where the
 * AT45DBxx1 Flash EEPROM is connected to the BCD-decimal
 * decoder
 */
#define AT45DB_CS 					1

/*!
 * Status Register Address. Bit 7 signalizes if the device is
 * busy.
 * <ul>
 * <li> 1 : not busy
 * <li> 0 : busy
 * </ul>
 */
#define AT45DB_STATUS_REG			0xD7

/*!
 * Block Erase Opcode
 */
#define AT45DB_BLOCK_ERASE			0x50
/*!
 * Page Erase Opcode
 */
#define AT45DB_PAGE_ERASE			0x81
/*!
 * Main Memory Page Program Buffer 1
 */
#define AT45DB_PAGE_PROGRAM_1		0x82
/*!
 * Main Memory Page Program Buffer 2
 */
#define AT45DB_PAGE_PROGRAM_2		0x85

/*!
 * Write byte(s) to buffer 1 opcode
 */
#define AT45DB_BUFFER_1				0x84
/*!
 * Write byte(s) to buffer 2 opcode
 */
#define AT45DB_BUFFER_2				0x87

/*!
 * Copy Buffer 1 to page Opcode
 */
#define AT45DB_BUF_1_TO_PAGE		0x83 //0x88 without auto erase
/*!
 * Copy Buffer 2 to page Opcode
 */
#define AT45DB_BUF_2_TO_PAGE		0x86 //0x89 without auto erase
/*!
 * Read direct from Flash EEPROM page Opcode
 */
#define AT45DB_PAGE_READ			0xD2
/*!
 * Transfer page to buffer 2 Opcode
 * \note Only Buffer 2 is used to readout a page, because the read
 * respectively transfer latency is only about 200us
 *
 */
#define AT45DB_PAGE_TO_BUF			0x55 //use buffer 2
/*!
 * Read buffer 2 opcode
 * \note Only Buffer 2 is used to readout a page, because the read
 * respectively transfer latency is only about 200us
 */
#define AT45DB_READ_BUFFER  		0xD6


/*!
 * This typedef manages the buffer switching, to perform
 * the write operation
 */
typedef struct{
/*!
 * Holds the active buffer
 *  <ul>
 * <li> 0 : Active Buffer = Buffer 1
 * <li> 1 : Active Buffer = Buffer 2
 * </ul>
 */
	volatile uint8_t active_buffer;
/*!
 * The specific "byte(s) to buffer" opcode for buffer 1
 * and buffer 2
 */
	volatile uint8_t buffer_addr[2];
/*!
 * The specific "buffer to page" opcode for buffer 1 and
 * buffer 2
 */
	volatile uint8_t buf_to_page_addr[2];

/*!
 * Main Memory Page Program (Erase Page + Reprogram directly in one operation)
 */
	volatile uint8_t page_program[2];
}bufmgr_t;

/*!
 * Buffer manager allows it to improve write times, by switching
 * the dual buffer and parallelize flash write operations. (e.g. Write
 * to buffer 1 while buffer 2 is transfered to flash EEPROM)
 */
static bufmgr_t buffer_mgr;

/**
 * \brief Initialize the AT45DBxx1 Flash EEPROM
 *
 *\return 	<ul>
 *  		<li> 0 at45db available
 *  		<li> -1 at45db not available
 * 		 	</ul>
 *
 * \note No special settings are necessary to initialize
 * the Flash memory. The standard data flash page size is
 * 528Byte e.g. AT45DB161
 */
int8_t at45db_init(void);

/**
 * \brief This function erases the whole chip
 *
 * \note The time to erase the whole chip can take
 * up to 20sec!
 */
void at45db_erase_chip(void);

/**
 * \brief This function erases one block (4 Kbytes)
 *
 * \param addr block address e.g. AT45DB161 (0 ... 511)
 *
 * \note The time to erase one block can take
 * up to 45ms - 100ms!
 */
void at45db_erase_block(uint16_t addr);

/**
 * \brief This function erases one page e.g. AT45DB161 (512 bytes)
 *
 * \param addr page address e.g. AT45DB161 (0 ... 4095)
 *
 * \note The time to erase one bock can take
 * up to 15ms - 35ms!
 */
void at45db_erase_page(uint16_t addr);

/**
 * \brief This function writes bytes to the active buffer, while
 * the buffer management is done automatically.
 *
 * \param addr Byte address within the buffer e.g. AT45DB161 (0 ... 527)
 * \param *buffer Pointer to local byte buffer
 * \param bytes Number of bytes (e.g. byte buffer size) which have to
 *        be written to the active buffer
 *
 */
void at45db_write_buffer(uint16_t addr, uint8_t *buffer, uint16_t bytes);

/**
 * \brief This function copies the active buffer into the Flash
 * EEPROM page. Moreover it switches the active buffer to avoid
 * latency.
 *
 * \param addr page address e.g. AT45DB161 (0 ... 4095)
 *
 */
void at45db_buffer_to_page(uint16_t addr);

/**
 * \brief This function copies the data from the the pointer
 * into the buffer, erases the EEPROM page and flashes the new
 * content directly into the page
 *
 * \param p_addr page address e.g. AT45DB161 (0 - 4095)
 * \param b_addr byte address within the page e.g. AT45DB161 (0 - 527)
 * \param *buffer Pointer to local byte buffer
 * \param bytes Number of bytes (e.g. byte buffer size) which have to
 *        be read to the local byte buffer
 */
void at45db_write_page(uint16_t p_addr, uint16_t b_addr, uint8_t *buffer, uint16_t bytes);

/**
 * \brief Bytes can be read via buffer from a Flash EEPROM page. With this
 * function you select the page, the start byte within the page and the
 * number of bytes you want to read.
 *
 * \param p_addr page address e.g. AT45DB161 (0 - 4095)
 * \param b_addr byte address within the page e.g. AT45DB161 (0 - 527)
 * \param *buffer Pointer to local byte buffer
 * \param bytes Number of bytes (e.g. byte buffer size) which have to
 *        be read to the local byte buffer
 *
 */
void at45db_read_page_buffered(uint16_t p_addr, uint16_t b_addr, uint8_t *buffer, uint16_t bytes);

/**
 * \brief Bytes can be read direct (bypassed) from a Flash EEPROM page. With this
 * function you select the page, the start byte within the page and the
 * number of bytes you want to read.
 *
 * \param p_addr page address e.g. AT45DB161 (0 - 4095)
 * \param b_addr byte address within the page e.g. AT45DB161 (0 - 527)
 * \param *buffer Pointer to local byte buffer
 * \param bytes Number of bytes (e.g. byte buffer size) which have to
 *        be read to the local byte buffer
 *
 */
void at45db_read_page_bypassed(uint16_t p_addr, uint16_t b_addr, uint8_t *buffer, uint16_t bytes);

/**
 * \brief Copies the given page into the buffer 2.
 * \note Only Buffer 2 is used to readout a page, because the read
 * respectively transfer latency is only about 200us
 *
 * \param addr page address e.g. AT45DB161 (0 - 4095)
 *
 */
void at45db_page_to_buf(uint16_t addr);

/**
 * \brief This function readouts the buffer 2 data.
 *
 * \param b_addr byte address within the page e.g. AT45DB161 (0 - 527)
 * \param *buffer Pointer to local byte buffer
 * \param bytes Number of bytes (e.g. byte buffer size) which have to
 *        be read to the local byte buffer
 *
 */
void at45db_read_buffer(uint8_t b_addr, uint8_t *buffer, uint16_t bytes);

/**
 * \brief The command word of the AT45DBxx1 normally consists of 4 bytes.
 * This function enables the chip select and sends the command (opcode +
 * address information) to the AT45DBxx1.
 *
 * \param *cmd Pointer to the 4 byte command array
 *
 */
void at45db_write_cmd(uint8_t *cmd);

/**
 * \brief This function waits until the busy flag of the status register is set,
 * to detect when the AT45DBxx1 device is ready to receive new commands
 */
void at45db_busy_wait(void);

/** @} */
/** @} */

#endif /* FLASHAT45DB_H_ */

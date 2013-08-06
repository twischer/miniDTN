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
 *		Atmel Flash EEPROM AT45DB interface implementation
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_device_driver
 * @{
 */

/**
 * \addtogroup AT45DB_interface
 * @{
 */

#include "at45db.h"

/*!
 * Buffer manager allows it to improve write times, by switching
 * the dual buffer and parallelize flash write operations. (e.g. Write
 * to buffer 1 while buffer 2 is transfered to flash EEPROM)
 */
static bufmgr_t buffer_mgr;
static uint8_t initialized = 0;

int8_t
at45db_init(void) {
  uint8_t i = 0, id = 0;
  /*setup buffer manager to perform write and read operations*/
  buffer_mgr.active_buffer = 0;
  buffer_mgr.buffer_addr[0] = AT45DB_BUFFER_1;
  buffer_mgr.buffer_addr[1] = AT45DB_BUFFER_2;
  buffer_mgr.buf_to_page_addr[0] = AT45DB_BUF_1_TO_PAGE;
  buffer_mgr.buf_to_page_addr[1] = AT45DB_BUF_2_TO_PAGE;
  buffer_mgr.page_program[0] = AT45DB_PAGE_PROGRAM_1;
  buffer_mgr.page_program[1] = AT45DB_PAGE_PROGRAM_2;

  mspi_chip_release(AT45DB_CS);
  /*init mspi in mode3, at chip select pin 3 and max baud rate*/
  mspi_init(AT45DB_CS, MSPI_MODE_3, MSPI_BAUD_MAX);

  while (id != 0x1F) {
    mspi_chip_select(AT45DB_CS);
    mspi_transceive(0x9F);
    id = mspi_transceive(0x00);
    mspi_chip_release(AT45DB_CS);
    _delay_ms(10);
    if (i++ > 10) {
      initialized = 0;
      return -1;
    }
  }
  initialized = 1;
  return 0;

}
/*----------------------------------------------------------------------------*/
void
at45db_erase_chip(void) {
  if (!initialized) return;
  /*chip erase command consists of 4 byte*/
  uint8_t cmd[4] = {0xC7, 0x94, 0x80, 0x9A};
  at45db_write_cmd(&cmd[0]);
  mspi_chip_release(AT45DB_CS);
  /*wait until AT45DB161 is ready again*/
  mspi_chip_select(AT45DB_CS);
  at45db_busy_wait();
  mspi_chip_release(AT45DB_CS);
}
/*----------------------------------------------------------------------------*/
void
at45db_erase_block(uint16_t addr) {
  if (!initialized) return;
  /*block erase command consists of 4 byte*/
  uint8_t cmd[4] = {AT45DB_BLOCK_ERASE, (uint8_t) (addr >> 3),
    (uint8_t) (addr << 5), 0x00};
  at45db_write_cmd(&cmd[0]);
  mspi_chip_release(AT45DB_CS);
  /*wait until AT45DB161 is ready again*/
  mspi_chip_select(AT45DB_CS);
  at45db_busy_wait();
  mspi_chip_release(AT45DB_CS);
}
/*----------------------------------------------------------------------------*/
void
at45db_erase_page(uint16_t addr) {
  if (!initialized) return;
  /*block erase command consists of 4 byte*/
  uint8_t cmd[4] = {AT45DB_PAGE_ERASE, (uint8_t) (addr >> 6),
    (uint8_t) (addr << 2), 0x00};
  at45db_write_cmd(&cmd[0]);
  mspi_chip_release(AT45DB_CS);
  /*wait until AT45DB161 is ready again*/
  mspi_chip_select(AT45DB_CS);
  at45db_busy_wait();
  mspi_chip_release(AT45DB_CS);
}
/*----------------------------------------------------------------------------*/
void
at45db_write_buffer(uint16_t addr, uint8_t *buffer, uint16_t bytes) {
  uint16_t i;
  if (!initialized) return;
  /*block erase command consists of 4 byte*/
  uint8_t cmd[4] = {buffer_mgr.buffer_addr[buffer_mgr.active_buffer], 0x00,
    (uint8_t) (addr >> 8), (uint8_t) (addr)};

  at45db_write_cmd(&cmd[0]);

  for (i = 0; i < bytes; i++) {
    mspi_transceive(~(*buffer++));
  }

  mspi_chip_release(AT45DB_CS);
}
/*----------------------------------------------------------------------------*/
void
at45db_buffer_to_page(uint16_t addr) {
  if (!initialized) return;
  /*wait until AT45DB161 is ready again*/
  at45db_busy_wait();
  /*write active buffer to page command consists of 4 byte*/
  uint8_t cmd[4] = {buffer_mgr.buf_to_page_addr[buffer_mgr.active_buffer],
    (uint8_t) (addr >> 6), (uint8_t) (addr << 2), 0x00};
  at45db_write_cmd(&cmd[0]);
  mspi_chip_release(AT45DB_CS);
  /* switch active buffer to allow the other one to be written,
   * while these buffer is copied to the Flash EEPROM page*/
  buffer_mgr.active_buffer ^= 1;
}
/*----------------------------------------------------------------------------*/
void
at45db_write_page(uint16_t p_addr, uint16_t b_addr, uint8_t *buffer, uint16_t bytes) {
  uint16_t i;
  if (!initialized) return;
  /*block erase command consists of 4 byte*/
  uint8_t cmd[4] = {buffer_mgr.page_program[buffer_mgr.active_buffer],
    (uint8_t) (p_addr >> 6), ((uint8_t) (p_addr << 2) & 0xFC) | ((uint8_t) (b_addr >> 8) & 0x3), (uint8_t) (b_addr)};

  at45db_write_cmd(&cmd[0]);

  for (i = 0; i < bytes; i++) {
    mspi_transceive(~(*buffer++));
  }

  mspi_chip_release(AT45DB_CS);

  /* switch active buffer to allow the other one to be written,
   * while these buffer is copied to the Flash EEPROM page*/
  buffer_mgr.active_buffer ^= 1;
}
/*----------------------------------------------------------------------------*/
void
at45db_read_page_buffered(uint16_t p_addr, uint16_t b_addr,
        uint8_t *buffer, uint16_t bytes) {
  if (!initialized) return;
  /*wait until AT45DB161 is ready again*/
  at45db_busy_wait();
  at45db_page_to_buf(p_addr);
  at45db_read_buffer(b_addr, buffer, bytes);
}
/*----------------------------------------------------------------------------*/
void
at45db_read_page_bypassed(uint16_t p_addr, uint16_t b_addr,
        uint8_t *buffer, uint16_t bytes) {
  uint16_t i;
  if (!initialized) return;
  /*wait until AT45DB161 is ready again*/
  at45db_busy_wait();
  /* read bytes directly from page command consists of 4 cmd bytes and
   * 4 don't care */
  uint8_t cmd[4] = {AT45DB_PAGE_READ, (uint8_t) (p_addr >> 6),
    (((uint8_t) (p_addr << 2)) & 0xFC) | ((uint8_t) (b_addr >> 8)),
    (uint8_t) (b_addr)};
  at45db_write_cmd(&cmd[0]);
  for (i = 0; i < 4; i++) {
    mspi_transceive(0x00);
  }
  /*now the data bytes can be received*/
  for (i = 0; i < bytes; i++) {
    *buffer++ = ~mspi_transceive(MSPI_DUMMY_BYTE);
  }
  mspi_chip_release(AT45DB_CS);
}
/*----------------------------------------------------------------------------*/
void
at45db_page_to_buf(uint16_t addr) {

  if (!initialized) return;
  /*write active buffer to page command consists of 4 byte*/
  uint8_t cmd[4] = {AT45DB_PAGE_TO_BUF, (uint8_t) (addr >> 6),
    (uint8_t) (addr << 2), 0x00};
  at45db_write_cmd(&cmd[0]);
  mspi_chip_release(AT45DB_CS);
  /* switch active buffer to allow the other one to be written,
   * while these buffer is copied to the Flash EEPROM page*/
  //buffer_mgr.active_buffer ^= 1;
  at45db_busy_wait();

}
/*----------------------------------------------------------------------------*/
void
at45db_read_buffer(uint8_t b_addr, uint8_t *buffer, uint16_t bytes) {
  uint16_t i;
  if (!initialized) return;
  uint8_t cmd[4] = {AT45DB_READ_BUFFER, 0x00, (uint8_t) (b_addr >> 8),
    (uint8_t) (b_addr)};
  at45db_busy_wait();
  at45db_write_cmd(&cmd[0]);
  mspi_transceive(0x00);

  for (i = 0; i < bytes; i++) {
    *buffer++ = ~mspi_transceive(0x00);
  }
  mspi_chip_release(AT45DB_CS);
}
/*----------------------------------------------------------------------------*/
void
at45db_write_cmd(uint8_t *cmd) {
  uint8_t i;
  if (!initialized) return;
  mspi_chip_select(AT45DB_CS);
  for (i = 0; i < 4; i++) {
    mspi_transceive(*cmd++);
  }
}
/*----------------------------------------------------------------------------*/
void
at45db_busy_wait(void) {
  uint16_t i = 0;
  if (!initialized) return;
  mspi_chip_select(AT45DB_CS);
  mspi_transceive(AT45DB_STATUS_REG);
  while ((mspi_transceive(MSPI_DUMMY_BYTE) >> 7) != 0x01) {
    if (i++ > 500) {
      return;
    }
  }
  mspi_chip_release(AT45DB_CS);
}

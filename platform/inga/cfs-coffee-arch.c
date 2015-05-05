/*
 * Copyright (c) 2009, Swedish Institute of Computer Science
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
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *  Coffee architecture-dependent functionality for INGA.
 * \author
 *  Nicolas Tsiftes <nvt@sics.se>
 *  Frederic Thepaut <frederic.thepaut@inooi.com>
 *  David Kopf <dak664@embarqmail.com>
 *  Enrico Joerns <e.joerns@tu-bs.de>
 */

#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include "dev/watchdog.h"

#include "cfs-coffee-arch.h"
#include "httpd-fs-arch.h"

#define DEBUG 0
/* Note: Debuglevel > 1 will also printout data read. */
#if DEBUG
#include <stdio.h>
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
/*---------------------------EEPROM ROUTINES---------------------------------*/
/*---------------------------------------------------------------------------*/
#ifdef COFFEE_INGA_EEPROM

/* Letting .bss initialize nullb to zero saves COFFEE_SECTOR_SIZE of flash */
//static const unsigned char nullb[COFFEE_SECTOR_SIZE] = {0};
static const unsigned char nullb[COFFEE_SECTOR_SIZE];

/*---------------------------------------------------------------------------*/
/* Erase EEPROM sector
 */
void
avr_eeprom_erase(uint16_t sector)
{
  eeprom_write(COFFEE_START + sector * COFFEE_SECTOR_SIZE,
          (unsigned char *) nullb, sizeof (nullb));
}
#endif /* COFFEE_INGA_EEPROM */

#ifdef COFFEE_INGA_FLASH
/*---------------------------------------------------------------------------*/
/*-----------------------Internal FLASH ROUTINES-----------------------------*/
/*---------------------------------------------------------------------------*/
/*
 * Read from flash info buf. addr contains starting flash byte address
 */
void
avr_flash_read(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  uint32_t addr32 = COFFEE_START + addr;
  uint16_t isize = size;
#if DEBUG
  unsigned char *bufo = (unsigned char *) buf;
  uint8_t i;
  uint16_t w = addr32 >> 1; //Show progmem word address for debug
  PRINTF("r0x%04x(%u) ", w, size);
#endif
#ifndef FLASH_WORD_READS
  for (; isize > 0; isize--) {
#if FLASH_COMPLEMENT_DATA
    *buf++ = ~(uint8_t) pgm_read_byte_far(addr32++);
#else
    *buf++ = (uint8_t) pgm_read_byte_far(addr32++);
#endif /* FLASH_COMPLEMENT_DATA */
  }
#else /* FLASH_WORD_READS */
  /* 130 bytes more PROGMEM, but faster */
  if (isize & 0x01) { //handle first odd byte
#if FLASH_COMPLEMENT_DATA
    *buf++ = ~(uint8_t) pgm_read_byte_far(addr32++);
#else
    *buf++ = (uint8_t) pgm_read_byte_far(addr32++);
#endif /* FLASH_COMPLEMENT_DATA */
    isize--;
  }
  for (; isize > 1; isize -= 2) {//read words from flash
#if FLASH_COMPLEMENT_DATA
    *(uint16_t *) buf = ~(uint16_t) pgm_read_word_far(addr32);
#else
    *(uint16_t *) buf = (uint16_t) pgm_read_word_far(addr32);
#endif /* FLASH_COMPLEMENT_DATA */
    buf += 2;
    addr32 += 2;
  }
  if (isize) { //handle last odd byte
#if FLASH_COMPLEMENT_DATA
    *buf++ = ~(uint8_t) pgm_read_byte_far(addr32);
#else
    *buf++ = (uint8_t) pgm_read_byte_far(addr32);
#endif /* FLASH_COMPLEMENT_DATA */
  }
#endif /* FLASH_WORD_READS */

#if DEBUG>1
  PRINTF("\nbuf=");
  //  PRINTF("%s",bufo);
  // for (i=0;i<16;i++) PRINTF("%2x ",*bufo++);
#endif /* DEBUG */
}
/*---------------------------------------------------------------------------*/
/*
 Erase the flash page(s) corresponding to the coffee sector.
 This is done by calling the write routine with a null buffer and any address
 within each page of the sector (we choose the first byte).
 */
BOOTLOADER_SECTION
void
avr_flash_erase(coffee_page_t sector)
{
  coffee_page_t i;

#if FLASH_COMPLEMENT_DATA
  uint32_t addr32;
  volatile uint8_t sreg;

  // Disable interrupts.
  sreg = SREG;
  cli();

  for (i = 0; i < COFFEE_SECTOR_SIZE / COFFEE_PAGE_SIZE; i++) {
    for (addr32 = COFFEE_START + (((sector + i) * COFFEE_PAGE_SIZE)
            & ~(COFFEE_PAGE_SIZE - 1)); addr32 < (COFFEE_START + (((sector
            + i + 1) * COFFEE_PAGE_SIZE) & ~(COFFEE_PAGE_SIZE - 1))); addr32
            += SPM_PAGESIZE) {
      boot_page_erase(addr32);
      boot_spm_busy_wait();

    }
  }
  //RE-enable interrupts
  boot_rww_enable();
  SREG = sreg;
#else /* FLASH_COMPLEMENT_DATA */
  for (i = 0; i < COFFEE_SECTOR_SIZE / COFFEE_PAGE_SIZE; i++) {
    avr_flash_write((sector + i) * COFFEE_PAGE_SIZE, 0, 0);
  }
#endif /* FLASH_COMPLEMENT_DATA */
}
/*---------------------------------------------------------------------------*/
/*
 * Transfer buf[size] from RAM to flash, starting at addr.
 * If buf is null, just erase the flash page
 * Note this routine has to be in the bootloader NRWW part of program memory,
 * and that writing to NRWW (last 32 pages on the 1284p) will halt the CPU.
 */
BOOTLOADER_SECTION
void
avr_flash_write(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  uint32_t addr32;
  uint16_t w;
  uint8_t bb, ba, sreg;

  /* Disable interrupts, make sure no eeprom write in progress */
  sreg = SREG;
  cli();
  eeprom_busy_wait();

  /* Calculate the starting address of the first flash page being
    modified (will be on a page boundary) and the number of
    unaltered bytes before and after the data to be written.          */
#if 0    //this is 8 bytes longer
  uint16_t startpage = addr / COFFEE_PAGE_SIZE;
  addr32 = COFFEE_START + startpage*COFFEE_PAGE_SIZE;
#else
  addr32 = (COFFEE_ADDRESS&~(SPM_PAGESIZE - 1))+(addr&~(SPM_PAGESIZE - 1));
#endif
  bb = addr & (SPM_PAGESIZE - 1);
  ba = COFFEE_PAGE_SIZE - ((addr + size)&0xff);

#if DEBUG
  uint16_t startpage = addr / COFFEE_PAGE_SIZE;
  w = addr32 >> 1; //Show progmem word address for debug
  if (buf) {
    PRINTF("w0x%04x %u %u %u", w, size, bb, ba);
  } else {
    PRINTF("e0x%04x %u ", w, startpage);
  }
#endif /* DEBUG */

  /* If buf not null, modify the page(s) */
  if (buf) {
    if (size == 0) return; //nothing to write
    /*Copy the first part of the existing page into the write buffer */
    while (bb > 1) {
      w = pgm_read_word_far(addr32);
      boot_page_fill(addr32, w);
      addr32 += 2;
      bb -= 2;
    }
    /* Transfer the bytes to be modified */
    while (size > 1) {
      if (bb) { //handle odd byte boundary
        w = pgm_read_word_far(addr32);
#if FLASH_COMPLEMENT_DATA
        w = ~w;
#endif /* FLASH_COMPLEMENT_DATA */
        w &= 0xff;
        bb = 0;
        size++;
      } else {
        w = *buf++;
      }
      w += (*buf++) << 8;
#if FLASH_COMPLEMENT_DATA
      w = ~w;
#endif /* FLASH_COMPLEMENT_DATA */
      boot_page_fill(addr32, w);
      size -= 2;
      /* Below ought to work but writing to 0xnnnnnnfe modifies the NEXT flash page
         for some reason, at least in the AVR Studio simulator.
            if ((addr32&0x000000ff)==0x000000fe) { //handle page boundary
              if (size) {
                boot_page_erase(addr32);
                boot_spm_busy_wait();
                boot_page_write(addr32);
                boot_spm_busy_wait();
              }
            }
             addr32+=2;
       */

      /* This works...*/
      addr32 += 2;
      if ((addr32 & 0x000000ff) == 0) { //handle page boundary
        if (size) {
          addr32 -= 0x42; //get an address within the page
          boot_page_erase(addr32);
          boot_spm_busy_wait();
          boot_page_write(addr32);
          boot_spm_busy_wait();
          addr32 += 0x42;
        }
      }
    }
    /* Copy the remainder of the existing page */
    while (ba > 1) {
      w = pgm_read_word_far(addr32);
      if (size) { //handle odd byte boundary
        w &= 0xff00;
#if FLASH_COMPLEMENT_DATA
        w += ~(*buf);
#else /* FLASH_COMPLEMENT_DATA */
        w += *buf;
#endif /* FLASH_COMPLEMENT_DATA */
        size = 0;
      }
      boot_page_fill(addr32, w);
      addr32 += 2;
      ba -= 2;
    }
    /* If buf is null, erase the page to zero */
  } else {
#if FLASH_COMPLEMENT_DATA
    addr32 += 2 * SPM_PAGESIZE;
#else
    for (w = 0; w < SPM_PAGESIZE; w++) {
      boot_page_fill(addr32, 0);
      addr32 += 2;
    }
#endif /* FLASH_COMPLEMENT_DATA */
  }
  /* Write the last (or only) page */
  addr32 -= 0x42; //get an address within the page
  boot_page_erase(addr32);
  boot_spm_busy_wait();
#if FLASH_COMPLEMENT_DATA
  if (buf) { //don't write zeroes to erased page
    boot_page_write(addr32);
    boot_spm_busy_wait();
  }
#else /* FLASH_COMPLEMENT_DATA */
  boot_page_write(addr32);
  boot_spm_busy_wait();
#endif /* FLASH_COMPLEMENT_DATA */
  /* Reenable RWW-section again. We need this if we want to jump back
   * to the application after bootloading. */
  boot_rww_enable();

  /* Re-enable interrupts (if they were ever enabled). */
  SREG = sreg;
}

#endif /* COFFEE_INGA_FLASH */


/*---------------------------------------------------------------------------*/
/*-----------------------External FLASH ROUTINES-----------------------------*/
/*---------------------------------------------------------------------------*/
#ifdef COFFEE_INGA_EXTERNAL

#include "dev/at45db.h"
void
external_flash_write_page(coffee_page_t page, CFS_CONF_OFFSET_TYPE offset, uint8_t * buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF("external_flash_write_page(page %u, offset %u, buf %p, size %u) \n", page, offset, buf, size);

  if (size < 1) {
    return;
  }

  if (page > COFFEE_PAGES) {
    return;
  }

  unsigned char buffer[COFFEE_PAGE_SIZE];

  // Now read the current content of that page
  at45db_read_page_bypassed(page, 0, buffer, COFFEE_PAGE_SIZE);

  watchdog_periodic();

  // Copy over the new content
  memcpy(buffer + offset, buf, size);

  // And write the page again
  at45db_write_page(page, 0, buffer, COFFEE_PAGE_SIZE);

  watchdog_periodic();

  PRINTF("Page %u programmed with %u bytes (%u new)\n", page, COFFEE_PAGE_SIZE, size);
}
/*----------------------------------------------------------------------------*/
void
external_flash_write(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF(">>>>> external_flash_write(addr %u, buf %p, size %u)\n", addr, buf, size);

  if (addr > COFFEE_SIZE) {
    return;
  }

  // Which is the first page we will be programming
  coffee_page_t current_page = addr / COFFEE_PAGE_SIZE;
  CFS_CONF_OFFSET_TYPE written = 0;

  while (written < size) {
    // get the start address of the current page
    CFS_CONF_OFFSET_TYPE page_start = current_page * COFFEE_PAGE_SIZE;
    CFS_CONF_OFFSET_TYPE offset = 0;

    if (addr > page_start) {
      // Frist page offset
      offset = addr - page_start;
    }

    CFS_CONF_OFFSET_TYPE length = size - written;

    if (length > (COFFEE_PAGE_SIZE - offset)) {
      length = COFFEE_PAGE_SIZE - offset;
    }

    external_flash_write_page(current_page, offset, buf + written, length);
    written += length;
    current_page++;
  }

#if DEBUG
  int g;
  printf("WROTE: ");
  for (g = 0; g < size; g++) {
    printf("%02X %c ", buf[g] & 0xFF, buf[g] & 0xFF);
  }
  printf("\n");
#endif /* DEBUG */
}
/*----------------------------------------------------------------------------*/
void
external_flash_read_page(coffee_page_t page, CFS_CONF_OFFSET_TYPE offset, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF("external_flash_read_page(page %u, offset %u, buf %p, size %u)\n", page, offset, buf, size);

  if (page > COFFEE_PAGES) {
    return;
  }

  at45db_read_page_bypassed(page, offset, buf, size);
  watchdog_periodic();

#if DEBUG > 1
  int g;
  printf("READ: ");
  for (g = 0; g < size; g++) {
    printf("%02X %c ", buf[g] & 0xFF, buf[g] & 0xFF);
  }
  printf("\n");
#endif /* DEBUG */
}
/*----------------------------------------------------------------------------*/
void
external_flash_read(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF(">>>>> external_flash_read(addr %u, buf %p, size %u)\n", addr, buf, size);

  if (size < 1) {
    return;
  }

  if (addr > COFFEE_SIZE) {
    return;
  }

  // First of all: find out what the number of the page is
  coffee_page_t current_page = addr / COFFEE_PAGE_SIZE;
  CFS_CONF_OFFSET_TYPE read = 0;

  while (read < size) {
    // get the start address of the current page
    CFS_CONF_OFFSET_TYPE page_start = current_page * COFFEE_PAGE_SIZE;
    CFS_CONF_OFFSET_TYPE offset = 0;

    if (addr > page_start) {
      // Frist page offset
      offset = addr - page_start;
    }

    CFS_CONF_OFFSET_TYPE length = size - read;

    if (length > (COFFEE_PAGE_SIZE - offset)) {
      length = (COFFEE_PAGE_SIZE - offset);
    }

    external_flash_read_page(current_page, offset, buf + read, length);

//    PRINTF("Page %u read with %u bytes (offset %u)\n", h, length, offset);

    read += length;
    current_page++;
  }

#if DEBUG > 1
  int g;
  printf("READ: ");
  for (g = 0; g < size; g++) {
    printf("%02X %c ", buf[g] & 0xFF, buf[g] & 0xFF);
  }
  printf("\n");
#endif /* DEBUG */
}
/*----------------------------------------------------------------------------*/
void
external_flash_erase(coffee_page_t page)
{
  if (page > COFFEE_PAGES) {
    return;
  }

  PRINTF("external_flash_erase(page %u)\n", page);

  at45db_erase_page(page);
  watchdog_periodic();
}

/*
void external_flash_erase(coffee_page_t sector) {
  if( sector > COFFEE_SECTORS ) {
    return;
  }

  // This has to erase the contents of a whole sector
  // AT45DB cannot directly delete a sector, we have to do it manually
  PRINTF("external_flash_erase(sector %u)\n", sector);
  CFS_CONF_OFFSET_TYPE h;

  coffee_page_t start = sector * COFFEE_BLOCKS_PER_SECTOR;
  coffee_page_t end = start + COFFEE_BLOCKS_PER_SECTOR;

  for(h=start; h<end; h++) {
    PRINTF("Deleting block %u\n", h);

    at45db_erase_block(h);
    watchdog_periodic();
  }
}
 */

#endif /* COFFEE_INGA_EXTERNAL */

#ifdef COFFEE_INGA_SDCARD

#include "cfs/fat/diskio.h"

static uint8_t cfs_buffer[512];
struct diskio_device_info *cfs_info = 0;
void
sd_write_page(coffee_page_t page, CFS_CONF_OFFSET_TYPE offset, uint8_t * buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF("sd_write_page(page %lu, offset %lu, buf %p, size %lu) \n", page, offset, buf, size);

  if (size < 1) {
    return;
  }

  if (page > COFFEE_PAGES) {
    return;
  }

  // Now read the current content of that page
  diskio_read_block(cfs_info, page, cfs_buffer);

  watchdog_periodic();

  // Copy over the new content
  memcpy(cfs_buffer + offset, buf, size);

  // And write the page again
  diskio_write_block(cfs_info, page, cfs_buffer);

  watchdog_periodic();

  PRINTF("Page %lu programmed with %lu bytes (%lu new)\n", page, COFFEE_PAGE_SIZE, size);
}
/*----------------------------------------------------------------------------*/
void
sd_write(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF(">>>>> sd_write(addr %lu, buf %p, size %lu)\n", addr, buf, size);

  if (addr > COFFEE_SIZE) {
    return;
  }

  // Which is the first page we will be programming
  coffee_page_t current_page = addr / COFFEE_PAGE_SIZE;
  CFS_CONF_OFFSET_TYPE written = 0;

  while (written < size) {
    // get the start address of the current page
    CFS_CONF_OFFSET_TYPE page_start = current_page * COFFEE_PAGE_SIZE;
    CFS_CONF_OFFSET_TYPE offset = 0;

    if (addr > page_start) {
      // Frist page offset
      offset = addr - page_start;
    }

    CFS_CONF_OFFSET_TYPE length = size - written;

    if (length > (COFFEE_PAGE_SIZE - offset)) {
      length = COFFEE_PAGE_SIZE - offset;
    }

    sd_write_page(current_page, offset, buf + written, length);
    written += length;
    current_page++;
  }

#if DEBUG
  int g;
  printf("WROTE: ");
  for (g = 0; g < size; g++) {
    printf("%02X ", buf[g] & 0xFF, buf[g] & 0xFF);
  }
  printf("\n");
#endif /* DEBUG */
}
/*----------------------------------------------------------------------------*/
void
sd_read_page(coffee_page_t page, CFS_CONF_OFFSET_TYPE offset, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF("sd_read_page(page %lu, offset %lu, buf %p, size %lu)\n", page, offset, buf, size);

  if (page > COFFEE_PAGES) {
    return;
  }

  diskio_read_block(cfs_info, page, cfs_buffer);
  memcpy(buf, cfs_buffer + offset, size);

#if DEBUG
  int g;
  printf("READ: ");
  for (g = 0; g < size; g++) {
    printf("%02X ", buf[g] & 0xFF, buf[g] & 0xFF);
  }
  printf("\n");
#endif /* DEBUG */

}
/*----------------------------------------------------------------------------*/
void
sd_read(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size)
{
  PRINTF(">>>>> sd_read(addr %lu, buf %p, size %lu)\n", addr, buf, size);

  if (size < 1) {
    return;
  }

  if (addr > COFFEE_SIZE) {
    return;
  }

  // First of all: find out what the number of the page is
  coffee_page_t current_page = addr / COFFEE_PAGE_SIZE;
  CFS_CONF_OFFSET_TYPE read = 0;

  while (read < size) {
    // get the start address of the current page
    CFS_CONF_OFFSET_TYPE page_start = current_page * COFFEE_PAGE_SIZE;
    CFS_CONF_OFFSET_TYPE offset = 0;

    if (addr > page_start) {
      // Frist page offset
      offset = addr - page_start;
    }

    CFS_CONF_OFFSET_TYPE length = size - read;

    if (length > (COFFEE_PAGE_SIZE - offset)) {
      length = (COFFEE_PAGE_SIZE - offset);
    }

    sd_read_page(current_page, offset, buf + read, length);

    PRINTF("Page %lu read with %lu bytes (offset %lu)\n", current_page, length, offset);

    read += length;
    current_page++;
  }

#if DEBUG > 1
  int g;
  printf("READ: ");
  for (g = 0; g < size; g++) {
    printf("%02X ", buf[g] & 0xFF, buf[g] & 0xFF);
  }
  printf("\n");
#endif /* DEBUG */
}
/*----------------------------------------------------------------------------*/
void
sd_erase(coffee_page_t page)
{
  if (page > COFFEE_PAGES) {
    return;
  }

  PRINTF("sd_erase(page %lu)\n", page);
  memset(cfs_buffer, 0, 512);

  diskio_write_block(cfs_info, page, cfs_buffer);
  watchdog_periodic();
}

#endif /* COFFEE_INGA_SDCARD */


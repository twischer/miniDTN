/*
 * Copyright (c) 2008, Swedish Institute of Computer Science
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
 *  Coffee architecture-dependent header for the AVR-Raven 1284p platform.
 *  The 1284p has 4KB of onboard EEPROM and 128KB of program flash.
 * \author
 *  Frederic Thepaut <frederic.thepaut@inooi.com>
 *  David Kopf <dak664@embarqmail.com>
 */

#ifndef CFS_COFFEE_ARCH_H
#define CFS_COFFEE_ARCH_H

#include "contiki-conf.h"

//Currently you may choose just one of the following for the coffee file sytem
//A static file sysstem allows file rewrites but no extensions or new files
//This allows a static linked list to index into the file system
#ifndef COFFEE_DEVICE
#define COFFEE_DEVICE 3
#endif

#if COFFEE_DEVICE==1             //1=eeprom for static file system
#define COFFEE_INGA_EEPROM 1
#define COFFEE_STATIC     1
#elif COFFEE_DEVICE==2           //2=eeprom for full file system
#define COFFEE_INGA_EEPROM 1
#elif COFFEE_DEVICE==3           //3=program flash for static file system
#define COFFEE_INGA_FLASH  1
#define COFFEE_STATIC     1
#elif COFFEE_DEVICE==4           //4=program flash with full file system
#define COFFEE_INGA_FLASH  1
#elif COFFEE_DEVICE==5			//5=use external onboard flash with full file system
#define COFFEE_INGA_EXTERNAL 1
#elif COFFEE_DEVICE==6           //6=sdcard flash for full file system
#define COFFEE_INGA_SDCARD  1
#endif

#ifdef COFFEE_INGA_EEPROM
#include "dev/eeprom.h"
//1284p EEPROM has 512 pages of 8 bytes each = 4KB

#if COFFEE_ADDRESS==DEFAULT       //Make can pass starting address with COFFEE_ADDRESS=0xnnnnnnnn
#undef COFFEE_ADDRESS
#ifdef CFS_EEPROM_CONF_OFFSET     //Else use the platform default
#define COFFEE_ADDRESS            CFS_EEPROM_CONF_OFFSET
#else                             //Use zero if no default defined
#define COFFEE_ADDRESS            0
#endif
#endif

/* Byte page size, starting address, and size of the file system */
#define COFFEE_PAGE_SIZE          16UL
#define COFFEE_START              COFFEE_ADDRESS
#define COFFEE_SIZE               ((3 * 1024U) - COFFEE_START)
/* These must agree with the parameters passed to makefsdata */
#define COFFEE_SECTOR_SIZE        (COFFEE_PAGE_SIZE*4)
#define COFFEE_NAME_LENGTH        16
/* These are used internally by the coffee file system */
#define COFFEE_MAX_OPEN_FILES     4
#define COFFEE_FD_SET_SIZE        8
#define COFFEE_LOG_TABLE_LIMIT    16
#define COFFEE_DYN_SIZE           (COFFEE_PAGE_SIZE * 4)
#define COFFEE_LOG_SIZE           128

typedef int16_t coffee_page_t;
typedef uint16_t coffee_offset_t;

#define COFFEE_ERASE(sector) avr_eeprom_erase(sector)
void avr_eeprom_erase(uint16_t sector);

#define COFFEE_WRITE(buf, size, offset) \
        eeprom_write(COFFEE_START + (offset), (unsigned char *)(buf), (size))

#define COFFEE_READ(buf, size, offset) \
        eeprom_read (COFFEE_START + (offset), (unsigned char *)(buf), (size))


#elif defined COFFEE_INGA_FLASH /* COFFEE_INGA_EEPROM */
/* 1284p PROGMEM has 512 pages of 256 bytes each = 128KB
 * Writing to the last 32 NRRW pages will halt the CPU.
 * Take care not to overwrite the .bootloader section...
 */

/* Byte page size, starting address on page boundary, and size of the file system */
#define COFFEE_PAGE_SIZE          (2*SPM_PAGESIZE)
#ifndef COFFEE_ADDRESS            //Make can pass starting address with COFFEE_ADDRESS=0xnnnnnnnn, default is 64KB for webserver
#define COFFEE_ADDRESS            0x10000
#endif
#define COFFEE_PAGES              (512-(COFFEE_ADDRESS/COFFEE_PAGE_SIZE)-32) /* COFFEE_ADDRESS undefined */
#define COFFEE_START              (COFFEE_ADDRESS & ~(COFFEE_PAGE_SIZE-1))
//#define COFFEE_START            (COFFEE_PAGE_SIZE*COFFEE_PAGES) 
#define COFFEE_SIZE               (COFFEE_PAGES*COFFEE_PAGE_SIZE) /* XXX */

/* These must agree with the parameters passed to makefsdata */
#define COFFEE_SECTOR_SIZE        (COFFEE_PAGE_SIZE*1)
#define COFFEE_NAME_LENGTH        16

/* These are used internally by the AVR flash read routines */
/* Word reads are faster but take 130 bytes more PROGMEM */
#define FLASH_WORD_READS          1
/* 1=Slower reads, but no page writes after erase and 18 bytes less PROGMEM. Best for dynamic file system */
#define FLASH_COMPLEMENT_DATA     1 

/* These are used internally by the coffee file system */
/* Micro logs are not needed for single page sectors */
#define COFFEE_MAX_OPEN_FILES     4
#define COFFEE_FD_SET_SIZE        8
#define COFFEE_LOG_TABLE_LIMIT    16
#define COFFEE_DYN_SIZE           (COFFEE_PAGE_SIZE*1)
#define COFFEE_MICRO_LOGS         0
#define COFFEE_LOG_SIZE           128

/* coffee_page_t is used for page and sector numbering
 * uint8_t can handle 511 pages.
 * cfs_offset_t is used for full byte addresses
 * If CFS_CONF_OFFSET_TYPE is not defined it defaults to int.
 * uint16_t can handle up to a 65535 byte file system.
 */
#define coffee_page_t uint8_t
#define CFS_CONF_OFFSET_TYPE uint16_t


#define COFFEE_WRITE(buf, size, offset) \
        avr_flash_write(offset, (uint8_t *) buf, size)

#define COFFEE_READ(buf, size, offset) \
        avr_flash_read(offset, (uint8_t *) buf, size)

#define COFFEE_ERASE(sector) avr_flash_erase(sector)

void avr_flash_erase(coffee_page_t sector);
void avr_flash_read (CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size);
void avr_flash_write(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size);


#elif defined COFFEE_INGA_EXTERNAL /* COFFEE_INGA_FLASH */

/* Byte page size, starting address on page boundary, and size of the file system */
#define COFFEE_PAGE_SIZE          528UL
#ifndef COFFEE_ADDRESS
#define COFFEE_ADDRESS            0x0
#endif
#define COFFEE_PAGES              4096UL
#define COFFEE_START              (COFFEE_ADDRESS)
#define COFFEE_SIZE               2162688UL

/* These must agree with the parameters passed to makefsdata */
#define COFFEE_SECTOR_SIZE        COFFEE_PAGE_SIZE
#define COFFEE_NAME_LENGTH        16

/* These are used internally by the coffee file system */
/* Micro logs are not needed for single page sectors */
#define COFFEE_MAX_OPEN_FILES     6
#define COFFEE_FD_SET_SIZE        8
#define COFFEE_LOG_TABLE_LIMIT    16
#define COFFEE_DYN_SIZE           (COFFEE_PAGE_SIZE*1)
#define COFFEE_MICRO_LOGS         0
#define COFFEE_LOG_SIZE           128

/* coffee_page_t is used for page and sector numbering
 * uint8_t can handle 511 pages.
 * cfs_offset_t is used for full byte addresses
 * If CFS_CONF_OFFSET_TYPE is not defined it defaults to int.
 * uint16_t can handle up to a 65535 byte file system.
 */
#define coffee_page_t uint16_t
#define CFS_CONF_OFFSET_TYPE uint32_t

#define COFFEE_WRITE(buf, size, offset) \
		external_flash_write((CFS_CONF_OFFSET_TYPE) offset, (uint8_t *) buf, (CFS_CONF_OFFSET_TYPE) size)

#define COFFEE_READ(buf, size, offset) \
		external_flash_read((CFS_CONF_OFFSET_TYPE) offset, (uint8_t *) buf, (CFS_CONF_OFFSET_TYPE) size)

#define COFFEE_ERASE(sector) \
		external_flash_erase((coffee_page_t) sector)

void external_flash_write_page(coffee_page_t page, CFS_CONF_OFFSET_TYPE offset, uint8_t * buf, CFS_CONF_OFFSET_TYPE size);

void external_flash_write(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size);

void external_flash_read(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size);

void external_flash_erase(coffee_page_t sector);


#elif defined COFFEE_INGA_SDCARD /* COFFEE_INGA_EXTERNAL */
/* Byte page size, starting address on page boundary, and size of the file system */
#define COFFEE_PAGE_SIZE          512
#ifndef COFFEE_ADDRESS
#define COFFEE_ADDRESS            0x0
#endif
#define COFFEE_PAGES              500UL
#define COFFEE_START              (COFFEE_ADDRESS)
#define COFFEE_SIZE               (COFFEE_PAGES * COFFEE_PAGE_SIZE)

/* These must agree with the parameters passed to makefsdata */
#define COFFEE_SECTOR_SIZE        COFFEE_PAGE_SIZE
#define COFFEE_NAME_LENGTH        16

/* These are used internally by the coffee file system */
/* Micro logs are not needed for single page sectors */
#define COFFEE_MAX_OPEN_FILES     6
#define COFFEE_FD_SET_SIZE        8
#define COFFEE_LOG_TABLE_LIMIT    16
#define COFFEE_DYN_SIZE           (COFFEE_PAGE_SIZE*1)
#define COFFEE_MICRO_LOGS         0
#define COFFEE_LOG_SIZE           128

/* coffee_page_t is used for page and sector numbering
 * uint8_t can handle 511 pages.
 * cfs_offset_t is used for full byte addresses
 * If CFS_CONF_OFFSET_TYPE is not defined it defaults to int.
 * uint16_t can handle up to a 65535 byte file system.
 */
#define coffee_page_t uint32_t
#define CFS_CONF_OFFSET_TYPE uint32_t

#define COFFEE_WRITE(buf, size, offset) \
		sd_write(offset, (uint8_t *) buf, size)

#define COFFEE_READ(buf, size, offset) \
		sd_read(offset, (uint8_t *) buf, size)

#define COFFEE_ERASE(sector) \
		sd_erase(sector)

void sd_write(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size);

void sd_read(CFS_CONF_OFFSET_TYPE addr, uint8_t *buf, CFS_CONF_OFFSET_TYPE size);

void sd_erase(coffee_page_t sector);

#else /* COFFEE_INGA_SDCARD */
#error No coffee device defined
#endif /* COFFEE_INGA_SDCARD */

int coffee_file_test(void);
#endif /* !COFFEE_ARCH_H */

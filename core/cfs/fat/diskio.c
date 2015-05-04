/*
 * Copyright (c) 2012, Institute of Operating Systems and Computer Networks (TU Braunschweig).
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
 * Author: Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup cfs
 * @{
 *
 * \addtogroup diskio_layer
 * @{
 */

/**
 * \file
 *      DiskIO Abstraction Layer Implementation
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */

#include "diskio.h"
#include "mbr.h"
#include <string.h>
#include "diskio-arch.h"

#ifndef DISKIO_DEBUG
#define DISKIO_DEBUG 0
#endif
#if DISKIO_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* Number of read/write retries */
#define DISKIO_RW_RETRIES   150
/* Delay between retries. */
#define DISKIO_RW_DELAY_MS  10

static struct diskio_device_info *default_device = 0;
static struct diskio_device_info devices[DISKIO_MAX_DEVICES];

static int diskio_rw_op(struct diskio_device_info *dev, uint32_t block_start_address, uint32_t num_blocks, uint8_t *buffer, uint8_t op);
/*----------------------------------------------------------------------------*/
void
diskio_print_device_info(struct diskio_device_info *dev)
{
  printf("DiskIO Device Info\n");

  printf("\ttype = ");
  switch (dev->type & 0x7F) {
    case DISKIO_DEVICE_TYPE_SD_CARD:
      printf("SD_CARD");
      break;
    case DISKIO_DEVICE_TYPE_GENERIC_FLASH:
      printf("Generic_Flash");
      break;
    default:
      printf("Unknown: %d", dev->type & 0x7F);
      break;
  }

  if (dev->type & DISKIO_DEVICE_TYPE_PARTITION) {
    printf(" (Partition)");
  }

  printf("\n");
  printf("\tnumber = %d\n", dev->number);
  printf("\tpartition = %d\n", dev->partition);
  printf("\tnum_sectors = %ld\n", dev->num_sectors);
  printf("\tsector_size = %d\n", dev->sector_size);
  printf("\tfirst_sector = %ld\n", dev->first_sector);
}
/*----------------------------------------------------------------------------*/
int
diskio_read_block(struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer)
{
  return diskio_rw_op(dev, block_address, 1, buffer, DISKIO_OP_READ_BLOCK);
}
/*----------------------------------------------------------------------------*/
int
diskio_read_blocks(struct diskio_device_info *dev, uint32_t block_start_address, uint32_t num_blocks, uint8_t *buffer)
{
  return diskio_rw_op(dev, block_start_address, num_blocks, buffer, DISKIO_OP_READ_BLOCKS);
}
/*----------------------------------------------------------------------------*/
int
diskio_write_block(struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer)
{
  return diskio_rw_op(dev, block_address, 1, buffer, DISKIO_OP_WRITE_BLOCK);
}
/*----------------------------------------------------------------------------*/
int
diskio_write_blocks_start(struct diskio_device_info *dev, uint32_t block_start_address, uint32_t num_blocks)
{
  return diskio_rw_op(dev, block_start_address, num_blocks, NULL, DISKIO_OP_WRITE_BLOCKS_START);
}
/*----------------------------------------------------------------------------*/
int
diskio_write_blocks_next(struct diskio_device_info *dev, uint8_t *buffer)
{
  return diskio_rw_op(dev, 0, 1, buffer, DISKIO_OP_WRITE_BLOCKS_NEXT);
}
/*----------------------------------------------------------------------------*/
int
diskio_write_blocks_done(struct diskio_device_info *dev)
{
  return diskio_rw_op(dev, 0, 0, NULL, DISKIO_OP_WRITE_BLOCKS_DONE);
}
/*----------------------------------------------------------------------------*/
static int
diskio_rw_op(struct diskio_device_info *dev, uint32_t block_start_address, uint32_t num_blocks, uint8_t *buffer, uint8_t op)
{
  static uint32_t multi_block_nr = 0;

  if (dev == NULL) {
    if (default_device == 0) {
      PRINTF("\nNo default device");
      return DISKIO_ERROR_NO_DEVICE_SELECTED;
    }
    dev = default_device;
  }

  block_start_address += dev->first_sector;

  uint8_t ret_code = 0;
  uint8_t tries = 0, reinit = 0;
  switch (dev->type & DISKIO_DEVICE_TYPE_MASK) {

#ifdef SD_INIT
    case DISKIO_DEVICE_TYPE_SD_CARD:
      switch (op) {
        case DISKIO_OP_READ_BLOCK:
#ifndef DISKIO_OLD_STYLE
          for (tries = 0; tries < DISKIO_RW_RETRIES; tries++) {
            ret_code = SD_READ_BLOCK(block_start_address, buffer);
            if (ret_code == 0) {
              return DISKIO_SUCCESS;
            } else {
              //PRINTF("\nret_code: %u", ret_code);
            }

#ifdef FAT_COOPERATIVE
            if (!coop_step_allowed) {
              next_step_type = READ;
              coop_switch_sp();
            } else {
              coop_step_allowed = 0;
            }
#else /* FAT_COOPERATIVE */
            _delay_ms(DISKIO_RW_DELAY_MS);
#endif /* FAT_COOPERATIVE */

            /* Try once to reinit sd card if access failed. */
            if ((reinit == 0) && (tries == DISKIO_RW_RETRIES - 1)) {
              PRINTF("\nReinit");
              tries = 0;
              reinit = 1;
              SD_INIT();
            }
          }
          PRINTF("\ndiskion_rw_op(): Unrecoverable Read Error!");
          return DISKIO_ERROR_INTERNAL_ERROR;
#else /* !DISKIO_OLD_STYLE */
          if (SD_READ_BLOCK(block_start_address, buffer) == 0) {
            return DISKIO_SUCCESS;
          }
          return DISKIO_ERROR_TRY_AGAIN;
#endif /* !DISKIO_OLD_STYLE */
          break;

        case DISKIO_OP_READ_BLOCKS:
          return DISKIO_ERROR_TO_BE_IMPLEMENTED;
          break;

        case DISKIO_OP_WRITE_BLOCK:
#ifndef DISKIO_OLD_STYLE
          for (tries = 0; tries < DISKIO_RW_RETRIES; tries++) {
            ret_code = SD_WRITE_BLOCK(block_start_address, buffer);
            if (ret_code == 0) {
              return DISKIO_SUCCESS;
            }

#ifdef FAT_COOPERATIVE
            if (!coop_step_allowed) {
              next_step_type = WRITE;
              coop_switch_sp();
            } else {
              coop_step_allowed = 0;
            }
#else /* FAT_COOPERATIVE */
            _delay_ms(DISKIO_RW_DELAY_MS);
#endif /* FAT_COOPERATIVE */
            if ((reinit == 0) && (tries == DISKIO_RW_RETRIES - 1)) {
              PRINTF("\nReinit");
              tries = 0;
              reinit = 1;
              SD_INIT();
            }
          }
          PRINTF("\ndiskion_rw_op(): Unrecoverable Write Error!");
          return DISKIO_ERROR_INTERNAL_ERROR;
#else /* !DISKIO_OLD_STYLE */
          if (SD_WRITE_BLOCK(block_start_address, buffer) == 0) {
            return DISKIO_SUCCESS;
          }
          return DISKIO_ERROR_TRY_AGAIN;
#endif /* !DISKIO_OLD_STYLE */
          break;

        case DISKIO_OP_WRITE_BLOCKS_START:
          ret_code = SD_WRITE_BLOCKS_START(block_start_address, num_blocks);
          if (ret_code == 0) {
            return DISKIO_SUCCESS;
          } else {
            return DISKIO_ERROR_INTERNAL_ERROR;
          }
          break;

        case DISKIO_OP_WRITE_BLOCKS_NEXT:
          ret_code = SD_WRITE_BLOCKS_NEXT(buffer);
          if (ret_code == 0) {
            return DISKIO_SUCCESS;
          } else {
            return DISKIO_ERROR_INTERNAL_ERROR;
          }
          break;

        case DISKIO_OP_WRITE_BLOCKS_DONE:
          ret_code = SD_WRITE_BLOCKS_DONE();
          if (ret_code == 0) {
            return DISKIO_SUCCESS;
          } else {
            return DISKIO_ERROR_INTERNAL_ERROR;
          }
          break;

        default:
          return DISKIO_ERROR_OPERATION_NOT_SUPPORTED;
          break;
      }
      break;
#endif /* SD_INIT */

#ifdef FLASH_INIT
    case DISKIO_DEVICE_TYPE_GENERIC_FLASH:
      switch (op) {
        case DISKIO_OP_READ_BLOCK:
          FLASH_READ_BLOCK(block_start_address, 0, buffer, 512);
          return DISKIO_SUCCESS;
          break;
        case DISKIO_OP_READ_BLOCKS:
          return DISKIO_ERROR_TO_BE_IMPLEMENTED;
          break;
        case DISKIO_OP_WRITE_BLOCK:
          FLASH_WRITE_BLOCK(block_start_address, 0, buffer, 512);
          return DISKIO_SUCCESS;
          break;
        // fake multi block write
        case DISKIO_OP_WRITE_BLOCKS_START:
          if (multi_block_nr != 0) {
            return DISKIO_ERROR_INTERNAL_ERROR;
          }
          multi_block_nr = block_start_address;
          return DISKIO_SUCCESS;
          break;
        case DISKIO_OP_WRITE_BLOCKS_NEXT:
          FLASH_WRITE_BLOCK(multi_block_nr, 0, buffer, 512);
          multi_block_nr++;
          return DISKIO_SUCCESS;
        case DISKIO_OP_WRITE_BLOCKS_DONE:
          multi_block_nr = 0;
          return DISKIO_SUCCESS;
          break;
        default:
          return DISKIO_ERROR_OPERATION_NOT_SUPPORTED;
          break;
      }
      break;
#endif /* FLASH_INIT */

    case DISKIO_DEVICE_TYPE_NOT_RECOGNIZED:
    default:
      return DISKIO_ERROR_NO_DEVICE_SELECTED;
  }
  return DISKIO_SUCCESS;
}
/*----------------------------------------------------------------------------*/
void
diskio_set_default_device(struct diskio_device_info *dev)
{
  default_device = dev;
}
/*----------------------------------------------------------------------------*/
struct diskio_device_info *
diskio_devices()
{
  return devices;
}
/*----------------------------------------------------------------------------*/
int
diskio_detect_devices()
{
  struct mbr mbr;
  int dev_num = 0;
  int i = 0, index = 0;

  memset(devices, 0, DISKIO_MAX_DEVICES * sizeof (struct diskio_device_info));

/** @todo Place definitions at proper position */
#ifndef FLASH_ARCH_NUM_SECTORS	
    /* This Flash has 4096 Pages */
#define FLASH_ARCH_NUM_SECTORS	4096
#endif
#ifndef FLASH_ARCH_SECTOR_SIZE
    /* A Page is 528 Bytes long, but for easier access we use only 512 Byte*/
#define FLASH_ARCH_SECTOR_SIZE	512
#endif

#ifdef FLASH_INIT
  if (FLASH_INIT() == 0) {
    devices[index].type = DISKIO_DEVICE_TYPE_GENERIC_FLASH;
    devices[index].number = dev_num;
    devices[index].num_sectors = FLASH_ARCH_NUM_SECTORS;
    devices[index].sector_size = FLASH_ARCH_SECTOR_SIZE;
    devices[index].first_sector = 0;
    index += 1;
  }
#endif /* FLASH_INIT */
  
#ifdef SD_INIT
  if (SD_INIT() == 0) {
    devices[index].type = DISKIO_DEVICE_TYPE_SD_CARD;
    devices[index].number = dev_num;
    devices[index].num_sectors = SD_GET_BLOCK_NUM();
    devices[index].sector_size = SD_GET_BLOCK_SIZE();
    devices[index].first_sector = 0;
    if (devices[index].sector_size > DISKIO_MAX_SECTOR_SIZE) {
      goto end_of_function;
    }
    
    mbr_init(&mbr);
    mbr_read(&devices[index], &mbr);
    index += 1;
    // test for max 5 partitions
    for (i = 0; i < 4; ++i) {
      if (mbr_hasPartition(&mbr, i + 1) != 0) {
        devices[index].type = DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION;
        devices[index].number = dev_num;
        devices[index].partition = i + 1;
        devices[index].num_sectors = mbr.partition[i].lba_num_sectors;
        devices[index].sector_size = devices[dev_num].sector_size;
        devices[index].first_sector = mbr.partition[i].lba_first_sector;
        index += 1;
      }
    }

    dev_num += 1;
    index += 1;
  }
#endif /* SD_INIT */

end_of_function:

  if (index == 0) {
    return DISKIO_FAILURE;
  }

  return DISKIO_SUCCESS;
}

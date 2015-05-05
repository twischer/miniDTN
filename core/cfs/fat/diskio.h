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
 */

/** \defgroup diskio_layer DiskIO Abstraction Layer
 *
 *	\note This interface was inspired by the diskio-interface of the FatFS-Module.
 *
 * <p> It's normally not important for Filesystem-Drivers to know, what type the underlying
 * storage device is. It's only important that the Driver can write and read on this device.
 * This abstraction layer enabled the filesystem drivers to do exactly that.</p>
 *
 * @{
 */

/**
 * \file
 *		DiskIO Layer definitions
 * \author
 * 		Original Source Code:
 * 		Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */
#ifndef _DISKIO_H_
#define _DISKIO_H_

/** Allows raw access to disks, used by the MBR-Subsystem */
#define DISKIO_DEVICE_TYPE_NOT_RECOGNIZED 0
#define DISKIO_DEVICE_TYPE_SD_CARD 1
#define DISKIO_DEVICE_TYPE_GENERIC_FLASH 2
#define DISKIO_DEVICE_TYPE_PARTITION 128

/** Bigger sectors then this are not supported. May be reduced down to 512 to use less memory. */
#define DISKIO_MAX_SECTOR_SIZE 512 

/** Mask used to ignore modifiers like the PARTITION flag */
#define DISKIO_DEVICE_TYPE_MASK 0x7f

#define DISKIO_SUCCESS 0
#define DISKIO_ERROR_NO_DEVICE_SELECTED 1
#define DISKIO_ERROR_INTERNAL_ERROR 2
#define DISKIO_ERROR_DEVICE_TYPE_NOT_RECOGNIZED 3
#define DISKIO_ERROR_OPERATION_NOT_SUPPORTED 4
#define DISKIO_ERROR_TO_BE_IMPLEMENTED 5
#define DISKIO_FAILURE 6
#define DISKIO_ERROR_TRY_AGAIN 7
#define DISKIO_ERROR_FATAL 8

#ifndef DISKIO_MAX_DEVICES
#define DISKIO_MAX_DEVICES 5
#endif

#define DISKIO_OP_WRITE_BLOCK  1
#define DISKIO_OP_READ_BLOCK   2
#define DISKIO_OP_WRITE_BLOCKS_START 10
#define DISKIO_OP_WRITE_BLOCKS_NEXT  11
#define DISKIO_OP_WRITE_BLOCKS_DONE  12
#define DISKIO_OP_READ_BLOCKS  4


#include <stdint.h>

/**
 * Stores the necessary information to identify a device using the diskio-Library.
 */
struct diskio_device_info {
  /** Specifies the recognized type of the memory */
  uint8_t type;
  /** Number to identify device */
  uint8_t number;
  /** Will be ignored if PARTITION flag is not set in type,
   * otherwise tells which partition of this device is meant */
  uint8_t partition;
  /** Number of sectors on this device */
  uint32_t num_sectors;
  /** How big is one sector in bytes */
  uint16_t sector_size;
  /** If this is a Partition, this indicates which is the
   * first_sector belonging to this partition on this device */
  uint32_t first_sector;
};

/**
 * Prints information about the specified device.
 * 
 * <b>Output Format:</b> \n
 * DiskIO Device Info\n
 * type = text \n
 * number = X\n
 * partition = X\n
 * num_sectors = X\n
 * sector_size = X\n
 * first_sector = X

 * \param *dev the pointer to the device info struct
 */
void diskio_print_device_info(struct diskio_device_info *dev);

/**
 * Reads one block from the specified device and stores it in buffer.
 *
 * \param *dev the pointer to the device info struct
 * \param block_address Which block should be read
 * \param *buffer buffer in which the data is written
 * \return DISKIO_SUCCESS on success, otherwise not 0 for an error
 */
int diskio_read_block(struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer);

/**
 * Reads multiple blocks from the specified device.
 *
 * This may or may not be supported for every device. DISKIO_ERROR_OPERATION_NOT_SUPPORTED will be
 * returned in that case.
 * \param *dev the pointer to the device info
 * \param block_start_address the address of the first block to be read
 * \param num_blocks the number of blocks to be read
 * \param *buffer buffer in which the data is written
 * \return DISKIO_SUCCESS on success, otherwise not 0 for an error
 */
int diskio_read_blocks(struct diskio_device_info *dev, uint32_t block_start_address, uint32_t num_blocks, uint8_t *buffer);

/**
 * Writes a single block to the specified device
 *
 * \param *dev the pointer to the device info
 * \param block_address address where the block should be written
 * \param *buffer buffer in which the data is stored
 * \return DISKIO_SUCCESS on success, !0 on error
 */
int diskio_write_block(struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer);

/**
 * Start writing multiple blocks to the specified device
 *
 * \param *dev the pointer to the device info
 * \param block_start_address the address of the first block to be written
 * \param num_blocks number of blocks to be written
 * \return DISKIO_SUCCESS on success, !0 on error
 */
int diskio_write_blocks_start(struct diskio_device_info *dev, uint32_t block_start_address, uint32_t num_blocks);

/**
 * Write next of multiple blocks to the specified device
 *
 * \param *dev the pointer to the device info
 * \param *buffer buffer in which the data is stored
 * \return DISKIO_SUCCESS on success, !0 on error
 */
int diskio_write_blocks_next(struct diskio_device_info *dev, uint8_t *buffer);

/**
 * Start writing multiple blocks to the specified device
 *
 * \param *dev the pointer to the device info
 * \return DISKIO_SUCCESS on success, !0 on error
 */
int diskio_write_blocks_done(struct diskio_device_info *dev);

/**
 * Returns the device-Database.
 *
 * Number of devices in the database is limited by the DISKIO_MAX_DEVICES define.
 * \return pointer to the Array of device infos. No available device entries have the type set to DISKIO_DEVICE_TYPE_NOT_RECOGNIZED.
 */
struct diskio_device_info * diskio_devices();

/**
 * Creates the internal database of available devices.
 *
 * Adds virtual devices for multiple Partitions on devices using the MBR-Library.
 * Number of devices in the database is limited by the DISKIO_MAX_DEVICES define.
 * Warning the device numbers may change when calling this function. It should be called
 * once on start but may also be called, if the microSD-Card was ejected to update the device
 * database.
 */
int diskio_detect_devices();

/**
 * Sets the default operation device.
 *
 * This allows to call the diskio_* functions with the dev parameter set to NULL.
 * The functions will then use the default device to operate.
 * \param *dev the pointer to the device info, which will be the new default device
 */
void diskio_set_default_device(struct diskio_device_info *dev);

#endif

/** @} */
/** @} */

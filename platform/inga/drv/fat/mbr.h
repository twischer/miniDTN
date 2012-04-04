/*
 * Copyright (c) 2012, Institute of Operating Systems and Computer Networks (TU Brunswick).
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
 * \addtogroup Drivers
 * @{
 *
 * \defgroup mbr_driver Master Boot Record Support
 *
 * <p></p>
 * @{
 *
 */

/**
 * \file
 *		MBR Support definitions
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */
 
#ifndef _MBR_H_
#define _MBR_H_

#include "diskio.h"

#define MBR_SUCCESS DISKIO_SUCCESS
#define MBR_ERROR_DISKIO_ERROR 1
#define MBR_ERROR_NO_MBR_FOUND 2
#define MBR_ERROR_INVALID_PARTITION 3
#define MBR_ERROR_PARTITION_EXISTS 4

#define MBR_PARTITION_TYPE_FAT32_LBA 0x0C
#define MBR_PARTITION_TYPE_FAT16_LBA 0x0E

/**
 * Represents a primary partition in the mbr.
 */
struct mbr_primary_partition {
	/** 0x00 non-bootable, 0x80 bootable, other values indicate that it is invalid */
	uint8_t status;
	/**
	 * Saves the first sector of the partition in the old CHS format (cylinder, head, sector)
	 *
	 * First byte stores the head value.
	 * Second byte stores the sector in the positions 0 - 5 and the upper to bits of the cylinder (Bits 8 and 9)
	 * Third byte stores the lower 8 bits of the cylinder value (0 - 7)
	 */
	uint8_t chs_first_sector[3];
	/** The type of partition this is, like FAT16 or FAT32; type = 0 means it's unused*/
	uint8_t type;
	/** Stores the last sector of the partition in the old CHS format */
	uint8_t chs_last_sector[3];
	/** Stores the absolute address of the first sector of the partition */
	uint32_t lba_first_sector;
	/** Stores the number of sectors the partition is long */
	uint32_t lba_num_sectors;
};

/**
 * Represents the MBR of a disk, without the code portion and the constant bytes
 */
struct mbr { //ignores everything but primary partitions (saves 448 bytes)
	/** The MBR supports max 4 Primary partitions.
	 * For the sake of simplicity Extended partitions are not implemented.
	 */
	struct mbr_primary_partition partition[4];
};

/**
 * Initializes a mbr structure.
 *
 * Should be called first on a new mbr structure.
 * \param *mbr the mbr which should be initialized
 * \param disk_size the size of the disk on which the mbr will reside
 */
void mbr_init( struct mbr *mbr );

/**
 * Reads the MBR from the specified device.
 *
 * The MBR is 512 Bytes long. That is normally one block to be read.
 * \param *from device from which the mbr is read
 * \param *to whe mbr structure in which the data is parsed
 * \return MBR_SUCCESS on success, MBR_ERROR_DISKIO_ERROR if there was a problem reading the block or MBR_ERROR_NO_MBR_FOUND when there is no MBR on this device.
 */
int mbr_read( struct diskio_device_info *from, struct mbr *to );

/**
 * Write the MBR to the specified device.
 *
 * \param *from the mbr structure which should be written on the device
 * \param *to the device pointer to which we will write the mbr
 * \return MBR_SUCCESS on success, !0 on a diskio error which is returned, see diskio_write_block for more information
 */
int mbr_write( struct mbr *from, struct diskio_device_info *to );

/**
 * Adds a Partition to the mbr-structure.
 *
 * \param *mbr The mbr-structure in which to insert the partition.
 * \param part_num Number of the Partition which should be added.
 * \param part_type Type of the partition.
 * \param start LBA-style start of the partition.
 * \param len LBA-style length of the partition.
 * \return MBR_SUCCESS on success or MBR_ERROR_PARTITION_EXISTS.
 */
int mbr_addPartition(struct mbr *mbr, uint8_t part_num, uint8_t part_type, uint32_t start, uint32_t len );

/**
 * Deletes a Partition from the mbr-structure.
 *
 * \param *mbr The mbr-structure which should be edited.
 * \param part_num Number of the partition which should be removed.
 * \return MBR_SUCCESS on success or MBR_ERROR_INVALID_PARTITION.
 */
int mbr_delPartition(struct mbr *mbr, uint8_t part_num );

/**
 * Checks if the given Partition exists.
 *
 * \param part_num Number of the Partition which should be checked.
 * \return 1 if there is a Partition with the given number, otherwise 0.
 */
int mbr_hasPartition(struct mbr *mbr, uint8_t part_num );

#endif

/*
 * Copyright (c) 2012, (TU Braunschweig).
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
/**
 * \defgroup fat_driver FAT Driver
 *
 * Additional function for the FAT file system.
 * 
 * @{
 */

/**
 * \file
 *		FAT driver definitions
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */

#ifndef _FAT_H_
#define _FAT_H_

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

/** Seek type is 32 bit for FAT files. */
#ifndef CFS_CONF_OFFSET_TYPE
#define CFS_CONF_OFFSET_TYPE uint32_t
#endif

#include "diskio.h"
#include "cfs/cfs.h"


#ifdef FAT_CONF_COOPERATIVE
#define FAT_COOPERATIVE FAT_CONF_COOPERATIVE
#else
#undef FAT_COOPERATIVE
#endif

/** Allows to enable synchronization of FATs when unmounting device.
 * This may allow to restore a corrupted primary FAT but
 * note that this will lead to poor performance and decreases flash life because
 * of a remarkable number of additional write cycles.
 */
#ifndef FAT_SYNC
#define FAT_SYNC 0
#endif

#define FAT_COOP_QUEUE_SIZE 15

#define FAT12 0
#define FAT16 1
#define FAT32 2
#define FAT_INVALID 3

#define EOC 0x0FFFFFFF

/* Maximum number of files that can be opened simulatneously */
#ifndef FAT_FD_POOL_SIZE
#define FAT_FD_POOL_SIZE 5
#endif

/** Holds boot sector information. */
struct FAT_Info {
  uint8_t type; /** Either FAT16, FAT32 or FAT_INVALID */
  uint16_t BPB_BytesPerSec;
  uint8_t BPB_SecPerClus;
  uint16_t BPB_RsvdSecCnt;
  uint8_t BPB_NumFATs;
  uint16_t BPB_RootEntCnt;
  uint32_t BPB_TotSec;
  uint8_t BPB_Media; /*! obsolete */
  uint32_t BPB_FATSz; /*! Number of sectors per FAT, i.e. size of FAT */
  uint32_t BPB_RootClus; /** only valid for FAT32 */
};
/** Fat table entry for file */
struct dir_entry {
  uint8_t DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t DIR_NTRes;
  uint8_t CrtTimeTenth;
  /** Create time [15-11: hours, 10-5: minutes, 4-0: seconds/2] */
  uint16_t DIR_CrtTime;
  /** Create date [15-9: year sine 1980, 8-5: month, 4-0: day] */
  uint16_t DIR_CrtDate;
  /** Last access date [15-9: year sine 1980, 8-5: month, 4-0: day] */
  uint16_t DIR_LstAccessDate;
  uint16_t DIR_FstClusHI;
  /** Last modified time [15-11: hours, 10-5: minutes, 4-0: seconds/2] */
  uint16_t DIR_WrtTime;
  /** Create date [15-9: year sine 1980, 8-5: month, 4-0: day] */
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClusLO;
  /** Size of file in [byte] */
  uint32_t DIR_FileSize;
};

struct file {
  //metadata
  /** Cluster Position on disk */
  uint32_t cluster;
  uint32_t dir_entry_sector;
  uint16_t dir_entry_offset;
  struct dir_entry dir_entry;
  uint32_t nth_cluster;
  uint32_t n;
};

struct file_desc {
  //cfs_offset_t offset;
  uint32_t offset;
  struct file *file;
  uint8_t flags;
};

//int cfs_fat_rmdir(char *);
//int cfs_fat_mkdir(char *);

/**
 * Formats the specified device as FAT16/32
 * 
 * @param dev device to format
 * @return 
 */
int cfs_fat_mkfs(struct diskio_device_info *dev);

/**
 * Tries to mount the defined device.
 *
 * \param dev The device on which a FAT-FS should be mounted.
 * \return 0 on success, 1 if the bootsector was not found or corrupted,
 * 2 if the FAT-Type wasn't supported.
 */
uint8_t cfs_fat_mount_device(struct diskio_device_info *dev);

/**
 * Umounts the mounted device. Invalidates all file descriptors.
 * (Syncs all FATs (only if FAT_SYNC is set)) and flushes cached data.
 */
void cfs_fat_umount_device();

/**
 * Populates the given FAT_Info with the mounted FAT_Info.
 *
 * \param *info The FAT_Info struct which should be populated.
 */
void cfs_fat_get_fat_info(struct FAT_Info *info);

/**
 * Returns the date of last modification.
 * 
 * Format: [15-9: year sine 1980, 8-5: month, 4-0: day]
 * @param fd File descriptor
 */

uint16_t cfs_fat_get_last_date(int fd);

/**
 * Returns the time of last modification.
 * 
 * Format: [15-11: hours, 10-5: minutes, 4-0: seconds/2]
 * @param fd File descriptor
 */
uint16_t cfs_fat_get_last_time(int fd);

/**
 * Returns the date of creation.
 * 
 * Format: [15-9: year sine 1980, 8-5: month, 4-0: day]
 * @param fd File descriptor
 */
uint16_t cfs_fat_get_create_date(int fd);

/**
 * Returns the time of creation.
 * 
 * Format: [15-11: hours, 10-5: minutes, 4-0: seconds/2]
 * @param fd File descriptor
 */
uint16_t cfs_fat_get_create_time(int fd);

/**
 * Syncs every FAT with the first FAT. Can take much time.
 */
void cfs_fat_sync_fats();

/**
 * Writes the current buffered block back to the disk if it was changed.
 */
void cfs_fat_flush();

/**
 * Returns the file size of the associated file
 * 
 * @param fd
 * @return 
 */
uint32_t cfs_fat_file_size(int fd);

/**
 * 
 * @param fd
 */
void cfs_fat_print_cluster_chain(int fd);

/**
 * 
 * @param fd
 */
void cfs_fat_print_file_info(int fd);

/**
 * 
 * @param dir_entry
 */
void cfs_fat_print_dir_entry(struct dir_entry *dir_entry);


uint8_t is_a_power_of_2(uint32_t value);
uint32_t round_down_to_power_of_2(uint32_t value);

#endif

/** @} */
/** @} */

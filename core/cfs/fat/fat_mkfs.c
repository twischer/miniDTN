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
 * \addtogroup fat_driver
 * @{
 */

/**
 * \file
 *      FAT driver mkfs implementation
 * \author
 *      Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 *      Enrico Joerns <joerns.ibr.cs.tu-bs.de>
 */

#include "cfs-fat.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* specifies divisor of fat size to erase during mkfs, because erasing the whole FAT
 * may take some time.
 * e.g. FAT_ERASE_DIV set to 4 means 1/4 of the fat will be erased.
 */
#define FAT_ERASE_DIV 2


static int mkfs_write_boot_sector(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi);
static void mkfs_write_fats(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi);
static void mkfs_write_fsinfo(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi);
static void mkfs_write_root_directory(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi);
static uint8_t mkfs_calc_cluster_size(uint16_t sec_size, uint16_t bytes);
static uint16_t mkfs_determine_fat_type_and_SPC(uint32_t total_sec_count, uint16_t bytes_per_sec);
static uint32_t mkfs_compute_fat_size(struct FAT_Info *fi);
/*----------------------------------------------------------------------------*/
int
cfs_fat_mkfs(struct diskio_device_info *dev)
{
  uint8_t buffer[512];
  struct FAT_Info fi;
  int ret = 0;

  PRINTF("\nWriting boot sector ... ");

  memset(buffer, 0, 512);
  ret = mkfs_write_boot_sector(buffer, dev, &fi);

  if (ret != 0) {
    PRINTF("Error(%d).", ret);
    return 1;
  } else {
    PRINTF("done.");
  }
  
  PRINTF("\nWriting FATs ... ");
  mkfs_write_fats(buffer, dev, &fi);

  if (fi.type == FAT32) {
    PRINTF("done.\nWriting FSINFO ... ");
    mkfs_write_fsinfo(buffer, dev, &fi);
  }
  PRINTF("done.\n");

  PRINTF("Writing root directory ... ");
  mkfs_write_root_directory(buffer, dev, &fi);
  PRINTF("done.");

  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * Determines the number of sectors per cluster.
 *
 * \param sec_size The size of one sector in bytes.
 * \param bytes number of bytes one cluster should be big
 * \return 1,2,4,8,16,32,64,128 depending on the number of sectors per cluster
 */
static uint8_t
mkfs_calc_cluster_size(uint16_t sec_size, uint16_t bytes)
{
  uint8_t SecPerCluster = 0;

  SecPerCluster = (uint8_t) (bytes / sec_size);

  if (SecPerCluster == 0) {
    return 1;
  }

  if (is_a_power_of_2(SecPerCluster) != 0) {
    SecPerCluster = (uint8_t) round_down_to_power_of_2(SecPerCluster);
  }

  if (SecPerCluster > 128 || SecPerCluster * sec_size > 32 * ((uint32_t) 1024)) {
    return 1;
  }

  return SecPerCluster;
}
/*----------------------------------------------------------------------------*/
/**
 * Determines for mkfs which FAT-Type and cluster-size should be used.
 *
 * This is based upon the FAT specification, which states that the table only works for 512
 * Bytes sized sectors. For that the function SPC is used to compute this right. but
 * it probalby only works for powers of 2 in the bytes_per_sec field. It tries to use FAT16
 * if at all possible, FAT32 only when there is no other choice.
 * \param total_sec_count the total number of sectors of the filesystem
 * \param bytes_per_sec how many bytes are there in one sector
 * \return in the upper two bits is the fat type, either FAT12, FAT16, FAT32 or FAT_INVALID. In the lower 8 bits is the number of sectors per cluster.
 */
static uint16_t
mkfs_determine_fat_type_and_SPC(uint32_t total_sec_count, uint16_t bytes_per_sec)
{
  uint64_t vol_size = (total_sec_count * bytes_per_sec) / 512;

  if (vol_size < ((uint32_t) 8400)) {
    return (FAT16 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 512);
  } else if (vol_size < 32680) {
    return (FAT16 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 1024);
  } else if (vol_size < 262144) {
    return (FAT16 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 2048);
  } else if (vol_size < 524288) {
    return (FAT16 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 4096);
  } else if (vol_size < 1048576) {
    return (FAT16 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 8192);
  } else if (vol_size < 2097152) {
    return (FAT16 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 16384);
  } else if (vol_size < 4194304L) {
    return (FAT32 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 2048);
  }

  if (vol_size < 16777216) {
    return (FAT32 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 4096);
  } else if (vol_size < 33554432) {
    return (FAT32 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 8192);
  } else if (vol_size < 67108864) {
    return (FAT32 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 16384);
  } else if (vol_size < 0xFFFFFFFF) {
    return (FAT32 << 8) +mkfs_calc_cluster_size(bytes_per_sec, 32768);
  } else {
    return (FAT_INVALID << 8) + 0;
  }
}
/*----------------------------------------------------------------------------*/
/**
 * Computes the required size of one FAT according to standard formula.
 *
 * \param fi FAT_Info structure that must contain BPB_RootEntCnt, BPB_BytesPerSec, BPB_TotSec, BPB_RsvdSecCnt, BPB_NumFATs and type.
 * \return the required size of one FAT [sectors] for this FS.
 */
static uint32_t
mkfs_compute_fat_size(struct FAT_Info *fi)
{
  uint32_t FATSz = 0;
  uint16_t RootDirSectors = ((fi->BPB_RootEntCnt * 32) + (fi->BPB_BytesPerSec - 1)) / fi->BPB_BytesPerSec;
  uint32_t TmpVal1 = fi->BPB_TotSec - (fi->BPB_RsvdSecCnt + RootDirSectors);
  uint32_t TmpVal2 = (256UL * fi->BPB_SecPerClus) + fi->BPB_NumFATs;

  if (fi->type == FAT32) {
    TmpVal2 /= 2;
  }

  FATSz = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;

  PRINTF("\nFat size: %ld", FATSz);
  return FATSz;
}
/*----------------------------------------------------------------------------*/
static int
mkfs_write_boot_sector(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi)
{
  // Test if we can make FAT16 or FAT32
  uint16_t type_SPC = mkfs_determine_fat_type_and_SPC(dev->num_sectors, dev->sector_size);
  uint8_t sectors_per_cluster = (uint8_t) type_SPC;

  fi->BPB_FATSz = 0;
  fi->type = (uint8_t) (type_SPC >> 8);

  PRINTF("\nA: SPC = %u; type = %u; dev->num_sectors = %lu", sectors_per_cluster, fi->type, dev->num_sectors);
  if (fi->type == FAT12 || fi->type == FAT_INVALID) {
    return -1;
  }

  // BS_jmpBoot
  buffer[0x000] = 0x00;
  buffer[0x001] = 0x00;
  buffer[0x002] = 0x00;

  // BS_OEMName
  memcpy(&(buffer[0x03]), "cfs-mkfs", 8);

  // BPB_BytesPerSec
  buffer[0x00B] = (uint8_t) dev->sector_size;
  buffer[0x00C] = (uint8_t) (dev->sector_size >> 8);
  fi->BPB_BytesPerSec = dev->sector_size;

  // BPB_SecPerClus
  buffer[0x00D] = sectors_per_cluster;
  fi->BPB_SecPerClus = sectors_per_cluster;

  //BPB_RsvdSecCnt
  if (fi->type == FAT16) {
    buffer[0x00E] = 1;
    buffer[0x00F] = 0;
    fi->BPB_RsvdSecCnt = 1;
  } else if (fi->type == FAT32) {
    buffer[0x00E] = 32;
    buffer[0x00F] = 0;
    fi->BPB_RsvdSecCnt = 32;
  }

  // BPB_NumFATs
  buffer[0x010] = 2;
  fi->BPB_NumFATs = 2;

  // BPB_RootEntCnt
  if (fi->type == FAT16) {
    buffer[0x011] = (uint8_t) 512;
    buffer[0x012] = (uint8_t) (512 >> 8);
    fi->BPB_RootEntCnt = 512;
  } else if (fi->type == FAT32) {
    buffer[0x011] = 0;
    buffer[0x012] = 0;
    fi->BPB_RootEntCnt = 0;
  }

  // BPB_TotSec16
  if (dev->num_sectors < 0x10000) {
    buffer[0x013] = (uint8_t) dev->num_sectors;
    buffer[0x014] = (uint8_t) (dev->num_sectors >> 8);
  } else {
    buffer[0x013] = 0;
    buffer[0x014] = 0;
  }
  fi->BPB_TotSec = dev->num_sectors;

  // BPB_Media
  buffer[0x015] = 0xF8;
  fi->BPB_Media = 0xF8;

  // BPB_FATSz16
  fi->BPB_FATSz = mkfs_compute_fat_size(fi);

  if (fi->type == FAT16 && fi->BPB_FATSz < 0x10000) {
    buffer[0x016] = (uint8_t) fi->BPB_FATSz;
    buffer[0x017] = (uint8_t) (fi->BPB_FATSz >> 8);
  } else if (fi->type == FAT16) {
    PRINTF("B: FATSz = %lu", fi->BPB_FATSz);
    return -1;
  } else {
    // unused in FAT32 -> see offset 24h
    buffer[0x016] = 0;
    buffer[0x017] = 0;
  }

  // BPB_SecPerTrk
  buffer[0x018] = 0;
  buffer[0x019] = 0;

  // BPB_NumHeads
  buffer[0x01A] = 0;
  buffer[0x01B] = 0;

  // BPB_HiddSec
  buffer[0x01C] = 0;
  buffer[0x01D] = 0;
  buffer[0x01E] = 0;
  buffer[0x01F] = 0;

  // BPB_TotSec32
  if (dev->num_sectors < 0x10000) {
    buffer[0x020] = 0;
    buffer[0x021] = 0;
    buffer[0x022] = 0;
    buffer[0x023] = 0;
  } else {
    buffer[0x020] = (uint8_t) fi->BPB_TotSec;
    buffer[0x021] = (uint8_t) (fi->BPB_TotSec >> 8);
    buffer[0x022] = (uint8_t) (fi->BPB_TotSec >> 16);
    buffer[0x023] = (uint8_t) (fi->BPB_TotSec >> 24);
  }

  if (fi->type == FAT16) {
    // BS_DrvNum
    buffer[0x024] = 0x80;

    // BS_Reserved1
    buffer[0x025] = 0;

    // BS_BootSig
    buffer[0x026] = 0x29;

    // BS_VolID
    buffer[0x027] = 0;
    buffer[0x028] = 0;
    buffer[0x029] = 0;
    buffer[0x02A] = 0;

    // BS_VolLab
    memcpy(&(buffer[0x02B]), "NO NAME    ", 11);

    // BS_FilSysType
    memcpy(&(buffer[0x036]), "FAT16   ", 8);
  } else if (fi->type == FAT32) {
    // BPB_FATSz32
    buffer[0x024] = (uint8_t) fi->BPB_FATSz;
    buffer[0x025] = (uint8_t) (fi->BPB_FATSz >> 8);
    buffer[0x026] = (uint8_t) (fi->BPB_FATSz >> 16);
    buffer[0x027] = (uint8_t) (fi->BPB_FATSz >> 24);

    // BPB_ExtFlags
    buffer[0x028] = 0;
    buffer[0x029] = 0; // Mirror enabled, TODO:...

    // BPB_FSVer
    buffer[0x02A] = 0;
    buffer[0x02B] = 0;

    // BPB_RootClus, cluster number of root cluster (typically 2)
    buffer[0x02C] = 2;
    buffer[0x02D] = 0;
    buffer[0x02E] = 0;
    buffer[0x02F] = 0;

    // BPB_FSInfo
    buffer[0x030] = 1;
    buffer[0x031] = 0;

    // BPB_BkBootSec, set to 0 as we do not create a boot sectors copy yet
    // TODO: maybe we should create it, as it does not require to much effort?
    buffer[0x032] = 0x00;
    buffer[0x033] = 0x00;

    // BPB_Reserved
    memset(&(buffer[0x034]), 0, 12);

    // BS_DrvNum
    buffer[0x040] = 0x80;

    // BS_Reserved1
    buffer[0x041] = 0;

    // BS_BootSig
    buffer[0x042] = 0x29;

    // BS_VolID
    buffer[0x043] = 0;
    buffer[0x044] = 0;
    buffer[0x045] = 0;
    buffer[0x046] = 0;

    // BS_VolLab
    memcpy(&(buffer[0x047]), "NO NAME    ", 11);

    // BS_FilSysType
    memcpy(&(buffer[0x052]), "FAT32   ", 8);
  }

  // Specification demands this values at this precise positions
  buffer[0x1FE] = 0x55;
  buffer[0x1FF] = 0xAA;

  diskio_write_block(dev, 0, buffer);

  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * Writes the FAT-Portions of the FS.
 *
 * \param buffer Buffer / 2 and buffer / 4 should be integers.
 * \param dev the Device where the FATs are written to
 */
static void
mkfs_write_fats(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi)
{
  uint32_t *fat32_buf = (uint32_t *) buffer;
  uint16_t *fat16_buf = (uint16_t *) buffer;
  uint32_t i = 0;
#if FAT_SYNC
  uint32_t j = 0;
#endif

  memset(buffer, 0x00, 512);
  if (fi->type == FAT32) {
    // BPB_Media Copy
    fat32_buf[0] = 0x0FFFFF00 + fi->BPB_Media;
    // End of Clusterchain marker 
    fat32_buf[1] = 0x0FFFFFFF;
    // End of Clusterchain marker for root directory at cluster 2
    fat32_buf[2] = 0x0FFFFFF8;
  } else {
    // BPB_Media Copy
    fat16_buf[0] = 0xFF00 + fi->BPB_Media;
    // End of Clusterchain marker
    fat16_buf[1] = 0xFFF8;
  }

  // Write first sector of the FATs
  diskio_write_block(dev, fi->BPB_RsvdSecCnt, buffer);
#if FAT_SYNC
  // Write first sector of the secondary FAT(s)
  for (j = 1; j < fi->BPB_NumFATs; ++j) {
    diskio_write_block(dev, fi->BPB_RsvdSecCnt + fi->BPB_FATSz * j, buffer);
  }
#endif

  // Reset previously written first 96 bits = 12 Bytes of the buffer
  memset(buffer, 0x00, 12);

  // Write additional Sectors of the FATs
  diskio_write_blocks_start(dev, fi->BPB_RsvdSecCnt + 1, fi->BPB_FATSz / 2);
  for (i = 1; i < fi->BPB_FATSz / FAT_ERASE_DIV; ++i) {
    watchdog_periodic();
    diskio_write_blocks_next(dev, buffer);
    //diskio_write_block(dev, fi->BPB_RsvdSecCnt + i, buffer);
  }
  diskio_write_blocks_done(dev);

#if FAT_SYNC
  // Write additional Sectors of the secondary FAT(s)
  for (j = 1; j < fi->BPB_NumFATs; ++j) {
    for (i = 1; i < fi->BPB_FATSz; ++i) {
      diskio_write_block(dev, fi->BPB_RsvdSecCnt + i + fi->BPB_FATSz * j, buffer);
    }
  }
#endif
}
/*----------------------------------------------------------------------------*/
static void
mkfs_write_fsinfo(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi)
{
  uint32_t fsi_free_count = 0;
  uint32_t fsi_nxt_free = 0;

  memset(buffer, 0, 512);
  // FSI_LeadSig
  buffer[0] = 0x52;
  buffer[1] = 0x52;
  buffer[2] = 0x61;
  buffer[3] = 0x41;

  // FSI_Reserved1

  // FSI_StrucSig
  buffer[484] = 0x72;
  buffer[485] = 0x72;
  buffer[486] = 0x41;
  buffer[487] = 0x61;

  // FSI_Free_Count
  fsi_free_count = (fi->BPB_TotSec - ((fi->BPB_FATSz * fi->BPB_NumFATs) + fi->BPB_RsvdSecCnt)) / fi->BPB_SecPerClus;
  buffer[488] = (uint8_t) fsi_free_count;
  buffer[489] = (uint8_t) (fsi_free_count >> 8);
  buffer[490] = (uint8_t) (fsi_free_count >> 16);
  buffer[491] = (uint8_t) (fsi_free_count >> 24);

  // FSI_Nxt_Free
  fsi_nxt_free = (fi->BPB_RsvdSecCnt + (fi->BPB_NumFATs * fi->BPB_FATSz));
  if (fsi_nxt_free % fi->BPB_SecPerClus) {
    fsi_nxt_free = (fsi_nxt_free / fi->BPB_SecPerClus) + 1;
  } else {
    fsi_nxt_free = (fsi_nxt_free / fi->BPB_SecPerClus);
  }
  buffer[492] = (uint8_t) fsi_nxt_free;
  buffer[493] = (uint8_t) (fsi_nxt_free >> 8);
  buffer[494] = (uint8_t) (fsi_nxt_free >> 16);
  buffer[495] = (uint8_t) (fsi_nxt_free >> 24);

  // FSI_Reserved2

  // FSI_TrailSig
  buffer[508] = 0x00;
  buffer[509] = 0x00;
  buffer[510] = 0x55;
  buffer[511] = 0xAA;

  diskio_write_block(dev, 1, buffer);
}
/*----------------------------------------------------------------------------*/
static void
mkfs_write_root_directory(uint8_t *buffer, struct diskio_device_info *dev, struct FAT_Info *fi)
{
  // TODO: respect root cluster entry in boot sector
  uint16_t i;
  uint32_t firstDataSector = fi->BPB_RsvdSecCnt + (fi->BPB_NumFATs * fi->BPB_FATSz);// TODO: + RootDirSectors
  memset(buffer, 0x00, 512);
  
  // clear first root cluster
  for (i = 0; i < fi->BPB_SecPerClus; i++) {
    diskio_write_block(dev, firstDataSector + i, buffer);
  }
}
/*----------------------------------------------------------------------------*/

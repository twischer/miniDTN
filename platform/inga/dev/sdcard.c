/*
 * Copyright (c) 2013, Institute of Operating Systems and Computer Networks (TU Braunschweig).
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
 * \addtogroup inga_device_driver
 * @{
 *
 * \addtogroup sdcard_interface
 * @{
 */

/**
 * \file
 *      SD Card interface implementation
 * \author
 * 	Original source: Ulrich Radig
 * 	<< modified by >>
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *	Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 *	Enrico Joerns <e.joerns@tu-bs.de>
 */

#include "sdcard.h"
#include "dev/watchdog.h"
#include <util/delay.h>

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define PRINTD(...) printf(__VA_ARGS__)
#else
#define PRINTD(...)
#endif
// DEBUG > 1 is verbose
#if DEBUG > 1
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define SD_DATA_HIGH  0xFF

/** CMD0  -- GO_IDLE_STATE */
#define SDCARD_CMD0   0
/** CMD1  -- SEND_OP_COND */
#define SDCARD_CMD1   1
/** CMD8  -- SEND_IF_COND (deprecated?)*/
#define SDCARD_CMD8   8

/** CMD9  -- SEND_CSD */
#define SDCARD_CMD9   9
/** CMD10 -- SEND_CID */
#define SDCARD_CMD10  10
/** CMD13 -- SEND_STATUS */
#define SDCARD_CMD13  13

/** CMD16 -- SET_BLOCKLEN */
#define SDCARD_CMD16  16
// -- Read commands
/** CMD17 -- READ_SINGLE_BLOCK */
#define SDCARD_CMD17  17
/** CMD18 -- READ_MULTIPLE_BLOCK */
#define SDCARD_CMD18  18
// -- Write commands
/** CMD24 -- WRITE_BLOCK */
#define SDCARD_CMD24  24
/** CMD25 -- WRITE_MULTIPLE_BLOCK */
#define SDCARD_CMD25  25
// --
/** CMD27 -- PROGRAM_CSD */
#define SDCARD_CMD27  27
// -- Erase commands
/** CMD32 -- ERASE_WR_BLK_START_ADDR */
#define SDCARD_CMD32  32
/** CMD33 -- ERASE_WR_BLK_END_ADDR */
#define SDCARD_CMD33  33
/** CMD38 -- ERASE */
#define SDCARD_CMD38  38
// --
/** CMD58 -- READ_OCR */
#define SDCARD_CMD58  58
/** CMD59 -- CRC_ON_OFF */
#define SDCARD_CMD59  59

/** CMD55 -- APP_CMD */
#define SDCARD_CMD55  55

#define IS_ACMD       0x80
/** ACMD23 -- SET_WR_BLK_ERASE_COUNT */
#define SDCARD_ACMD23 (IS_ACMD | 23)
/** ACMD41 -- SD_SEND_OP_COND */
#define SDCARD_ACMD41 (IS_ACMD | 41)

/* Response types */
#define SDCARD_RESP1   0x01
#define SDCARD_RESP2   0x02
#define SDCARD_RESP3   0x03
#define SDCARD_RESP7   0x07

/* Response R1 bits */
#define SD_R1_IN_IDLE_STATE 0x01
#define SD_R1_ERASE_RESET   0x02
#define SD_R1_ILLEGAL_CMD   0x04
#define SD_R1_COM_CRC_ERR   0x08
#define SD_R1_ERASE_SEQ_ERR 0x10
#define SD_R1_ADDR_ERR      0x20
#define SD_R1_PARAM_ERR     0x40

/* Response R2 bits */
#define R2_OUT_OF_RANGE   0x80
#define R2_ERASE_PARAM    0x40
#define R2_WP_VIOLATION   0x20
#define R2_EEC_FAILED     0x10
#define R2_CC_ERROR       0x08
#define R2_ERROR          0x04
#define R2_ERASE_SKIP     0x02
#define R2_CARD_LOCKED    0x01

/* OCR register bits */
#define OCR_H_CCS       0x40
#define OCR_H_POWER_UP  0x80

/* Data response tokens (sent by card) */
#define DATA_RESP_ACCEPTED    0x05
#define DATA_RESP_CRC_REJECT  0x0B
#define DATA_RESP_WRITE_ERROR 0x0D

/* Single-Block Read, Single-Block Write and Multiple-Block Read
 * (sent both by host and by card) */
#define START_BLOCK_TOKEN         0xFE
/* Multiple Block Write Operation (sent by host) */
#define MULTI_START_BLOCK_TOKEN   0xFC
#define STOP_TRAN_TOKEN           0xFD

/* Data error token (sent by card) */
#define DATA_ERROR            0x01
#define DATA_CC_ERROR         0x02
#define DATA_CARD_ECC_FAILED  0x04
#define DATA_OUT_OF_RANGE     0x08

/* Configures the number of ms to busy wait
 * Note: Standard says, maximum busy for write must be <250ms.
 *       Only in context of multi-block write stop
 *       also 500ms are allowed. */
#define BUSY_WAIT_MS  300

#define N_CX_MAX  8
#define N_CR_MAX  8

/**
 * \brief The number of bytes in one block on the SD-Card.
 */
static uint16_t sdcard_block_size = 512;

/**
 * \brief Number of blocks on the SD-Card.
 */
static uint32_t sdcard_card_block_count = 0;

/**
 * \brief Indicates if the inserted card is a SDSC card (!=0) or not (==0).
 */
static uint8_t sdcard_sdsc_card = 1;

/**
 * \brief Indicates if the cards CRC mode is on (1) or off (0). Default is off.
 */
static uint8_t sdcard_crc_enable = 0;

static void get_csd_info(uint8_t *csd);
/**
 * Waits for the busy signal to become high.
 * \retval 0 successfull
 */
static uint8_t sdcard_busy_wait();

/* Debugging functions */
#if DEBUG
static void
print_r1_resp(uint8_t cmd, uint8_t rsp) {
  /* everything might be ok */
  if (rsp == 0x00) {
    PRINTF("\nCMD%d: ", cmd & ~IS_ACMD);
    PRINTF("\n\tOk");
    return;
  /* idle is not really critical */
  } else if (rsp == SD_R1_IN_IDLE_STATE) {
    PRINTF("\nCMD%d: ", cmd & ~IS_ACMD);
    PRINTF("\n\tIdle");
    return;
  /* let us see what kind of error(s) we have */
  } else {
    PRINTD("\nCMD%d: ", cmd & ~IS_ACMD);
    if (rsp & SD_R1_IN_IDLE_STATE) {
      PRINTD("\n\tIdle");
    }
    if (rsp & SD_R1_ERASE_RESET) {
      PRINTD("\n\tErase Reset");
    }
    if (rsp & SD_R1_ILLEGAL_CMD) {
      PRINTD("\n\tIllegal Cmd");
    }
    if (rsp & SD_R1_COM_CRC_ERR) {
      PRINTD("\n\tCom CRC Err");
    }
    if (rsp & SD_R1_ERASE_SEQ_ERR) {
      PRINTD("\n\tErase Seq Err");
    }
    if (rsp & SD_R1_ADDR_ERR) {
      PRINTD("\n\tAddr Err");
    }
    if (rsp & SD_R1_PARAM_ERR) {
      PRINTD("\n\tParam Err");
    }
  }
}
/*----------------------------------------------------------------------------*/
static void
dbg_resp_r1(uint8_t cmd, uint8_t rsp) {
  print_r1_resp(cmd, rsp);
}
/*----------------------------------------------------------------------------*/
static void
dbg_resp_r2(uint8_t cmd, uint8_t* rsp) {
  print_r1_resp(cmd, rsp[0]);
  if(rsp[1] & R2_OUT_OF_RANGE) {
    PRINTD("\nOut of Range");
  }
  if(rsp[1] & R2_ERASE_PARAM) {
    PRINTD("\nErase param");
  }
  if(rsp[1] & R2_WP_VIOLATION) {
    PRINTD("\nWP violation");
  }
  if(rsp[1] & R2_EEC_FAILED) {
    PRINTD("\nEEC failed");
  }
  if(rsp[1] & R2_CC_ERROR) {
    PRINTD("\nCC error");
  }
  if(rsp[1] & R2_ERROR) {
    PRINTD("\nError");
  }
  if(rsp[1] & R2_ERASE_SKIP) {
    PRINTD("\nErase skip");
  }
  if(rsp[1] & R2_CARD_LOCKED) {
    PRINTD("\nCard locked");
  }
}
/*----------------------------------------------------------------------------*/
static void
dbg_resp_r3(uint8_t cmd, uint8_t* rsp) {
  print_r1_resp(cmd, rsp[0]);
  PRINTF("\nOCR: 0x%04x", rsp[1]);
}
/*----------------------------------------------------------------------------*/
static void
dbg_resp_r7(uint8_t cmd, uint8_t* rsp) {
  print_r1_resp(cmd, rsp[0]);
  PRINTF("\nR7_DATA: 0x%04x", rsp[1]);
}
/*----------------------------------------------------------------------------*/
static void
dbg_data_err(uint8_t ret) {
  if ((ret & 0xF0) == 0) { // error bit mask
    if(ret & DATA_ERROR) {
      PRINTD("\nError");
    }
    if(ret & DATA_CC_ERROR) {
      PRINTD("\nCC Error");
    }
    if(ret & DATA_CARD_ECC_FAILED) {
      PRINTD("\nEEC Failed");
    }
    if(ret & DATA_OUT_OF_RANGE) {
      PRINTD("\nOut of range");
    }
  }
}
/*----------------------------------------------------------------------------*/
#else
/* dummy fuctions */
#define dbg_resp_r1(a, b)
#define dbg_resp_r2(a, b)
#define dbg_resp_r3(a, b)
#define dbg_resp_r7(a, b)
#define dbg_data_err(ret)
#endif
/*----------------------------------------------------------------------------*/
uint64_t
sdcard_get_card_size(void)
{
  return ((uint64_t) sdcard_card_block_count) * sdcard_block_size;
}
/*----------------------------------------------------------------------------*/
uint32_t
sdcard_get_block_num(void)
{
  return (sdcard_card_block_count / sdcard_block_size) * sdcard_get_block_size();
}
/*----------------------------------------------------------------------------*/
uint16_t
sdcard_get_block_size(void)
{
  return 512;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_is_SDSC(void)
{
  return sdcard_sdsc_card;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_set_CRC(uint8_t enable)
{
  uint8_t ret;
  uint32_t arg = enable ? 1L : 0L;

  mspi_chip_select(MICRO_SD_CS);

  if ((ret = sdcard_write_cmd(SDCARD_CMD59, &arg, NULL)) != 0x00) {
    PRINTD("\nsdcard_set_CRC(): ret = %u", ret);
    return SDCARD_CMD_ERROR;
  }

  sdcard_crc_enable = enable;
  mspi_chip_release(MICRO_SD_CS);

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_read_csd(uint8_t *buffer)
{
  uint8_t ret, i = 0;

  mspi_chip_select(MICRO_SD_CS);

  if ((ret = sdcard_write_cmd(SDCARD_CMD9, NULL, NULL)) != 0x00) {
    return SDCARD_CMD_ERROR;
  }

  /* wait for the 0xFE start byte */
  i = 0;
  while ((ret = mspi_transceive(MSPI_DUMMY_BYTE)) == SD_DATA_HIGH) {
    i++;
    if (i > N_CX_MAX) {
      PRINTD("\nsdcard_read_csd(): No data token");
      return SDCARD_DATA_TIMEOUT;
    }
  }

  /* Check for start block token */
  if (ret != START_BLOCK_TOKEN) {
    dbg_data_err(ret);
    return SDCARD_DATA_ERROR;
  }

  /* Read 16 bit CSD content */
  for (i = 0; i < 16; i++) {
    *buffer++ = mspi_transceive(MSPI_DUMMY_BYTE);
  }

  /* CRC-Byte: don't care */
  mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_transceive(MSPI_DUMMY_BYTE);

  /*release chip select and disable sdcard spi*/
  mspi_chip_release(MICRO_SD_CS);

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/**
 * Parse the given csd and extract the needed information out of it.
 *
 * \param *csd A Buffer in which the contents of the csd are saved (get them with sdcard_read_csd()).
 */
static void
get_csd_info(uint8_t *csd)
{
  uint8_t csd_version = 0;
  uint8_t READ_BL_LEN = 0;
  uint32_t C_SIZE = 0;
  uint16_t C_SIZE_MULT = 0;
  uint32_t mult = 0;
  uint8_t SECTOR_SIZE = 0;
  uint32_t capacity = 0;
  // TODO: Check CCC?

  /* First, determine CSD version and layout */
  csd_version = csd[0] >> 6;

  /* CSD Version 1.0 */
  if (csd_version == 0) {
    /* 80:83 (4 Bit) */
    READ_BL_LEN = csd[5] & 0x0F;
    /* 73:62 (12 Bit) [xxxxxxDD:DDDDDDDD:DDxxxxxx] */
    C_SIZE = (((uint16_t) (csd[6] & 0x03)) << 10) | (((uint16_t) csd[7]) << 2) | (csd[8] >> 6);
    /* 49:47 (3 Bit) */
    C_SIZE_MULT = ((csd[9] & 0x03) << 1) | (csd[10] >> 7);
    /* 45:39 (7 Bit) */
    SECTOR_SIZE = ((csd[11] >> 7) | ((csd[10] & 0x3F) << 1)) + 1;

    mult = (1 << (C_SIZE_MULT + 2));
    sdcard_card_block_count = (C_SIZE + 1) * mult;
    sdcard_block_size = 1 << READ_BL_LEN;
    capacity = sdcard_card_block_count * sdcard_block_size / 1024 / 1024;

  /* CSD Version 2.0 (SDHC/SDXC) */
  } else if (csd_version == 1) {
    /* 80:83, should be fixed to 512 Bytes */
    READ_BL_LEN = csd[5] & 0x0F;
    /* 69:48 (22 Bit) */
    C_SIZE = csd[9] + (((uint32_t) csd[8]) << 8) + (((uint32_t) csd[7] & 0x3f) << 16);
    /* 45:39 (7 Bit) */
    SECTOR_SIZE = ((csd[11] >> 7) | ((csd[10] & 0x3F) << 1)) + 1;

    sdcard_card_block_count = (C_SIZE + 1) * 1024;
    sdcard_block_size = 512;
    capacity = C_SIZE * 512 / 1024;
  } else {
    PRINTD("\nInvalid CSD version");
    return;
  }

  PRINTF("\nget_csd_info(): CSD Version = %u", csd_version);
  PRINTF("\nget_csd_info(): READ_BL_LEN = %u", READ_BL_LEN);
  PRINTF("\nget_csd_info(): C_SIZE = %lu", C_SIZE);
  PRINTF("\nget_csd_info(): Capacity = %lu MB", capacity);
  PRINTF("\nget_csd_info(): C_SIZE_MULT = %u", C_SIZE_MULT);
  PRINTF("\nget_csd_info(): mult = %lu", mult);
  PRINTF("\nget_csd_info(): sdcard_card_block_count  = %lu", sdcard_card_block_count);
  PRINTF("\nget_csd_info(): sdcard_block_size  = %u", sdcard_block_size);
  PRINTF("\nget_csd_info(): SECTOR_SIZE = %u", SECTOR_SIZE);
}
/*----------------------------------------------------------------------------*/
/**
 * \brief This function calculates the CRC7 for SD Card commands.
 *
 * \param *cmd The array containing the command with every byte except
 *	the CRC byte correctly set. The CRC part will be written into the
 *	cmd array (at the correct position).
 */
void
sdcard_cmd_crc(uint8_t *cmd)
{
  uint32_t stream = (((uint32_t) cmd[0]) << 24) +
          (((uint32_t) cmd[1]) << 16) +
          (((uint32_t) cmd[2]) << 8) +
          ((uint32_t) cmd[3]);
  uint8_t i = 0;
  uint32_t gen = ((uint32_t) 0x89) << 24;

  for (i = 0; i < 40; i++) {
    if (stream & (((uint32_t) 0x80) << 24)) {
      stream ^= gen;
    }

    stream = stream << 1;
    if (i == 7) {
      stream += cmd[4];
    }
  }

  cmd[5] = (stream >> 24) + 1;
}
/*----------------------------------------------------------------------------*/
uint16_t
sdcard_data_crc(uint8_t *data)
{
  uint32_t stream = (((uint32_t) data[0]) << 24) +
          (((uint32_t) data[1]) << 16) +
          (((uint32_t) data[2]) << 8) +
          ((uint32_t) data[3]);
  uint16_t i = 0;
  uint32_t gen = ((uint32_t) 0x11021) << 15;

  /* 4096 Bits = 512 Bytes a 8 Bits */
  for (i = 0; i < 4096; i++) {
    if (stream & (((uint32_t) 0x80) << 24)) {
      stream ^= gen;
    }

    stream = stream << 1;
    if (((i + 1) % 8) == 0 && i < 4064) {
      stream += data[((i + 1) / 8) + 3];
    }
  }

  return (stream >> 16);
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_init(void)
{
  uint16_t i;
  uint8_t ret = 0;
  sdcard_sdsc_card = 1;

  uint32_t cmd_arg = 0;
  /*Response Array for the R3 and R7 responses*/
  uint8_t resp[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  /* Memory for the contents of the csd */
  uint8_t csd[16];

  /* READY TO INITIALIZE micro SD / SD card */
  mspi_chip_release(MICRO_SD_CS);

  /*init mspi in mode0, at chip select pin 2 and max baud rate */
  mspi_init(MICRO_SD_CS, MSPI_MODE_0, MSPI_BAUD_2MBPS);

  /* set SPI mode by chip select (only necessary when mspi manager is active) */
  mspi_chip_select(MICRO_SD_CS);

  /* send initialization sequence (>74 clock cycles) */
  for (i = 0; i < 16; i++) {
    mspi_transceive(MSPI_DUMMY_BYTE);
  }

  /* CMD0 with CS asserted enables SPI mode.
   * Note: Card may hold line low until fully initialized */
  i = 0;
  while ((ret = sdcard_write_cmd(SDCARD_CMD0, NULL, NULL)) != SD_R1_IN_IDLE_STATE) {
    i++;
    if (i > 200) {
      mspi_chip_release(MICRO_SD_CS);
      PRINTD("\nsdcard_init(): cmd0 timeout -> %d", ret);
      return SDCARD_CMD_TIMEOUT;
    }
  }

  /* CMD8: Test if the card supports CSD Version 2.
   * Shall be issued before ACMD41. */
  i = 0;
  /* CMD8 payload [ 01 = voltage supplied (2.7-3.6V), 0xAA = any check-pattern ] */
  cmd_arg = 0x000001AAUL;
  resp[0] = SDCARD_RESP7;
  while ((ret = sdcard_write_cmd(SDCARD_CMD8, &cmd_arg, resp)) != SD_R1_IN_IDLE_STATE) {
    if ((ret & SD_R1_ILLEGAL_CMD) && ret != 0xFF) {
      PRINTF("\nsdcard_init(): cmd8 not supported -> Ver1.X or no SD Memory Card");
      break;
    }
    i++;
    if (i > 200) {
      mspi_chip_release(MICRO_SD_CS);
      PRINTD("\nsdcard_init(): cmd8 timeout -> %d", ret);
      return SDCARD_CMD_TIMEOUT;
    }
  }
  /* Check response payload in case of accepted response */
  if (ret == SD_R1_IN_IDLE_STATE) {
    // check voltage 
    if (resp[3] != ((uint8_t) (cmd_arg >> 8) & 0x0F)) {
      printf("\nsdcard_init(): Voltage not accepted");
      return SDCARD_REJECTED;
    }
    // check echo-back pattern
    if (resp[4] != ((uint8_t) (cmd_arg & 0xFF))) {
      printf("\nsdcard_init(): Echo-back pattern wrong, sent %02x, rec: %02x",
          (uint8_t) (cmd_arg & 0xFF), resp[4]);
      return SDCARD_CMD_ERROR;
    }
  }

  /* Illegal command -> Ver 1.X SD Memory Card or Not SD Memory Card*/
  if (ret & SD_R1_ILLEGAL_CMD) {

    /* CMD1: init CSD Version 1 and MMC cards*/
    i = 0;
    while ((ret = sdcard_write_cmd(SDCARD_CMD1, NULL, NULL)) != 0) {
      i++;
      if (i > 5500) {
        PRINTD("\nsdcard_init(): cmd1 timeout reached, last return value was %d", ret);
        mspi_chip_release(MICRO_SD_CS);
        return SDCARD_CMD_TIMEOUT;
      }
    }

    /* CMD16: Sets the block length, needed for CSD Version 1 cards */
    i = 0;
    /* CMD15 payload (block length = 512) */
    cmd_arg = 0x00000200UL;
    while ((ret = sdcard_write_cmd(SDCARD_CMD16, &cmd_arg, NULL)) != 0x00) {
      i++;
      if (i > 500) {
        PRINTF("\nsdcard_init(): cmd16 timeout reached, last return value was %d", ret);
        mspi_chip_release(MICRO_SD_CS);
        return SDCARD_CMD_TIMEOUT;
      }
    }

  /* Ver2.00 or later SD Memory Card */
  } else {

    i = 0;
    /* ACMD41 payload [HCS bit set] */
    cmd_arg = 0x40000000UL;
    while (sdcard_write_cmd(SDCARD_ACMD41, &cmd_arg, NULL) != 0) {
      i++;
      _delay_ms(2);

      if (i > 200) {
        PRINTD("\nsdcard_init(): acmd41 timeout reached, last return value was %u", ret);
        mspi_chip_release(MICRO_SD_CS);
        return SDCARD_CMD_TIMEOUT;
      }
    }

    /* CMD58: Gets the OCR-Register to check if card is SDSC or not */
    i = 0;
    resp[0] = SDCARD_RESP3;
    while ((ret = sdcard_write_cmd(SDCARD_CMD58, NULL, resp)) != 0x0) {
      i++;
      if (i > 900) {
        PRINTD("\nsdcard_init(): cmd58 timeout reached, last return value was %d", ret);
        mspi_chip_release(MICRO_SD_CS);
        return SDCARD_CMD_TIMEOUT;
      }
    }

    /* Check if power up finished */
    if (resp[1] & OCR_H_POWER_UP) {
      PRINTF("\nsdcard_init(): acmd41: Card power up okay!");
      /* Check if SDSC or SDHC/SDXC */
      if (resp[1] & OCR_H_CCS) {
        sdcard_sdsc_card = 0;
        PRINTF("\nsdcard_init(): acmd41: Card is SDHC/SDXC");
      } else {
        PRINTF("\nsdcard_init(): acmd41: Card is SDSC");
      }

    }
  }

  /* Read card-specific data (CSD) register */
  i = 0;
  while (sdcard_read_csd(csd) != SDCARD_SUCCESS) {
    i++;
    if (i > 100) {
      mspi_chip_release(MICRO_SD_CS);
      PRINTD("\nsdcard_init(): CSD read error");
      return SDCARD_CSD_ERROR;
    }
  }
  get_csd_info(csd);

  mspi_chip_release(MICRO_SD_CS);

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_erase_blocks(uint32_t startaddr, uint32_t endaddr)
{
  uint16_t ret;

  /* calculate the start address: block_addr = addr * 512
   * this is only needed if the card is a SDSC card and uses
   * byte addressing (Block size of 512 is set in sdcard_init()).
   * SDHC and SDXC card use block-addressing with a block size of
   * 512 Bytes.
   */
  if (sdcard_sdsc_card) {
    startaddr = startaddr << 9;
    endaddr = endaddr << 9;
  }

  mspi_chip_select(MICRO_SD_CS);

  /* send CMD32 with address information. */
  if ((ret = sdcard_write_cmd(SDCARD_CMD32, &startaddr, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    PRINTD("\nCMD32 failed");
    return ret;
  }

  /* send CMD33 with address information. */
  if ((ret = sdcard_write_cmd(SDCARD_CMD33, &endaddr, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    PRINTD("\nCMD33 failed");
    return ret;
  }

  /* send CMD38 with address information. */
  if ((ret = sdcard_write_cmd(SDCARD_CMD38, NULL, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    PRINTD("\nCMD38 failed");
    return ret;
  }

  /* release chip select and disable sdcard spi */
  mspi_chip_release(MICRO_SD_CS);

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_read_block(uint32_t addr, uint8_t *buffer)
{
  uint16_t i;
  uint8_t ret;

  /* calculate the start address: block_addr = addr * 512
   * this is only needed if the card is a SDSC card and uses
   * byte addressing (Block size of 512 is set in sdcard_init()).
   * SDHC and SDXC card use block-addressing with a block size of
   * 512 Bytes.
   */
  if (sdcard_sdsc_card) {
    addr = addr << 9;
  }

  mspi_chip_select(MICRO_SD_CS);

  if (sdcard_busy_wait() == SDCARD_BUSY_TIMEOUT) {
    return SDCARD_BUSY_TIMEOUT;
  }

  /* send CMD17 with address information. */ 
  if ((i = sdcard_write_cmd(SDCARD_CMD17, &addr, NULL)) != 0x00) {
    PRINTD("\nsdcard_read_block(): CMD17 failure! (%u)", i);
    return SDCARD_CMD_ERROR;
  }

  /* wait for the 0xFE start byte */
  i = 0;
  while ((ret = mspi_transceive(MSPI_DUMMY_BYTE)) == SD_DATA_HIGH) {
    if (i >= 200) {
      PRINTD("\nsdcard_read_block(): No Start Byte recieved, last was %d", ret);
      return SDCARD_DATA_TIMEOUT;
    }
  }
  /* exit on error response */
  if (ret != START_BLOCK_TOKEN) {
#if DEBUG
    dbg_data_err(ret);
#endif // debug
    return SDCARD_DATA_ERROR;
  }

  /* transfer block */
  for (i = 0; i < 512; i++) {
    buffer[i] = mspi_transceive(MSPI_DUMMY_BYTE);
  }

  /* Read CRC-Byte: don't care */
  mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* release chip select and disable sdcard spi */
  mspi_chip_release(MICRO_SD_CS);

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/* @TODO: currently not used in any way */
uint16_t
sdcard_get_status(void)
{
  uint8_t resp[5] = {SDCARD_RESP2, 0x00, 0x00, 0x00, 0x00};

  if (sdcard_busy_wait() == SDCARD_BUSY_TIMEOUT) {
    return SDCARD_BUSY_TIMEOUT;
  }

  if (sdcard_write_cmd(SDCARD_CMD13, NULL, resp) != 0x00) {
    printf("\nFailed to read status");
    return 0;
  }

  return ((uint16_t) resp[1] << 8) + ((uint16_t) resp[0]);
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_write_block(uint32_t addr, uint8_t *buffer)
{
  uint16_t i;

  /* calculate the start address: byte_addr = block_addr * 512.
   * this is only needed if the card is a SDSC card and uses
   * byte addressing (Block size of 512 is set in sdcard_init()).
   * SDHC and SDXC card use block-addressing with a fixed block size
   * of 512 Bytes.
   */
  if (sdcard_sdsc_card) {
    addr = addr << 9;
  }

  mspi_chip_select(MICRO_SD_CS);

  if (sdcard_busy_wait() == SDCARD_BUSY_TIMEOUT) {
    return SDCARD_BUSY_TIMEOUT;
  }

  /* send CMD24 with address information. */
  if (sdcard_write_cmd(SDCARD_CMD24, &addr, NULL) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    return SDCARD_CMD_ERROR;
  }

  /* send start byte 0xFE to the sdcard card to symbolize the beginning
   * of one data block (512byte) */
  mspi_transceive(START_BLOCK_TOKEN);

  /* send 1 block (512byte) to the sdcard card */
  for (i = 0; i < 512; i++) {
    mspi_transceive(buffer[i]);
  }

  /* write dummy 16 bit CRC checksum */
  /** @todo: handle crc enabled? */
  mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* failure check: Data Response XXX0RRR1 */
  i = mspi_transceive(MSPI_DUMMY_BYTE) & 0x1F;
  /* after the response the card requires additional 8 clock cycles. */
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* release chip select and disable sdcard spi
   * note: busy waiting is not required here
   *       becaus it is already done before writing/reading
   */
  mspi_chip_release(MICRO_SD_CS);

  /* Data Response XXX00101 = OK */
  if (i != DATA_RESP_ACCEPTED) {
#if DEBUG
    if (i == DATA_RESP_CRC_REJECT) {
      PRINTD("\nWrite response: CRC error");
    } else if (i == DATA_RESP_WRITE_ERROR) {
      PRINTD("\nData Resp: Write error");
    } else {
      PRINTD("\nData Resp %02x unknown", i);
    }
#endif
    return SDCARD_DATA_ERROR;
  }

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_write_multi_block_start(uint32_t addr, uint32_t num_blocks)
{

  /* calculate the start address: byte_addr = block_addr * 512.
   * this is only needed if the card is a SDSC card and uses
   * byte addressing (Block size of 512 is set in sdcard_init()).
   * SDHC and SDXC card use block-addressing with a fixed block size
   * of 512 Bytes.
   */
  if (sdcard_sdsc_card) {
    addr = addr << 9;
  }

  mspi_chip_select(MICRO_SD_CS);

  if (sdcard_busy_wait() == SDCARD_BUSY_TIMEOUT) {
    return SDCARD_BUSY_TIMEOUT;
  }

  if (num_blocks != 0) {
    /* Announce number of blocks to write to card for pre-erase
     * Note. */
    PRINTF("\nPre-erasing %ld blocks", num_blocks);
    sdcard_write_cmd(SDCARD_ACMD23, &num_blocks, NULL);
  }

  /* send CMD25 with address information. */
  if (sdcard_write_cmd(SDCARD_CMD25, &addr, NULL) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    return SDCARD_CMD_ERROR;
  }

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_write_multi_block_next(uint8_t *buffer)
{
  uint16_t i;

  mspi_chip_select(MICRO_SD_CS);

  if (sdcard_busy_wait() == SDCARD_BUSY_TIMEOUT) {
    return SDCARD_BUSY_TIMEOUT;
  }

  /* send start byte 0xFC to the sdcard card to symbolize the beginning
   * of one data block (512byte) */
  mspi_transceive(MULTI_START_BLOCK_TOKEN);

  /* send 1 block (512byte) to the sdcard card */
  for (i = 0; i < 512; i++) {
    mspi_transceive(buffer[i]);
  }

  /* write dummy 16 bit CRC checksum */
  /** @todo: handle crc enabled? */
  mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* failure check: Data Response XXX0RRR1 */
  i = mspi_transceive(MSPI_DUMMY_BYTE) & 0x1F;
  /* after the response the card requires additional 8 clock cycles. */
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* release chip select and disable sdcard spi */
  mspi_chip_release(MICRO_SD_CS);

  /* Data Response XXX00101 = OK */
  if (i != DATA_RESP_ACCEPTED) {
#if DEBUG
    if (i == DATA_RESP_CRC_REJECT) {
      PRINTD("\nWrite response: CRC error");
    } else if (i == DATA_RESP_WRITE_ERROR) {
      PRINTD("\nData Resp: Write error");
    } else {
      PRINTD("\nData Resp %02x unknown", i);
    }
#endif
    return SDCARD_DATA_ERROR;
  }

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_write_multi_block_stop()
{
  mspi_chip_select(MICRO_SD_CS);

  if (sdcard_busy_wait() == SDCARD_BUSY_TIMEOUT) {
    return SDCARD_BUSY_TIMEOUT;
  }

  mspi_transceive(STOP_TRAN_TOKEN);
  /* after the response the card requires additional 8 clock cycles. */
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* release chip select and disable sdcard spi */
  mspi_chip_release(MICRO_SD_CS);

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
static uint8_t
sdcard_busy_wait()
{
  /* Wait until the card is not busy anymore! 
   * Note: It is always checked twice because
   *       busy signal becomes high only when clocked before */
  uint16_t i = 0;
  while ((mspi_transceive(MSPI_DUMMY_BYTE) != SD_DATA_HIGH)
      && (mspi_transceive(MSPI_DUMMY_BYTE) != SD_DATA_HIGH)) {
    _delay_ms(1);
    i++;
    if (i >= BUSY_WAIT_MS) {
      PRINTD("\nsdcard_busy_wait(): Busy wait timeout");
      return SDCARD_BUSY_TIMEOUT;
    }
  }

  return SDCARD_SUCCESS;
}
/*----------------------------------------------------------------------------*/
// returns 0xFF as error code, else R1 part if available
uint8_t
sdcard_write_cmd(uint8_t cmd, uint32_t *arg, uint8_t *resp)
{
  uint16_t i;
  uint8_t data;
  uint8_t idx = 0;
  uint8_t resp_type = 0x01;
  static uint8_t cmd_seq[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0xFF};

  uint8_t ret;
  /* pre-send CMD55 for ACMD */
  if (cmd & IS_ACMD) {
    /* Abort if response has error bit set */
    if (((ret = sdcard_write_cmd(SDCARD_CMD55, NULL, NULL)) & 0xFE) != 0x00) {
      return 0xFF;
    }
  }

  /* Give card chance to raise busy signal while we perform some calculations */
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* (remove potential ACMD-marker, add host bit to complete command */
  cmd_seq[0] = (cmd & ~IS_ACMD) | 0x40;

  if (resp != NULL) {
    resp_type = resp[0];
  }
  /* If no argument given, send 0's */
  if (arg == NULL) {
    cmd_seq[1] = 0x00;
    cmd_seq[2] = 0x00;
    cmd_seq[3] = 0x00;
    cmd_seq[4] = 0x00;
  /* else create cmd bytes according to the (address) argument */
  } else {
    cmd_seq[1] = ((*arg & 0xFF000000) >> 24);
    cmd_seq[2] = ((*arg & 0x00FF0000) >> 16);
    cmd_seq[3] = ((*arg & 0x0000FF00) >> 8);
    cmd_seq[4] = ((*arg & 0x000000FF));
  }

  /* CMD0 is the only command that always requires a valid CRC */
  if (cmd == SDCARD_CMD0) {
    cmd_seq[5] = 0x95;
  /* Calc CRC if enabled */
  } else if ((sdcard_crc_enable) || (cmd == SDCARD_CMD8)) {
    sdcard_cmd_crc(cmd_seq);
  /* Else set fixed value (last bit must be '1') */
  } else {
    cmd_seq[5] = 0xFF;
  }

  /* Send the 48 command bits */
  for (i = 0; i < 6; i++) {
    mspi_transceive(*(cmd_seq + i));
  }

  /* wait for the answer of the sd card */
  i = 0;
  do {
    i++;
    /*0x01 for acknowledge*/
    data = mspi_transceive(MSPI_DUMMY_BYTE);
    watchdog_periodic();

    /* If we still wait for the Rx response,
     * we do not accept 0xFF */
    if ((data == 0xFF) && (idx == 0)) {
      continue;
    }

    /* For R1 with NULL arg (no return buffer) */
    if (resp == NULL) {
      dbg_resp_r1(cmd, data);
      break;
    }

    switch (resp_type) {

      case SDCARD_RESP1:
        resp[0] = data;
        dbg_resp_r1(cmd, data);
        i = 501;
        break;

      case SDCARD_RESP2:
        resp[idx] = data;
        idx++;
        if ((idx >= 2) || (resp[0] & 0xFE) != 0) {
          dbg_resp_r2(cmd, resp);
          data = resp[0];
          i = 501;
        }
        break;

      case SDCARD_RESP3:
      case SDCARD_RESP7:
        resp[idx] = data;
        idx++;
        if ((idx >= 5) || (resp[0] & 0xFE) != 0) {
          if (resp_type == SDCARD_RESP3) {
            dbg_resp_r3(cmd, resp);
          } else {
            dbg_resp_r7(cmd, resp);
          }
          data = resp[0];
          i = 501;
        }
        break;
    }
  } while (i < 100); // should not be more than N_CR_MAX + 5
 
  /* Loop left at 500 indicates timeout error */
  if (i == 500) {
    PRINTD("\nResponse timeout");
    data = 0xFF;
  }

  return data;
}
/*----------------------------------------------------------------------------*/

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

#define DEBUG 1

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


#define SD_SET_BLOCKLEN cmd16

#define SD_R1_IN_IDLE_STATE 0x01
#define SD_R1_ERASE_RESET   0x02
#define SD_R1_ILLEGAL_CMD   0x04
#define SD_R1_COM_CRC_ERR   0x08
#define SD_R1_ERASE_SEQ_ERR 0x10
#define SD_R1_ADDR_ERR      0x20
#define SD_R1_PARAM_ERR     0x40

/* Single-Block Read, Single-Block Write and Multiple-Block Read */
#define START_BLOCK_TOKEN         0xFE
/* Multiple Block Write Operation */
#define MULTI_START_BLOCK_TOKEN   0xFC
#define STOP_TRAN_TOKEN           0xFD

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
/** ACMD41 -- */
#define SDCARD_ACMD41 41

#define RESPONSE_R1   0x01
#define RESPONSE_R2   0x02
#define RESPONSE_R3   0x03
#define RESPONSE_R7   0x07

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

static void setSDCInfo(uint8_t *csd);

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
/**
 * Returns the size of one block in bytes.
 *
 * Currently it's fixed at 512 Bytes.
 * \return Number of Bytes in one Block.
 */
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

  if ((ret = sdcard_write_cmd(SDCARD_CMD59, &arg, NULL)) != 0x00) {
    PRINTD("\nsdcard_set_CRC(): ret = %u", ret);
    return 1;
  }

  sdcard_crc_enable = enable;
  mspi_chip_release(MICRO_SD_CS);

  return 0;
}
/*----------------------------------------------------------------------------*/

#define DATA_ERROR            0x01
#define DATA_CC_ERROR         0x02
#define DATA_CARD_ECC_FAILED  0x04
#define DATA_OUT_OF_RANGE     0x08

uint8_t
sdcard_read_csd(uint8_t *buffer)
{
  uint8_t i = 0;

  /** TODO: mutliple attempts? */
  if ((i = sdcard_write_cmd(SDCARD_CMD9, NULL, NULL)) != 0x00) {
    // eval data error token
    if (i & DATA_ERROR) {
      printf("\ndata error");
    }
    if (i & DATA_CC_ERROR) {
      printf("\ndata CC error");
    }
    if (i & DATA_CARD_ECC_FAILED) {
      printf("\n card EEC failed");
    }
    if (i & DATA_OUT_OF_RANGE) {
      printf("\n data out of range");
    }
    PRINTD("\nsdcard_read_csd(): CMD9 failure! (%u)", i);
    return 1;
  }

  /* wait for the 0xFE start byte */
  i = 0;
  while (mspi_transceive(MSPI_DUMMY_BYTE) != START_BLOCK_TOKEN) {
    if (i > 200) {
      PRINTD("\nsdcard_read_csd(): No data token");
      return 2;
    }
  }

  for (i = 0; i < 16; i++) {
    *buffer++ = mspi_transceive(MSPI_DUMMY_BYTE);
  }

  /* CRC-Byte: don't care */
  mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_transceive(MSPI_DUMMY_BYTE);

  /*release chip select and disable sdcard spi*/
  mspi_chip_release(MICRO_SD_CS);

  return 0;
}
/*----------------------------------------------------------------------------*/
/**
 * Parse the given csd and extract the needed information out of it.
 *
 * \param *csd A Buffer in which the contents of the csd are saved (get them with sdcard_read_csd()).
 */
static void
setSDCInfo(uint8_t *csd)
{
  uint8_t csd_version = ((csd[0] & (12 << 4)) >> 6);
  uint8_t READ_BL_LEN = 0;
  uint32_t C_SIZE = 0;
  uint16_t C_SIZE_MULT = 0;
  uint32_t mult = 0;
  uint8_t SECTOR_SIZE = 0;

  if (csd_version == 0) {
    /*Bits 80 till 83 are the READ_BL_LEN*/
    READ_BL_LEN = csd[5] & 0x0F;

    /*Bits 62 - 73 are C_SIZE*/
    C_SIZE = ((csd[8] & 07000) >> 6) + (((uint32_t) csd[7]) << 2) + (((uint32_t) csd[6] & 07) << 10);

    /*Bits 47 - 49 are C_SIZE_MULT*/
    C_SIZE_MULT = ((csd[9] & 0x80) >> 7) + ((csd[8] & 0x03) << 1);

    mult = (2 << (C_SIZE_MULT + 2));
    sdcard_card_block_count = (C_SIZE + 1) * mult;
    sdcard_block_size = 1 << READ_BL_LEN;
  } else if (csd_version == 1) {
    /*Bits 80 till 83 are the READ_BL_LEN*/
    READ_BL_LEN = csd[5] & 0x0F;
    C_SIZE = csd[9] + (((uint32_t) csd[8]) << 8) + (((uint32_t) csd[7] & 0x3f) << 16);
    sdcard_card_block_count = (C_SIZE + 1) * 1024;
    sdcard_block_size = 512;
    SECTOR_SIZE = ((csd[4] >> 7) | ((csd[5] & 0x3F) << 1)) + 1;
  }

  PRINTF("\nsetSDCInfo(): CSD Version = %u", ((csd[0] & (12 << 4)) >> 6));
  PRINTF("\nsetSDCInfo(): READ_BL_LEN = %u", READ_BL_LEN);
  PRINTF("\nsetSDCInfo(): C_SIZE = %lu", C_SIZE);
  PRINTF("\nsetSDCInfo(): C_SIZE_MULT = %u", C_SIZE_MULT);
  PRINTF("\nsetSDCInfo(): mult = %lu", mult);
  PRINTF("\nsetSDCInfo(): sdcard_card_block_count  = %lu", sdcard_card_block_count);
  PRINTF("\nsetSDCInfo(): sdcard_block_size  = %u", sdcard_block_size);
  PRINTF("\nsetSDCInfo(): SECTOR_SIZE = %u", SECTOR_SIZE);
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

  /*READY TO INITIALIZE micro SD / SD card */
  mspi_chip_release(MICRO_SD_CS);

  /*init mspi in mode0, at chip select pin 2 and max baud rate */
  mspi_init(MICRO_SD_CS, MSPI_MODE_0, MSPI_BAUD_2MBPS);

  /* set SPI mode by chip select (only necessary when mspi manager is active) */
  mspi_chip_select(MICRO_SD_CS);

  /* send initialization sequence (>74 clock cycles) */
  for (i = 0; i < 16; i++) {
    mspi_transceive(MSPI_DUMMY_BYTE);
  }

  /* CMD8 payload */
  uint32_t cmd8_arg = 0x000001AAUL;
  /* CMD15 payload */
  uint32_t cmd16_arg = 0x00000200UL;
  /* ACMD41 payload */
  uint32_t acmd41_arg = 0x40000000UL;
  /*Response Array for the R3 and R7 responses*/
  uint8_t resp[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

  /* Memory for the contents of the csd */
  uint8_t csd[16];

  /* CMD0 with CS asserted enables SPI mode.
   * Note: Card may hold line low until fully initialized */
  i = 0;
  while ((ret = sdcard_write_cmd(SDCARD_CMD0, NULL, NULL)) != 0x01) {
    _delay_ms(5);
    i++;
    if (i > 200) {
      mspi_chip_release(MICRO_SD_CS);
      PRINTD("\nsdcard_init(): cmd0 timeout -> %d", ret);
      return 1;
    }
  }

  /*CMD8: Test if the card supports CSD Version 2*/
  /** @todo: is CRC required here? */
  //sdcard_cmd_crc(cmd8);
  i = 0;
  resp[0] = RESPONSE_R7;
  while ((ret = sdcard_write_cmd(SDCARD_CMD8, &cmd8_arg, resp)) != 0x01) {
    if ((ret & 0x04) && ret != 0xFF) {
      PRINTD("\nsdcard_init(): cmd8 not supported -> legacy card");
      break;
    }
    i++;
    if (i > 200) {
      mspi_chip_release(MICRO_SD_CS);
      PRINTD("\nsdcard_init(): cmd8 timeout -> %d", ret);
      return 4;
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
        return 2;
      }
    }

    /* CMD16: Sets the block length, needed for CSD Version 1 cards*/
    /* Set Block Length to 512 Bytes */
    i = 0;
    /** @todo: Is this command really needed? It does not seem to set anything. */
    while ((ret = sdcard_write_cmd(SDCARD_CMD16, &cmd16_arg, NULL)) != 0x00) {
      i++;
      if (i > 500) {
        PRINTF("\nsdcard_init(): cmd16 timeout reached, last return value was %d", ret);
        mspi_chip_release(MICRO_SD_CS);
        return 5;
      }
    }

  /* Ver2.00 or later SD Memory Card */
  } else {

    uint16_t j = 0;
    do {
      _delay_ms(5);
      j++;
      /* CMD55: First part of ACMD41, Initiates a App. Spec. Cmd */
      i = 0;
      while ((ret = sdcard_write_cmd(SDCARD_CMD55, NULL, NULL)) != 0x01) {
        i++;
        if (i > 500) {
          PRINTD("\nsdcard_init(): acmd41 timeout reached, last return value was %u", ret);
          mspi_chip_release(MICRO_SD_CS);
          return 6;
        }
      }

      if (j > 12000) {
        PRINTD("\nwooot?");
        return 8;
      }
    /* CMD41: Second part of ACMD41 */
    } while ((sdcard_write_cmd(SDCARD_ACMD41, &acmd41_arg, NULL) != 0));

    /* CMD58: Gets the OCR-Register to check if card is SDSC or not */
    i = 0;
    resp[0] = RESPONSE_R3;
    while ((ret = sdcard_write_cmd(SDCARD_CMD58, NULL, resp)) != 0x0) {
      i++;
      if (i > 900) {
        PRINTD("\nsdcard_init(): cmd58 timeout reached, last return value was %d", ret);
        mspi_chip_release(MICRO_SD_CS);
        return 7;
      }
    }

    if (resp[1] & 0x80) {
      if (resp[1] & 0x40) {
        sdcard_sdsc_card = 0;
      }

      PRINTF("\nsdcard_init(): acmd41: Card power up okay!");
      if (resp[1] & 0x40) {
        PRINTF("\nsdcard_init(): acmd41: Card is SDHC/SDXC");
      } else {
        PRINTF("\nsdcard_init(): acmd41: Card is SDSC");
      }

    }

    /* Set Block Length to 512 Bytes */
    //i = 0;
    ///** @todo: Is this command really needed? It does not seem to set anything. */
    //while ((ret = sdcard_write_cmd(SDCARD_CMD16, &cmd16_arg, NULL)) != 0x00) {
    //  i++;
    //  if (i > 500) {
    //    PRINTD("\nsdcard_init(): cmd16 timeout reached, last return value was %d", ret);
    //    mspi_chip_release(MICRO_SD_CS);
    //    return 5;
    //  }
    //}
  }

  /* Read card-specific data (CSD) register */
  i = 0;
  while (sdcard_read_csd(csd) != 0) {
    i++;
    if (i > 900) {
      mspi_chip_release(MICRO_SD_CS);
      PRINTD("\nsdcard_init(): CSD read error");
      return 3;
    }
  }
  setSDCInfo(csd);

  mspi_chip_release(MICRO_SD_CS);

  return 0;
}
/*----------------------------------------------------------------------------*/
uint8_t
sdcard_erase_blocks(uint32_t startaddr, uint32_t endaddr)
{
  uint16_t i;

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

  /* send CMD32 with address information.
   * Chip select is done by the sdcard_write_cmd method */
  if ((i = sdcard_write_cmd(SDCARD_CMD32, &startaddr, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    PRINTD("\nCMD32 failed");
    return 1;
  }

  /* send CMD33 with address information.
   * Chip select is done by the sdcard_write_cmd method */
  if ((i = sdcard_write_cmd(SDCARD_CMD33, &endaddr, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    PRINTD("\nCMD33 failed");
    return 1;
  }

  /* send CMD38 with address information.
   * Chip select is done by the sdcard_write_cmd method */
  if ((i = sdcard_write_cmd(SDCARD_CMD38, NULL, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    PRINTD("\nCMD38 failed");
    return 1;
  }

  /*release chip select and disable sdcard spi*/
  mspi_chip_release(MICRO_SD_CS);

  return 0;
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

  /* send CMD17 with address information. Chip select is done by
   * the sdcard_write_cmd method and */
  if ((i = sdcard_write_cmd(SDCARD_CMD17, &addr, NULL)) != 0x00) {
    PRINTD("\nsdcard_read_block(): CMD17 failure! (%u)", i);
    return 1;
  }

  /* wait for the 0xFE start byte */
  i = 0;
  while ((ret = mspi_transceive(MSPI_DUMMY_BYTE)) != START_BLOCK_TOKEN) {
    if (i >= 200) {
      PORTA ^= (1 << PA1);
      PRINTD("\nsdcard_read_block(): No Start Byte recieved, last was %d", ret);
      return 2;
    }
  }

  for (i = 0; i < 512; i++) {
    buffer[i] = mspi_transceive(MSPI_DUMMY_BYTE);
  }

  /* Read CRC-Byte: don't care */
  mspi_transceive(MSPI_DUMMY_BYTE);
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* release chip select and disable sdcard spi */
  mspi_chip_release(MICRO_SD_CS);

  return 0;
}
/*----------------------------------------------------------------------------*/
/* @TODO: currently not used in any way */
uint16_t
sdcard_get_status(void)
{
  uint8_t resp[5] = {RESPONSE_R2, 0x00, 0x00, 0x00, 0x00};

  if (sdcard_write_cmd(SDCARD_CMD13, NULL, resp) != 0x00) {
    return 0;
  }

  return ((uint16_t) resp[1] << 8) + ((uint16_t) resp[0]);
}
/*----------------------------------------------------------------------------*/
#define SDCARD_WRITE_COMMAND_ERROR  1
#define SDCARD_WRITE_DATA_ERROR     2
#define DATA_RESP_ACCEPTED    0x05
#define DATA_RESP_CRC_REJECT  0x0B
#define DATA_RESP_WRITE_ERROR 0x0D
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

  /* send CMD24 with address information.
   * Chip select is done by the sdcard_write_cmd method */
  if ((i = sdcard_write_cmd(SDCARD_CMD24, &addr, NULL)) != 0x00) {
    mspi_chip_release(MICRO_SD_CS);
    return 1;
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
  if (i != 0x05) {
#if DEBUG
    if (i == DATA_RESP_ACCEPTED) {
      PRINTD("\nWrite response: CRC error");
    } else if (i == DATA_RESP_CRC_REJECT) {
      PRINTD("\nData Resp: Write error");
    } else if (i == DATA_RESP_WRITE_ERROR) {
      PRINTD("\nData Resp: Error token");
    } else {
      PRINTD("\nData Resp %d unknown", i);
    }
#endif
    return 2;
  }

  return 0;
}
/*----------------------------------------------------------------------------*/


#define R1_RSP_IDLE_STATE       0x01
#define R1_RSP_ERASE_RESET      0x02
#define R1_RSP_ILLEGAL_CMD      0x04
#define R1_RSP_COM_CRC_ERROR    0x08
#define R1_RSP_ERASE_SEQ_ERROR  0x10
#define R1_RSP_ADDRESS_ERROR    0x20
#define R1_RSP_PARAMETER_ERROR  0x40

#if DEBUG
void
print_r1_resp(uint8_t cmd, uint8_t rsp) {
  /* everything might be ok */
  if (rsp == 0x00) {
    PRINTF("\nCMD%d: ", cmd);
    PRINTF("\n\tOk");
    return;
  /* idle is not really critical */
  } else if (rsp == R1_RSP_IDLE_STATE) {
    PRINTF("\nCMD%d: ", cmd);
    PRINTF("\n\tIdle");
    return;
  /* let us see what kind of error(s) we have */
  } else {
    PRINTD("\nCMD%d: ", cmd);
    if (rsp & R1_RSP_IDLE_STATE) {
      PRINTD("\n\tIdle");
    }
    if (rsp & R1_RSP_ERASE_RESET) {
      PRINTD("\n\tErase Reset");
    }
    if (rsp & R1_RSP_ILLEGAL_CMD) {
      PRINTD("\n\tIllegal Cmd");
    }
    if (rsp & R1_RSP_COM_CRC_ERROR) {
      PRINTD("\n\tCom CRC Err");
    }
    if (rsp & R1_RSP_ERASE_SEQ_ERROR) {
      PRINTD("\n\tErase Seq Err");
    }
    if (rsp & R1_RSP_ADDRESS_ERROR) {
      PRINTD("\n\tAddr Err");
    }
    if (rsp & R1_RSP_PARAMETER_ERROR) {
      PRINTD("\n\tParam Err");
    }
  }
}
/*----------------------------------------------------------------------------*/
void
output_response_r1(uint8_t cmd, uint8_t rsp) {
  print_r1_resp(cmd, rsp);
}
/*----------------------------------------------------------------------------*/
void
output_response_r2(uint8_t cmd, uint8_t* rsp) {
  print_r1_resp(cmd, rsp[0]);
  // todo: byte 2
}
/*----------------------------------------------------------------------------*/
void
output_response_r3(uint8_t cmd, uint8_t* rsp) {
  print_r1_resp(cmd, rsp[0]);
  PRINTF("\nOCR: 0x%04x", rsp[1]);
}
#else
#define output_response_r1(a, b)
#define output_response_r2(a, b)
#define output_response_r3(a, b)
#endif


/*----------------------------------------------------------------------------*/
uint8_t
sdcard_write_cmd(uint8_t cmd, uint32_t *arg, uint8_t *resp)
{
  uint16_t i;
  uint8_t data;
  uint8_t idx = 0;
  uint8_t resp_type = 0x01;
  static uint8_t cmd_seq[6] = {0x40, 0x00, 0x00, 0x00, 0x00, 0xFF};

  /* Give card chance to raise busy signal while we perform some calculations */
  mspi_transceive(MSPI_DUMMY_BYTE);

  /* Add host bit to complete command */
  cmd_seq[0] = cmd | 0x40;

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

  /* Wait until the card is not busy anymore! 
   * Note: It is always checked twice because
   *       busy signal becomes high only when clocked before */
  i = 0;
  while ((mspi_transceive(MSPI_DUMMY_BYTE) != 0xff)
      && (mspi_transceive(MSPI_DUMMY_BYTE) != 0xff)
      && (i < 200)) {
    _delay_ms(1);
    i++;
  }

  /* Send the 48 command bits */
#if DEBUG
  PRINTF("\nSend CMD%d", cmd);
  if (cmd == 55) {
    PRINTF(" (->ACMD)");
  }
#endif
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
      output_response_r1(cmd, data);
      break;
    }

    switch (resp_type) {

      case RESPONSE_R1:
        resp[0] = data;
        output_response_r1(cmd, data);
        i = 500;
        break;

      case RESPONSE_R2:
        resp[idx] = data;
        idx++;
        if ((idx >= 2) || (resp[0] & 0xFE) != 0) {
          output_response_r2(cmd, resp);
          i = 500;
        }
        break;

      case RESPONSE_R3:
      case RESPONSE_R7:
        resp[idx] = data;
        idx++;
        if ((idx >= 5) || (resp[0] & 0xFE) != 0) {
          output_response_r3(cmd, resp); // TODO: R7
          i = 500;
        }
        break;
    }
  } while (i < 500);

  return data;
}
/*----------------------------------------------------------------------------*/

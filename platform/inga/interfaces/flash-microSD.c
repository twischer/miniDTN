/*
 * Copyright (c) 2010, Institute of Operating Systems and Computer Networks (TU Brunswick).
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
 * Author: Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *         Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup Device Interfaces
 * @{
 *
 * \addtogroup microSD_interface
 * @{
 */

/**
 * \file
 *      MicroSD Card interface implementation
 * \author
 * 		Original source: Ulrich Radig
 * 		<< modified by >>
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 *		Christoph Peltz <peltz@ibr.cs.tu-bs.de>
 *
 */

#include "flash-microSD.h"
#include "dev/watchdog.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/**
 * \brief The number of bytes in one block on the SD-Card.
 */
uint16_t microSD_block_size = 512;

/**
 * \brief Number of blocks on the SD-Card.
 */
uint32_t microSD_card_block_count = 0;

/**
 * \brief Indicates if the inserted card is a SDSC card (!=0) or not (==0).
 */
uint8_t microSD_sdsc_card = 1;

/**
 * \brief Indicates if the cards CRC mode is on (1) or off (0). Default is off.
 */
uint8_t microSD_crc_enable = 0;

uint64_t microSD_get_card_size(void) {
	return ((uint64_t) microSD_card_block_count) * microSD_block_size;
}

uint32_t microSD_get_block_num(void) {
	return (microSD_card_block_count / microSD_block_size) * microSD_get_block_size();
}

/**
 * Returns the size of one block in bytes.
 *
 * Currently it's fixed at 512 Bytes.
 * \return Number of Bytes in one Block.
 */
uint16_t microSD_get_block_size(void) {
	return 512;
}

uint8_t microSD_is_SDSC(void) {
	return microSD_sdsc_card;
}

/**
 * Turns crc capabilities of the card on or off.
 *
 * Does not work properly at this time. (FIXME)
 * \param enable 0 if CRC should be disabled, 1 if it should be enabled
 * \return 0 on success, !=0 otherwise
 */
uint8_t microSD_set_CRC( uint8_t enable ) {
	uint8_t cmd[6] = { 0x7A, 0x00, 0x00, 0x00, 0x00, 0xFF };
	uint8_t ret;

	if( enable != 0 ) {
		cmd[1] = 0xFF;
		cmd[2] = 0xFF;
		cmd[3] = 0xFF;
		cmd[4] = 0xFF;
	}

	if( (ret = microSD_write_cmd( cmd, NULL )) != 0x00 ) {
		PRINTF("microSD_set_CRC(): ret = %u\n", ret);
		return 1;
	}

	microSD_crc_enable = cmd[1];
	mspi_chip_release(MICRO_SD_CS);

	return 0;
}

uint8_t microSD_read_csd( uint8_t *buffer ) {
	/*CMD9 this is just a guess, need to verify*/
	uint8_t cmd[6] = { 0x49, 0x00, 0x00, 0x00, 0x00, 0xFF };
	uint8_t i = 0;

	if( (i = microSD_write_cmd( cmd, NULL )) != 0x00 ) {
		PRINTF("microSD_read_csd(): CMD9 failure! (%u)\n", i);
		return 1;
	}

	/*wait for the 0xFE start byte*/
	while (mspi_transceive(MSPI_DUMMY_BYTE) != 0xFE) ;

	for (i = 0; i < 16; i++) {
		*buffer++ = mspi_transceive(MSPI_DUMMY_BYTE);
	}

	/*CRC-Byte: don't care*/
	mspi_transceive(MSPI_DUMMY_BYTE);
	mspi_transceive(MSPI_DUMMY_BYTE);

	/*release chip select and disable microSD spi*/
	mspi_chip_release(MICRO_SD_CS);

	return 0;
}

/**
 * Parse the given csd and extract the needed information out of it.
 *
 * \param *csd A Buffer in which the contents of the csd are saved (get them with microSD_read_csd()).
 */
void microSD_setSDCInfo( uint8_t *csd ) {
	uint8_t csd_version = ((csd[0] & (12 << 4)) >> 6);
	uint8_t READ_BL_LEN = 0;
	uint32_t C_SIZE = 0;
	uint16_t C_SIZE_MULT = 0;
	uint32_t mult = 0;

	if( csd_version == 0 ) {
		/*Bits 80 till 83 are the READ_BL_LEN*/
		READ_BL_LEN = csd[5] & 0x0F;

		/*Bits 62 - 73 are C_SIZE*/
		C_SIZE = ((csd[8] & 07000) >> 6) + (((uint32_t)csd[7]) << 2) + (((uint32_t)csd[6] & 07) << 10);

		/*Bits 47 - 49 are C_SIZE_MULT*/
		C_SIZE_MULT = ((csd[9] & 0x80) >> 7) + ((csd[8] & 0x03) << 1);

		mult = (2 << (C_SIZE_MULT + 2));
		microSD_card_block_count = (C_SIZE + 1) * mult;
		microSD_block_size = 1 << READ_BL_LEN;
	} else if( csd_version == 1 ) {
		/*Bits 80 till 83 are the READ_BL_LEN*/
		READ_BL_LEN = csd[5] & 0x0F;
		C_SIZE = csd[9] + (((uint32_t) csd[8]) << 8) + (((uint32_t) csd[7] & 0x3f) << 16);
		microSD_card_block_count = (C_SIZE + 1) * 1024;
		microSD_block_size = 512;
	}

	PRINTF("\nmicroSD_setSDCInfo(): CSD Version = %u", ((csd[0] & (12 << 4)) >> 6));
	PRINTF("\nmicroSD_setSDCInfo(): READ_BL_LEN = %u", READ_BL_LEN);
	PRINTF("\nmicroSD_setSDCInfo(): C_SIZE = %lu", C_SIZE);
	PRINTF("\nmicroSD_setSDCInfo(): C_SIZE_MULT = %u", C_SIZE_MULT);
	PRINTF("\nmicroSD_setSDCInfo(): mult = %lu", mult);
	PRINTF("\nmicroSD_setSDCInfo(): microSD_card_block_count  = %lu", microSD_card_block_count);
	PRINTF("\nmicroSD_setSDCInfo(): microSD_block_size  = %u", microSD_block_size);
}

/**
 * \brief This function calculates the CRC7 for SD Card commands.
 *
 * \param *cmd The array containing the command with every byte except
 *	the CRC byte correctly set. The CRC part will be written into the
 *	cmd array (at the correct position).
 */
void microSD_cmd_crc( uint8_t *cmd ) {
	uint32_t stream = (((uint32_t) cmd[0]) << 24) +
					  (((uint32_t) cmd[1]) << 16) +
					  (((uint32_t) cmd[2]) << 8) +
					  ((uint32_t) cmd[3]);
	uint8_t i = 0;
	uint32_t gen = ((uint32_t) 0x89) << 24;

	for( i = 0; i < 40; i++ ) {
		if( stream & (((uint32_t) 0x80) << 24)) {
			stream ^= gen;
		}

		stream = stream << 1;
		if( i == 7 ) {
			stream += cmd[4];
		}
	}

	cmd[5] = (stream >> 24) + 1;
}

/**
 * \brief This function calculates the CRC16 for a 512 Byte data block.
 *
 * \param *data The array containing the data to be send. Must be 512 Bytes long.
 * \return The CRC16 code for the given data.
 */
uint16_t microSD_data_crc( uint8_t *data ) {
	uint32_t stream = (((uint32_t) data[0]) << 24) +
					  (((uint32_t) data[1]) << 16) +
					  (((uint32_t) data[2]) << 8) +
					  ((uint32_t) data[3]);
	uint16_t i = 0;
	uint32_t gen = ((uint32_t) 0x11021) << 15;

	/* 4096 Bits = 512 Bytes a 8 Bits */
	for( i = 0; i < 4096; i++ ) {
		if( stream & (((uint32_t) 0x80) << 24)) {
			stream ^= gen;
		}

		stream = stream << 1;
		if( ((i + 1) % 8) == 0 && i < 4064) {
			stream += data[((i+1)/8)+3];
		}
	}

	return (stream >> 16);
}

uint8_t microSD_init(void) {
	uint16_t i;
	uint8_t ret = 0;
	microSD_sdsc_card = 1;

	/*set pin for mcro sd card power switch to output*/
	MICRO_SD_PWR_PORT_DDR |= (1 << MICRO_SD_PWR_PIN);

	/*switch off the sd card and tri state buffer whenever this is not done
	 *to avoid initialization failures */
	microSD_deinit();

	/*switch on the SD card and the tri state buffer*/
	MICRO_SD_PWR_PORT |= (1 << MICRO_SD_PWR_PIN);

	/*READY TO INITIALIZE micro SD / SD card*/
	mspi_chip_release(MICRO_SD_CS);

	/*init mspi in mode0, at chip select pin 2 and max baud rate*/
	mspi_init(MICRO_SD_CS, MSPI_MODE_0, MSPI_BAUD_2MBPS);

	/*set SPI mode by chip select (only necessary when mspi manager is active)*/

	/** FIXME: For unknown reasons we have to wait here, otherwise INGA will reset */
	_delay_ms(2);

	mspi_chip_select(MICRO_SD_CS);
	mspi_chip_release(MICRO_SD_CS);

	/*wait 1ms before initialize sd card*/
	_delay_ms(1);

	/*send >74 clock cycles to setup spi-mode*/
	for (i = 0; i < 16; i++) {
		mspi_transceive(MSPI_DUMMY_BYTE);
	}

	/*CMD0: set sd card to idle state*/
	uint8_t cmd0[6]  = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };

	/*CMD1: init CSD Version 1 and MMC cards*/
	uint8_t cmd1[6]  = { 0x41, 0x00, 0x00, 0x00, 0x00, 0xFF };

	/*CMD8: Test if the card supports CSD Version 2*/
	uint8_t cmd8[6]  = { 0x48, 0x00, 0x00, 0x01, 0xaa, 0x01 };

	/*CMD16: Sets the block length, needed for CSD Version 1 cards*/
	uint8_t cmd16[6] = { 0x50, 0x00, 0x00, 0x02, 0x00, 0x01 };

	/*CMD41: Second part of ACMD41*/
	uint8_t cmd41[6] = { 0x69, 0x40, 0x00, 0x00, 0x00, 0x01 };

	/*CMD55: First part of ACMD41, Initiates a App. Spec. Cmd*/
	uint8_t cmd55[6] = { 0x77, 0x00, 0x00, 0x00, 0x00, 0x01 };

	/*CMD58: Gets the OCR-Register to check if card is SDSC or not*/
	uint8_t cmd58[6] = { 0x7a, 0x00, 0x00, 0x00, 0x00, 0x01 };

	/*Response Array for the R3 and R7 responses*/
	uint8_t resp[5]  = { 0x00, 0x00, 0x00, 0x00, 0x00 };

	/*Memory for the contents of the csd*/
	uint8_t csd[16];

	PRINTF("\nmicroSD_init(): Writing cmd0");

	i = 0;
	while( (ret = microSD_write_cmd( cmd0, NULL )) != 0x01 ) {
		i++;
		if (i > 200) {
			mspi_chip_release(MICRO_SD_CS);
			microSD_deinit();

			PRINTF("\nmicroSD_init(): cmd0 timeout -> %d", ret);

			return 1;
		}
	}

	/*send CMD8*/
	microSD_cmd_crc( cmd8 );
	PRINTF("\nmicroSD_init(): Writing cmd8");

	resp[0] = 0x07;
	i = 0;
	while( (ret = microSD_write_cmd( cmd8, resp )) != 0x01 ) {
		if( (ret & 0x04) && ret != 0xFF ) {
			PRINTF("\nmicroSD_init(): cmd8 not supported -> legacy card");

			break;
		}
		i++;
		if( i > 200 ) {
			mspi_chip_release( MICRO_SD_CS );
			microSD_deinit();
			PRINTF("\nmicroSD_init(): cmd8 timeout -> %d", ret);

			return 4;
		}
	}

	if( ret & 0x04 ) {
		/*prepare next 6 data bytes: CMD1*/
		PRINTF("\nmicroSD_init(): Writing cmd1");

		i = 0;
		while( (ret = microSD_write_cmd( cmd1, NULL )) != 0 ) {
			i++;
			if (i > 5500) {
				PRINTF("\nmicroSD_init(): cmd1 timeout reached, last return value was %d", ret);

				mspi_chip_release(MICRO_SD_CS);
				microSD_deinit();
				return 2;
			}
		}
		/* Set Block Length to 512 Bytes */
		i = 0;
		while( (ret = microSD_write_cmd( cmd16, NULL )) != 0x00 ) {
			i++;
			if (i > 500) {
				PRINTF("\nmicroSD_init(): cmd16 timeout reached, last return value was %d", ret);

				mspi_chip_release(MICRO_SD_CS);
				microSD_deinit();
				return 5;
			}
		}
	} else {
		PRINTF("\nWriting acmd41 (55 + 41)");
		PRINTF("\nmicroSD_init():\tWriting cmd55");

		resp[0] = 0x01;
		uint16_t j=0;
		do{
			j++;
			i = 0;
			while( microSD_write_cmd( cmd55, NULL ) != 0x01 ) {
				i++;
				if (i > 500) {
					
					PRINTF("\nmicroSD_init(): acmd41 timeout reached, last return value was %u", ret);
					
					mspi_chip_release(MICRO_SD_CS);
					microSD_deinit();
					return 6;
				}
			}
			
			PRINTF("\nmicroSD_init():\tWriting cmd41");
			
			if(j>12000){
				return 8;
			}
		}while( (microSD_write_cmd( cmd41, NULL ) != 0)  );
		resp[0] = 0x03;
		i = 0;

		PRINTF("\nmicroSD_init(): Writing cmd58");

		while( (ret = microSD_write_cmd( cmd58, resp )) != 0x0 ) {
			i++;
			if (i > 900) {
				PRINTF("\nmicroSD_init(): cmd58 timeout reached, last return value was %d", ret);

				mspi_chip_release(MICRO_SD_CS);
				microSD_deinit();
				return 7;
			}
		}

		if( resp[1] & 0x80 ) {
			if( resp[1] & 0x40 ) {
				microSD_sdsc_card = 0;
			}

			PRINTF("\nmicroSD_init(): acmd41: Card power up okay!");
			if( resp[1] & 0x40 ) {
				PRINTF("\nmicroSD_init(): acmd41: Card is SDHC/SDXC");
			} else {
				PRINTF("\nmicroSD_init(): acmd41: Card is SDSC");
			}

		}
	}

	i=0;
	while( microSD_read_csd( csd ) != 0 ) {
		i++;
		if (i > 900) {
			mspi_chip_release(MICRO_SD_CS);
			microSD_deinit();
			return 3;
		}
	}
	microSD_setSDCInfo( csd );
	mspi_chip_release(MICRO_SD_CS);

	return 0;
}

uint8_t microSD_read_block(uint32_t addr, uint8_t *buffer) {
	uint16_t i;
	uint8_t ret;
	/*CMD17 read block*/
	uint8_t cmd[6] = { 0x51, 0x00, 0x00, 0x00, 0x00, 0xff };

	/* calculate the start address: block_addr = addr * 512
	 * this is only needed if the card is a SDSC card and uses
	 * byte addressing (Block size of 512 is set in microSD_init()).
	 * SDHC and SDXC card use block-addressing with a block size of
	 * 512 Bytes.
	 */
	if( microSD_sdsc_card ) {
		addr = addr << 9;
	}

	/* create cmd bytes according to the address
	 * Note that cmd[4] will always be 0 for SDSC cards,
	 * because of the shift above
	 */
	cmd[1] = ((addr & 0xFF000000) >> 24);
	cmd[2] = ((addr & 0x00FF0000) >> 16);
	cmd[3] = ((addr & 0x0000FF00) >> 8);
	cmd[4] = ((addr & 0x000000FF));

	/* send CMD17 with address information. Chip select is done by
	 * the microSD_write_cmd method and */
	if ((i = microSD_write_cmd( cmd, NULL)) != 0x00) {
		PRINTF("\nmicroSD_read_block(): CMD17 failure! (%u)",i);
		return 1;
	}

	/*wait for the 0xFE start byte*/
    uint8_t success=0;
	for(i = 0; i < 100; i++) {
		if((ret = mspi_transceive(MSPI_DUMMY_BYTE)) == 0xFE ) {
			success=1;
			break;
		}
	}
	if(success == 0){
		PRINTF("\nmicroSD_read_block(): No Start Byte recieved, last was %d", ret);
		return 2;
    }

	for (i = 0; i < 512; i++) {
		buffer[i] = mspi_transceive(MSPI_DUMMY_BYTE);
	}

	/*CRC-Byte: don't care*/
	mspi_transceive(MSPI_DUMMY_BYTE);
	mspi_transceive(MSPI_DUMMY_BYTE);

	/*release chip select and disable microSD spi*/
	mspi_chip_release(MICRO_SD_CS);

	return 0;
}

uint8_t microSD_deinit(void) {
	MICRO_SD_PWR_PORT &= ~(1 << MICRO_SD_PWR_PIN);
	return 0;
}

uint16_t microSD_get_status(void) {
	uint8_t cmd[6] = { 0x4D, 0x00, 0x00, 0x00, 0x00, 0xFF };
	uint8_t resp[5]  = { 0x02, 0x00, 0x00, 0x00, 0x00 };

	if (microSD_write_cmd(cmd, resp) != 0x00) {
		return 0;
	}

	return ((uint16_t) resp[1] << 8) + ((uint16_t) resp[0]);
}

uint8_t microSD_write_block(uint32_t addr, uint8_t *buffer) {
	uint16_t i;

	/*CMD24 write block*/
	uint8_t cmd[6] = { 0x58, 0x00, 0x00, 0x00, 0x00, 0xFF };

	/* calculate the start address: block_addr = addr * 512
	 * this is only needed if the card is a SDSC card and uses
	 * byte addressing (Block size of 512 is set in microSD_init()).
	 * SDHC and SDXC card use block-addressing with a block size of
	 * 512 Bytes.
	 */
	if( microSD_sdsc_card ) {
		addr = addr << 9;
	}

	/* create cmd bytes according to the address
	 * Note that cmd[4] will always be 0 for SDSC cards,
	 * because of the shift above
	 */
	cmd[1] = ((addr & 0xFF000000) >> 24);
	cmd[2] = ((addr & 0x00FF0000) >> 16);
	cmd[3] = ((addr & 0x0000FF00) >> 8);
	cmd[4] = ((addr & 0x000000FF));

	/* send CMD24 with address information. Chip select is done by
	 * the microSD_write_cmd method and */
	if ((i = microSD_write_cmd( cmd, NULL)) != 0x00) {
		mspi_chip_release(MICRO_SD_CS);
		return 1;
	}
	mspi_transceive(MSPI_DUMMY_BYTE);

	/* send start byte 0xFE to the microSD card to symbolize the beginning
	 * of one data block (512byte)*/
	mspi_transceive(0xFE);

	/*send 1 block (512byte) to the microSD card*/
	for (i = 0; i < 512; i++) {
		mspi_transceive(buffer[i]);
	}

	/*write CRC checksum: Dummy*/
	mspi_transceive(MSPI_DUMMY_BYTE);
	mspi_transceive(MSPI_DUMMY_BYTE);

	/*failure check: Data Response XXX00101 = OK*/
	if (((i = mspi_transceive(MSPI_DUMMY_BYTE)) & 0x1F) != 0x05) {
		mspi_chip_release(MICRO_SD_CS);
		return 2;
	}

	/*wait while microSD card is busy*/
	i=0;
	while ((mspi_transceive(MSPI_DUMMY_BYTE) != 0xff)&& (i<100)) {
		i++;
	}
	/*release chip select and disable microSD spi*/
	mspi_chip_release(MICRO_SD_CS);

	return 0;
}

uint8_t microSD_write_cmd(uint8_t *cmd, uint8_t *resp) {
	uint16_t i;
	uint8_t data;
	uint8_t idx = 0;
	uint8_t resp_type = 0x01;

	if( resp != NULL ) {
		resp_type = resp[0];
	}

	if( microSD_crc_enable ) {
		microSD_cmd_crc( cmd );
	}

	mspi_chip_release(MICRO_SD_CS);
	mspi_transceive(MSPI_DUMMY_BYTE);

	/*begin to send 6 command bytes to the sd card*/
	mspi_chip_select(MICRO_SD_CS);

	for (i = 0; i < 6; i++) {
		mspi_transceive(*(cmd+i));
	}

	i = 0;

	/*wait for the answer of the sd card*/
	do {
		/*0x01 for acknowledge*/
		data = mspi_transceive(MSPI_DUMMY_BYTE);
		if (i > 500) {
			break;
		}
		watchdog_periodic();
		i++;
		if( resp != NULL && data != 0xFF) {
			if( resp_type == 0x01 ) {
				resp[0] = data;
			} else if( resp_type == 0x03 || resp_type == 0x07) {
				i = 0;
				resp[idx] = data;
				idx++;
				if( idx >= 5 || (resp[0] & 0xFE) != 0 ) {
					break;
				}
				data = 0xFF;
				continue;
			} else if( resp_type == 0x02 ) {
				i = 0;
				resp[idx] = data;
				idx++;
				if( idx >= 2 || (resp[0] & 0xFE) != 0 ) {
					break;
				}
				data = 0xFF;
				continue;
			}
		}
	} while (data == 0xFF);

	return data;
}

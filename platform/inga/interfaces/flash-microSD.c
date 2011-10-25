/* Copyright (c) 2010, Ulf Kulau
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
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
 *
 */

#include "flash-microSD.h"

uint8_t microSD_init(void) {
	uint16_t i;
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
	mspi_init(MICRO_SD_CS, MSPI_MODE_0, MSPI_BAUD_MAX);
	/*set SPI mode by chip select (only necessary when mspi manager is active)*/

	mspi_chip_select(MICRO_SD_CS);
	mspi_chip_release(MICRO_SD_CS);
	/*wait 1ms before initialize sd card*/
	_delay_ms(1);
	/*send >74 clock cycles to setup spi-mode*/
	for (i = 0; i < 16; i++) {
		mspi_transceive(MSPI_DUMMY_BYTE);
	}
	/*CMD0: set sd card to idle state*/
	uint8_t cmd[6] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x95 };
	i = 0;
	while (microSD_write_cmd(&cmd[0]) != 0x01) {
		i++;
		if (i > 200) {
			mspi_chip_release(MICRO_SD_CS);
			microSD_deinit();
			return 1;
		}
	}
	i = 0;
	/*prepare next 6 data bytes: CMD1*/
	cmd[0] = 0x41;
	cmd[5] = 0xFF;
	while (microSD_write_cmd(&cmd[0]) != 0) {
		i++;
		if (i > 500) {
			mspi_chip_release(MICRO_SD_CS);
			microSD_deinit();
			return 2;
		}
	}
	mspi_chip_release(MICRO_SD_CS);
	return 0;
}

uint8_t microSD_read_block(uint32_t addr, uint8_t *buffer) {
	uint16_t i;
	/*CMD17 read block*/
	uint8_t cmd[6] = { 0x51, 0x00, 0x00, 0x00, 0x00, 0xff };
	/*calculate the start address: block_addr = addr * 512*/
	addr = addr << 9;
	/*create cmd bytes according to the address*/
	cmd[1] = ((addr & 0xFF000000) >> 24);
	cmd[2] = ((addr & 0x00FF0000) >> 16);
	cmd[3] = ((addr & 0x0000FF00) >> 8);

	/* send CMD17 with address information. Chip select is done by
	 * the microSD_write_cmd method and */
	if (microSD_write_cmd(&cmd[0]) != 0x00) {
#if DEBUG
		printf("\nCMD17 failure!");
#endif
		return 1;
	}

	/*wait for the 0xFE start byte*/
	while (mspi_transceive(MSPI_DUMMY_BYTE) != 0xFE) {
	};

	for (i = 0; i < 512; i++) {
		*buffer++ = mspi_transceive(MSPI_DUMMY_BYTE);
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

uint8_t microSD_write_block(uint32_t addr, uint8_t *buffer) {
	uint16_t i;
	/*CMD24 write block*/
	uint8_t cmd[6] = { 0x58, 0x00, 0x00, 0x00, 0x00, 0xFF };

	/*calculate the start address: block_addr = addr * 512*/
	addr = addr << 9;
	/*create cmd bytes according to the address*/
	cmd[1] = ((addr & 0xFF000000) >> 24);
	cmd[2] = ((addr & 0x00FF0000) >> 16);
	cmd[3] = ((addr & 0x0000FF00) >> 8);
	/* send CMD24 with address information. Chip select is done by
	 * the microSD_write_cmd method and */
	if (microSD_write_cmd(&cmd[0]) != 0x00) {
#if DEBUG
		printf("\nCMD24 failure!");
#endif
		return -1;
	}

	for (i = 0; i < 10; i++) {
		mspi_transceive(MSPI_DUMMY_BYTE);
	}

	/* send start byte 0xFE to the microSD card to symbolize the beginning
	 * of one data block (512byte)*/
	mspi_transceive(0xFE);

	/*send 1 block (512byte) to the microSD card*/
	for (i = 0; i < 512; i++) {
		mspi_transceive(*buffer++);
	}

	/*write CRC checksum: Dummy*/
	mspi_transceive(MSPI_DUMMY_BYTE);
	mspi_transceive(MSPI_DUMMY_BYTE);

	/*failure check: Data Response XXX00101 = OK*/
	if ((mspi_transceive(MSPI_DUMMY_BYTE) & 0x1F) != 0x05) {
#if DEBUG
		printf("\nblock failure!");
#endif
		return -1;
	}

	/*wait while microSD card is busy*/
	while (mspi_transceive(MSPI_DUMMY_BYTE) != 0xff) {
	};
	/*release chip select and disable microSD spi*/
	mspi_chip_release(MICRO_SD_CS);

	return 0;
}

uint8_t microSD_write_cmd(uint8_t *cmd) {
	uint16_t i;
	uint8_t data;

	mspi_chip_release(MICRO_SD_CS);
	mspi_transceive(MSPI_DUMMY_BYTE);
	/*begin to send 6 command bytes to the sd card*/
	mspi_chip_select(MICRO_SD_CS);

	for (i = 0; i < 6; i++) {
		mspi_transceive(*cmd++);
	}
	i = 0;
	/*wait for the answer of the sd card*/
	do {
		/*0x01 for acknowledge*/
		data = mspi_transceive(MSPI_DUMMY_BYTE);
		if (i > 500) {
#if DEBUG
			printf("\nwrite_cmd timeout!");
#endif
			break;
		}
		i++;
	} while (data == 0xFF);
	return data;
}

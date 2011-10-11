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
 * \addtogroup Drivers
 * @{
 *
 * \defgroup spi_bus_manager Master SPI Bus Manager
 *
 * <p>The various SPI devices are all connected to the SPI Bus (SCK, MOSI/SDA,
 *    MISO/SDI) and only separated by the Chip Select. With a high probability,
 *    not every SPI Device will use the same SPI Bus configuration (SPI Mode), so
 *    you would have to change (or to check) the SPI mode whenever accessing another
 *    SPI device. But in higher software layers you are not interested in such
 *    details like SPI Mode. Therefore the SPI Bus Manager was implemented, to separate
 *    the low level hardware and register level from higher software layers.
 *    The SPI Bus Manager holds all devices, which are connected to the SPI Bus. With
 *    a checksum he can decide, if the SPI configuration has to change. So in some cases
 *    these reconfiguration can be avoid.</p>
 *    \note If you know what you are doing, it is possible to disable the SPI Bus Manager
 *    by setting MSPI_BUS_MANAGER in the mspi-drv.h to 0.
 * @{
 *
 */

/**
 * \file
 *		Master SPI Bus Manager definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

#ifndef MSPIMGR_H_
#define MSPIMGR_H_
#include <avr/io.h>

/*!
 * Defines the maximum number of SPI devices
 * \note In our case we have 7 (2³-1) possible SPI devices, because
 * we are using 3 I/O pins and a BCD-to-decimal encoder for the
 * chip select.
 */
#define MAX_SPI_DEVICES			7

typedef struct {
	uint8_t dev_mode;
	uint16_t dev_baud;
	uint8_t checksum;
}spi_dev;
/*!
 * SPI Device Table: Holds the information about the SPI devices.
 * \note Index contains Chip Select information
 */
static spi_dev spi_bus_config[MAX_SPI_DEVICES];

/*!
 * Holds the current SPI-Bus configuration
 */
static uint8_t spi_current_config = 0xFF;

/**
 * \brief This function add a device to the SPI device table
 *        and calculates the specific checksum
 *
 * \param cs  Chip Select: Device ID
 * \param mode Select the (M)SPI mode (MSPI_MODE_0 ...
 * 	      MSPI_MODE_3)
 * \param baud The MSPI BAUD rate. Sometimes it is necessary
 *        to reduce the SCK. Use MSPI_BAUD_MAX in common case.
 *
 */
void add_to_spi_mgr(uint8_t cs, uint8_t mode, uint16_t baud);

/**
 * \brief This function changes the SPI configuration
 *
 * \param spi_dev The specified entry of the SPI Device Table
 *
 */
void change_spi_mode(spi_dev new_config);





#endif /* SPIMGR_H_ */

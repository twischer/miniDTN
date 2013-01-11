/*
 * Copyright (c) 2012, TU Braunschweig.
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
 * \file
 *		Master SPI Bus Manager definitions
 * \author
 *      Ulf Kulau <kulau@ibr.cs.tu-bs.de>
 */

/**
 * \addtogroup inga_bus_driver
 * @{
 */

/**
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

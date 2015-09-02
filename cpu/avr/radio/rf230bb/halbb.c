/*   Copyright (c) 2009, Swedish Institute of Computer Science
 *  All rights reserved. 
 *
 *  Additional fixes for AVR contributed by:
 *
 *	Colin O'Flynn coflynn@newae.com
 *	Eric Gnoske egnoske@gmail.com
 *	Blake Leverett bleverett@gmail.com
 *	Mike Vidales mavida404@gmail.com
 *	Kevin Brown kbrown3@uccs.edu
 *	Nate Bohlmann nate@elfwerks.com
 *	David Kopf dak664@embarqmail.com
 *  Ivan Delamer delamer@ieee.com
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * 
*/

/**
 *   \addtogroup wireless
 *  @{
*/

/**
 *   \defgroup hal RF230 hardware level drivers
 *   @{
 */

/**
 *  \file
 *  This file contains low-level radio driver code.
 *  This version is optimized for use with the "barebones" RF230bb driver,
 *  which communicates directly with the contiki core MAC layer.
 *  It is optimized for speed at the expense of generality.
 */
#include "contiki-conf.h"
#if DEBUGFLOWSIZE
extern uint8_t debugflowsize,debugflow[DEBUGFLOWSIZE];
#define DEBUGFLOW(c) if (debugflowsize<(DEBUGFLOWSIZE-1)) debugflow[debugflowsize++]=c
#else
#define DEBUGFLOW(c)
#endif


/*============================ INCLUDE =======================================*/
#include <stdio.h>
#include <stdlib.h>

#include "led.h"
#include "delay.h"
#include "hal.h"

#include "at86rf230_registermap.h"


/*============================ VARIABLES =====================================*/

volatile extern signed char rf230_last_rssi;
static volatile bool rx_buffer_overflow = false;

/*============================ CALLBACKS =====================================*/


/*============================ IMPLEMENTATION ================================*/

// SPI transfers
#define HAL_SPI_TRANSFER_OPEN() { uint8_t spiTemp; \
  HAL_ENTER_CRITICAL_REGION();	  \
  HAL_SS_LOW(); /* Start the SPI transaction by pulling the Slave Select low. */
#define HAL_SPI_TRANSFER_WRITE(to_write) (spiTemp = SPI2_SendReceiveByte(to_write))
#define HAL_SPI_TRANSFER_WAIT()  ({0;})
#define HAL_SPI_TRANSFER_READ() (spiTemp)
#define HAL_SPI_TRANSFER_CLOSE() \
    HAL_SS_HIGH(); /* End the transaction by pulling the Slave Select High. */ \
    HAL_LEAVE_CRITICAL_REGION(); \
    }
#define HAL_SPI_TRANSFER(to_write) (spiTemp = SPI2_SendReceiveByte(to_write))

bool
hal_rx_buffer_overflow()
{
	const bool last = rx_buffer_overflow;
	rx_buffer_overflow = false;

	return last;
}



/** \brief  This function initializes the Hardware Abstraction Layer.
 */
void
hal_init(void)
{
	/*
	 * The full initalisation will be done
	 * by the STM32-CubeMX generated files.
	 * See directory "Src" for detailed information.
	 */

	/* To avoid a SPI glitch, the port register shall be set before the port will be intitialized */
	HAL_SS_HIGH();
}


/*----------------------------------------------------------------------------*/
/** \brief  This function reads data from one of the radio transceiver's registers.
 *
 *  \param  address Register address to read from. See datasheet for register
 *                  map.
 *
 *  \see Look at the at86rf230_registermap.h file for register address definitions.
 *
 *  \returns The actual value of the read register.
 */
uint8_t
hal_register_read(uint8_t address)
{
    uint8_t register_value;
    /* Add the register read command to the register address. */
    /* Address should be < 0x2f so no need to mask */
//  address &= 0x3f;
    address |= 0x80;

    HAL_SPI_TRANSFER_OPEN();

    /*Send Register address and read register content.*/
    HAL_SPI_TRANSFER(address);
    register_value = HAL_SPI_TRANSFER(0);

    HAL_SPI_TRANSFER_CLOSE();

    return register_value;
}

/*----------------------------------------------------------------------------*/
/** \brief  This function writes a new value to one of the radio transceiver's
 *          registers.
 *
 *  \see Look at the at86rf230_registermap.h file for register address definitions.
 *
 *  \param  address Address of register to write.
 *  \param  value   Value to write.
 */
void
hal_register_write(uint8_t address, uint8_t value)
{
    /* Add the Register Write (short mode) command to the address. */
    address = 0xc0 | address;

    HAL_SPI_TRANSFER_OPEN();

    /*Send Register address and write register content.*/
    HAL_SPI_TRANSFER(address);
    HAL_SPI_TRANSFER(value);

    HAL_SPI_TRANSFER_CLOSE();
}
/*----------------------------------------------------------------------------*/
/** \brief  This function reads the value of a specific subregister.
 *
 *  \see Look at the at86rf230_registermap.h file for register and subregister
 *       definitions.
 *
 *  \param  address  Main register's address.
 *  \param  mask  Bit mask of the subregister.
 *  \param  position   Bit position of the subregister
 *  \retval Value of the read subregister.
 */
uint8_t
hal_subregister_read(uint8_t address, uint8_t mask, uint8_t position)
{
    /* Read current register value and mask out subregister. */
    uint8_t register_value = hal_register_read(address);
    register_value &= mask;
    register_value >>= position; /* Align subregister value. */

    return register_value;
}
/*----------------------------------------------------------------------------*/
/** \brief  This function writes a new value to one of the radio transceiver's
 *          subregisters.
 *
 *  \see Look at the at86rf230_registermap.h file for register and subregister
 *       definitions.
 *
 *  \param  address  Main register's address.
 *  \param  mask  Bit mask of the subregister.
 *  \param  position  Bit position of the subregister
 *  \param  value  Value to write into the subregister.
 */
void
hal_subregister_write(uint8_t address, uint8_t mask, uint8_t position,
                            uint8_t value)
{
    /* Read current register value and mask area outside the subregister. */
    volatile uint8_t register_value = hal_register_read(address);
    register_value &= ~mask;

    /* Start preparing the new subregister value. shift in place and mask. */
    value <<= position;
    value &= mask;

    value |= register_value; /* Set the new subregister value. */

    /* Write the modified register value. */
    hal_register_write(address, value);
}

/*----------------------------------------------------------------------------*/
/** \brief  Transfer a frame from the radio transceiver to a RAM buffer
 *
 *          This version is optimized for use with contiki RF230BB driver.
 *          The callback routine and CRC are left out for speed in reading the rx buffer.
 *          Any delays here can lead to overwrites by the next packet!
 *
 *          If the frame length is out of the defined bounds, the length, lqi and crc
 *          are set to zero.
 *
 *  \param  rx_frame    Pointer to the data structure where the frame is stored.
 */
void
hal_frame_read(hal_rx_frame_t *rx_frame)
{
	uint8_t frame_length, *rx_data;

    /*Send frame read (long mode) command.*/
    HAL_SPI_TRANSFER_OPEN();
    HAL_SPI_TRANSFER(0x20);

    /*Read frame length. This includes the checksum. */
    frame_length = HAL_SPI_TRANSFER(0);

    /*Check for correct frame length. Bypassing this test can result in a buffer overrun! */
    if ((frame_length < HAL_MIN_FRAME_LENGTH) || (frame_length > HAL_MAX_FRAME_LENGTH)) {
        /* Length test failed */
        rx_frame->length = 0;
        rx_frame->lqi    = 0;
        rx_frame->crc    = false;
    }
    else {
        rx_data = (rx_frame->data);
        rx_frame->length = frame_length;

        /*Transfer frame buffer to RAM buffer */

        HAL_SPI_TRANSFER_WRITE(0);
        HAL_SPI_TRANSFER_WAIT();
        do{
            *rx_data++ = HAL_SPI_TRANSFER_READ();
            HAL_SPI_TRANSFER_WRITE(0);

            /* CRC was checked in hardware, but redoing the checksum here ensures the rx buffer
             * is not being overwritten by the next packet. Since that lengthy computation makes
             * such overwrites more likely, we skip it and hope for the best.
             * Without the check a full buffer is read in 320us at 2x spi clocking.
             * The 802.15.4 standard requires 640us after a greater than 18 byte frame.
             * With a low interrupt latency overwrites should never occur.
             */
    //          crc = _crc_ccitt_update(crc, tempData);

            HAL_SPI_TRANSFER_WAIT();

        } while (--frame_length > 0);


        /*Read LQI value for this frame.*/
        rx_frame->lqi = HAL_SPI_TRANSFER_READ();

        /* If crc was calculated set crc field in hal_rx_frame_t accordingly.
         * Else show the crc has passed the hardware check.
         */
        rx_frame->crc   = true;
    }

	HAL_SPI_TRANSFER_CLOSE();
}

/*----------------------------------------------------------------------------*/
/** \brief  This function will download a frame to the radio transceiver's frame
 *          buffer.
 *
 *  \param  write_buffer    Pointer to data that is to be written to frame buffer.
 *  \param  length          Length of data. The maximum length is 127 bytes.
 */
void
hal_frame_write(uint8_t *write_buffer, uint8_t length)
{
    /* Optionally truncate length to maximum frame length.
     * Not doing this is a fast way to know when the application needs fixing!
     */
//  length &= 0x7f; 

    HAL_SPI_TRANSFER_OPEN();

    /* Send Frame Transmit (long mode) command and frame length */
    HAL_SPI_TRANSFER(0x60);
    HAL_SPI_TRANSFER(length);

    /* Download to the Frame Buffer.
     * When the FCS is autogenerated there is no need to transfer the last two bytes
     * since they will be overwritten.
     */
#if !RF230_CONF_CHECKSUM
    length -= 2;
#endif
    do HAL_SPI_TRANSFER(*write_buffer++); while (--length);

	HAL_SPI_TRANSFER_CLOSE();
}

/*----------------------------------------------------------------------------*/
/** \brief Read SRAM
 *
 * This function reads from the SRAM of the radio transceiver.
 *
 * \param address Address in the TRX's SRAM where the read burst should start
 * \param length Length of the read burst
 * \param data Pointer to buffer where data is stored.
 */
#if 0  //Uses 80 bytes (on Raven) omit unless needed
void
hal_sram_read(uint8_t address, uint8_t length, uint8_t *data)
{
    HAL_SPI_TRANSFER_OPEN();

    /*Send SRAM read command and address to start*/
    HAL_SPI_TRANSFER(0x00);
    HAL_SPI_TRANSFER(address);

    HAL_SPI_TRANSFER_WRITE(0);
    HAL_SPI_TRANSFER_WAIT();

    /*Upload the chosen memory area.*/
    do{
        *data++ = HAL_SPI_TRANSFER_READ();
        HAL_SPI_TRANSFER_WRITE(0);
        HAL_SPI_TRANSFER_WAIT();
    } while (--length > 0);

    HAL_SPI_TRANSFER_CLOSE();
}
#endif
/*----------------------------------------------------------------------------*/
/** \brief Write SRAM
 *
 * This function writes into the SRAM of the radio transceiver. It can reduce
 * SPI transfers if only part of a frame is to be changed before retransmission.
 *
 * \param address Address in the TRX's SRAM where the write burst should start
 * \param length  Length of the write burst
 * \param data    Pointer to an array of bytes that should be written
 */
#if 0  //omit unless needed
void
hal_sram_write(uint8_t address, uint8_t length, uint8_t *data)
{
    HAL_SPI_TRANSFER_OPEN();

    /*Send SRAM write command.*/
    HAL_SPI_TRANSFER(0x40);

    /*Send address where to start writing to.*/
    HAL_SPI_TRANSFER(address);

    /*Upload the chosen memory area.*/
    do{
        HAL_SPI_TRANSFER(*data++);
    } while (--length > 0);

    HAL_SPI_TRANSFER_CLOSE();

}
#endif

/*----------------------------------------------------------------------------*/
/* This #if compile switch is used to provide a "standard" function body for the */
/* doxygen documentation. */
#if defined(DOXYGEN)
/** \brief ISR for the radio IRQ line, triggered by the input capture.
 *  This is the interrupt service routine for timer1.ICIE1 input capture.
 *  It is triggered of a rising edge on the radio transceivers IRQ line.
 */
void RADIO_VECT(void);
#else  /* !DOXYGEN */
/* These link to the RF230BB driver in rf230.c */
void rf230_interrupt(void);

extern hal_rx_frame_t rxframe[RF230_CONF_RX_BUFFERS];
extern uint8_t rxframe_head,rxframe_tail;

/* rf230interruptflag can be printed in the main idle loop for debugging */
#define DEBUG 0
#if DEBUG
volatile char rf230interruptflag;
#define INTERRUPTDEBUG(arg) rf230interruptflag=arg
#else
#define INTERRUPTDEBUG(arg)
#endif


/* Separate RF230 has a single radio interrupt and the source must be read from the IRQ_STATUS register */
void hal_rf230_isr()
{
	LED_On(LED_BLUE);

	INTERRUPTDEBUG(1);

	/* used after HAL_SPI_TRANSFER_OPEN/CLOSE block */
	uint8_t interrupt_source = 0;

	/* Using SPI bus from ISR is generally a bad idea... */
	/* Note: all IRQ are not always automatically disabled when running in ISR */
	HAL_SPI_TRANSFER_OPEN();

	/*Read Interrupt source.*/
	/*Send Register address and read register content.*/
	HAL_SPI_TRANSFER_WRITE(0x80 | RG_IRQ_STATUS);

	HAL_SPI_TRANSFER_WAIT(); /* AFTER possible interleaved processing */

	interrupt_source = HAL_SPI_TRANSFER(0);

	HAL_SPI_TRANSFER_CLOSE();



	/*Handle the incomming interrupt. Prioritized.*/
	if ((interrupt_source & HAL_RX_START_MASK)) {
		INTERRUPTDEBUG(10);
		/* Save RSSI for this packet if not in extended mode, scaling to 1dB resolution */
#if !RF230_CONF_AUTOACK
#if 0  // 3-clock shift and add is faster on machines with no hardware multiply
		// With -Os avr-gcc saves a byte by using the general routine for multiply by 3
		rf230_last_rssi = hal_subregister_read(SR_RSSI);
		rf230_last_rssi = (rf230_last_rssi <<1)  + rf230_last_rssi;
#else  // Faster with 1-clock multiply. Raven and Jackdaw have 2-clock multiply so same speed while saving 2 bytes of program memory
		rf230_last_rssi = 3 * hal_subregister_read(SR_RSSI);
#endif
#endif

	} else if (interrupt_source & HAL_TRX_END_MASK){
		INTERRUPTDEBUG(11);

		const uint8_t state = hal_subregister_read(SR_TRX_STATUS);
		if((state == BUSY_RX_AACK) || (state == RX_ON) || (state == BUSY_RX) || (state == RX_AACK_ON)) {


			/* Received packet interrupt */
			LED_On(LED_ORANGE);

			/* Buffer the frame and call rf230_interrupt to schedule poll for rf230 receive process */
			if (rxframe[rxframe_tail].length) {
				rx_buffer_overflow = true;
				INTERRUPTDEBUG(42);
			} else {
				INTERRUPTDEBUG(12);
			}

#ifdef RF230_MIN_RX_POWER
			/* Discard packets weaker than the minimum if defined. This is for testing miniature meshes.*/
			/* Save the rssi for printing in the main loop */
#if RF230_CONF_AUTOACK
			//rf230_last_rssi=hal_subregister_read(SR_ED_LEVEL);
			rf230_last_rssi=hal_register_read(RG_PHY_ED_LEVEL);
#endif
			if (rf230_last_rssi >= RF230_MIN_RX_POWER) {
#endif
				hal_frame_read(&rxframe[rxframe_tail]);
				rxframe_tail++;if (rxframe_tail >= RF230_CONF_RX_BUFFERS) rxframe_tail=0;
				rf230_interrupt();
#ifdef RF230_MIN_RX_POWER
			}
#endif
		}
	} else if (interrupt_source & HAL_TRX_UR_MASK){
		INTERRUPTDEBUG(13);
		;
	} else if (interrupt_source & HAL_PLL_UNLOCK_MASK){
		INTERRUPTDEBUG(14);
		;
	} else if (interrupt_source & HAL_PLL_LOCK_MASK){
		INTERRUPTDEBUG(15);
		;
	} else if (interrupt_source & HAL_BAT_LOW_MASK){
		/*  Disable BAT_LOW interrupt to prevent endless interrupts. The interrupt */
		/*  will continously be asserted while the supply voltage is less than the */
		/*  user-defined voltage threshold. */
		uint8_t trx_isr_mask = hal_register_read(RG_IRQ_MASK);
		trx_isr_mask &= ~HAL_BAT_LOW_MASK;
		hal_register_write(RG_IRQ_MASK, trx_isr_mask);
		INTERRUPTDEBUG(16);
		;
	} else {
		INTERRUPTDEBUG(99);
		;
		LED_On(LED_RED);
	}
	LED_Off(LED_BLUE);
}
#   endif /* defined(DOXYGEN) */

/** @} */
/** @} */

/*EOF*/

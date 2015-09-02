/*   Copyright (c) 2008, Swedish Institute of Computer Science
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
 *  David Kopf dak664@embarqmail.com
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
 */

/**
 *    \addtogroup hal
 *    @{
 */

/**
 *  \file
 *  \brief This file contains low-level radio driver code.
 *
*/

#ifndef HAL_AVR_H
#define HAL_AVR_H
/*============================ INCLUDE =======================================*/
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "spi.h"

#include "FreeRTOS.h"
#include "task.h"


#include "contiki-conf.h"

/*============================ MACROS ========================================*/

/** \name This is the list of pin configurations needed for a given platform.
 * \brief Change these values to port to other platforms.
 * \{
 */
///* 1284 inga */
#define SSPORT		GPIOB
#define SSPIN		GPIO_PIN_14
#define RSTPORT		GPIOB
#define RSTPIN		GPIO_PIN_0
#define SLPTRPORT	GPIOB
// TODO check if if is always used as mask and not as number
#define SLPTRPIN	GPIO_PIN_1

/** \} */



/**
 * \name Pin macros
 * \brief These macros convert the platform-specific pin defines into names and functions
 *       that the source code can directly use.
 * \{
 */

#define hal_set_slptr_high()	(SLPTRPORT->BSRR = SLPTRPIN)      /**< This macro pulls the SLP_TR pin high. */
#define hal_set_slptr_low()		(SLPTRPORT->BSRR = (uint32_t)SLPTRPIN << 16)     /**< This macro pulls the SLP_TR pin low. */
// TODO check if it is realy connected to the output, if it is configured as input
#define hal_get_slptr()			( (SLPTRPORT->IDR & SLPTRPIN) ? true : false )   /**< Read current state of the SLP_TR pin (High/Low). */

#define hal_set_rst_high()		(RSTPORT->BSRR = RSTPIN)  /**< This macro pulls the RST pin high. */
#define hal_set_rst_low()		(RSTPORT->BSRR = (uint32_t)RSTPIN << 16) /**< This macro pulls the RST pin low. */
#define hal_get_rst()			( (RSTPORT->IDR & RSTPIN) ? true : false )  /**< Read current state of the RST pin (High/Low). */

/** \} */


#define HAL_SS_HIGH()			(SSPORT->BSRR = SSPIN) /**< MACRO for pulling SS high. */
#define HAL_SS_LOW()			(SSPORT->BSRR = (uint32_t)SSPIN << 16); delay_us(1); /**< MACRO for pulling SS low. */
#define hal_get_ss()			( (SSPORT->IDR & SSPIN) ? true : false )

// TODO check where it will be used and whether it works right
/** This macro will protect the following code from interrupts.*/
#define HAL_ENTER_CRITICAL_REGION( ) { NVIC_DisableIRQ(EXTI4_IRQn); taskDISABLE_INTERRUPTS();

/** This macro must always be used in conjunction with HAL_ENTER_CRITICAL_REGION
	so that interrupts are enabled again.*/
#define HAL_LEAVE_CRITICAL_REGION( ) taskENABLE_INTERRUPTS(); NVIC_EnableIRQ(EXTI4_IRQn); }


/*============================ TYPDEFS =======================================*/
/*============================ PROTOTYPES ====================================*/
/*============================ MACROS ========================================*/
/** \name Macros for radio operation.
 * \{ 
 */
#define HAL_BAT_LOW_MASK       ( 0x80 ) /**< Mask for the BAT_LOW interrupt. */
#define HAL_TRX_UR_MASK        ( 0x40 ) /**< Mask for the TRX_UR interrupt. */
#define HAL_TRX_END_MASK       ( 0x08 ) /**< Mask for the TRX_END interrupt. */
#define HAL_RX_START_MASK      ( 0x04 ) /**< Mask for the RX_START interrupt. */
#define HAL_PLL_UNLOCK_MASK    ( 0x02 ) /**< Mask for the PLL_UNLOCK interrupt. */
#define HAL_PLL_LOCK_MASK      ( 0x01 ) /**< Mask for the PLL_LOCK interrupt. */

#define HAL_MIN_FRAME_LENGTH   ( 0x03 ) /**< A frame should be at least 3 bytes. */
#define HAL_MAX_FRAME_LENGTH   ( 0x7F ) /**< A frame should no more than 127 bytes. */
/** \} */
/*============================ TYPDEFS =======================================*/
/** \struct hal_rx_frame_t
 *  \brief  This struct defines the rx data container.
 *
 *  \see hal_frame_read
 */
typedef struct{
    uint8_t length;                       /**< Length of frame. */
    uint8_t data[ HAL_MAX_FRAME_LENGTH ]; /**< Actual frame data. */
    uint8_t lqi;                          /**< LQI value for received frame. */
    bool crc;                             /**< Flag - did CRC pass for received frame? */
} hal_rx_frame_t;


/*============================ PROTOTYPES ====================================*/
void hal_init( void );

uint8_t hal_register_read( uint8_t address );
void hal_register_write( uint8_t address, uint8_t value );
uint8_t hal_subregister_read( uint8_t address, uint8_t mask, uint8_t position );
void hal_subregister_write( uint8_t address, uint8_t mask, uint8_t position,
							uint8_t value );



void hal_frame_read(hal_rx_frame_t *rx_frame);
void hal_frame_write( uint8_t *write_buffer, uint8_t length );
void hal_sram_read( uint8_t address, uint8_t length, uint8_t *data );
void hal_sram_write( uint8_t address, uint8_t length, uint8_t *data );

bool hal_rx_buffer_overflow();

/* Number of receive buffers in RAM. */
#ifndef RF230_CONF_RX_BUFFERS
#define RF230_CONF_RX_BUFFERS 1
#endif

#endif
/** @} */
/*EOF*/

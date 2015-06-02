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

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

#include "FreeRTOS.h"
#include "task.h"

#include "contiki-conf.h"
/*============================ MACROS ========================================*/

/** \name This is the list of pin configurations needed for a given platform.
 * \brief Change these values to port to other platforms.
 * \{
 */
///* 1284 inga */
#   define SSPORT     GPIOB
#   define SSPIN      GPIO_Pin_12
//#   define SPIPORT    GPIOB
//#   define MOSIPIN    (0x05)
//#   define MISOPIN    (0x06)
//#   define SCKPIN     (0x07)
#   define RSTPORT    GPIOD
#   define RSTPIN     GPIO_Pin_8
//#   define IRQPORT    D
//#   define IRQPIN     (0x06)
#   define SLPTRPORT  GPIOD
// TODO check if if is always used as mask and not as number
#   define SLPTRPIN   GPIO_Pin_9

/* For architectures that have all SPI signals on the same port */
#ifndef SSPORT
#define SSPORT SPIPORT
#endif

#ifndef SCKPORT
#define SCKPORT SPIPORT
#endif

#ifndef MOSIPORT
#define MOSIPORT SPIPORT
#endif

#ifndef MISOPORT
#define MISOPORT SPIPORT
#endif

/** \} */



/**
 * \name Pin macros
 * \brief These macros convert the platform-specific pin defines into names and functions
 *       that the source code can directly use.
 * \{
 */

//#define SLP_TR                SLPTRPIN            /**< Pin number that corresponds to the SLP_TR pin. */
//#define DDR_SLP_TR            DDR( SLPTRPORT )    /**< Data Direction Register that corresponds to the port where SLP_TR is connected. */
//#define PORT_SLP_TR           PORT( SLPTRPORT )   /**< Port (Write Access) where SLP_TR is connected. */
//#define PIN_SLP_TR            PIN( SLPTRPORT )    /**< Pin (Read Access) where SLP_TR is connected. */
#define hal_set_slptr_high( ) (SLPTRPORT->BSRRL = SLPTRPIN)      /**< This macro pulls the SLP_TR pin high. */
#define hal_set_slptr_low( )  (SLPTRPORT->BSRRH = SLPTRPIN)     /**< This macro pulls the SLP_TR pin low. */
// TODO check if it is realy connected to the output, if it is configured as input
#define hal_get_slptr( )      ( (SLPTRPORT->IDR & SLPTRPIN) ? true : false )   /**< Read current state of the SLP_TR pin (High/Low). */
//#define RST                   RSTPIN              /**< Pin number that corresponds to the RST pin. */
//#define DDR_RST               DDR( RSTPORT )      /**< Data Direction Register that corresponds to the port where RST is */
//#define PORT_RST              PORT( RSTPORT )     /**< Port (Write Access) where RST is connected. */
//#define PIN_RST               PIN( RSTPORT /* BUG? */)      /**< Pin (Read Access) where RST is connected. */
#define hal_set_rst_high( )   (RSTPORT->BSRRL = RSTPIN)  /**< This macro pulls the RST pin high. */
#define hal_set_rst_low( )    (RSTPORT->BSRRH = RSTPIN) /**< This macro pulls the RST pin low. */
#define hal_get_rst( )        ( (RSTPORT->IDR & RSTPIN) ? true : false )  /**< Read current state of the RST pin (High/Low). */
//#define HAL_SS_PIN            SSPIN               /**< The slave select pin. */
//#define HAL_SCK_PIN           SCKPIN              /**< Data bit for SCK. */
//#define HAL_MOSI_PIN          MOSIPIN
//#define HAL_MISO_PIN          MISOPIN
//#define HAL_PORT_SPI          PORT( SPIPORT )     /**< The SPI module is located on PORTB. */
//#define HAL_PORT_SS            PORT( SSPORT )
//#define HAL_PORT_SCK           PORT( SCKPORT )
//#define HAL_PORT_MOSI          PORT( MOSIPORT )     /**< The SPI module uses GPIO might be split on different ports. */
//#define HAL_PORT_MISO          PORT( MISOPORT )     /**< The SPI module uses GPIO might be split on different ports. */
//#define HAL_DDR_SPI           DDR( SPIPORT )      /**< Data Direction Register for PORTB. */
//#define HAL_DDR_SS             DDR( SSPORT )      /**< Data Direction Register for MISO GPIO pin. */
//#define HAL_DDR_SCK            DDR( SCKPORT )      /**< Data Direction Register for MISO GPIO pin. */
//#define HAL_DDR_MOSI           DDR( MOSIPORT )      /**< Data Direction Register for MISO GPIO pin. */
//#define HAL_DDR_MISO           DDR( MISOPORT )      /**< Data Direction Register for MOSI GPIO pin. */
//#define HAL_DD_SS             SSPIN               /**< Data Direction bit for SS. */
//#define HAL_DD_SCK            SCKPIN              /**< Data Direction bit for SCK. */
//#define HAL_DD_MOSI           MOSIPIN             /**< Data Direction bit for MOSI. */
//#define HAL_DD_MISO           MISOPIN             /**< Data Direction bit for MISO. */

/** \} */


#define HAL_SS_HIGH( ) (SSPORT->BSRRL = SSPIN) /**< MACRO for pulling SS high. */
#define HAL_SS_LOW( )  (SSPORT->BSRRH = SSPIN) /**< MACRO for pulling SS low. */

// TODO
#define RADIO_VECT TIMER1_CAPT_vect
// Raven and Jackdaw
//#define HAL_ENABLE_RADIO_INTERRUPT( ) { TCCR1B = ( 1 << ICES1 ) | ( 1 << CS10 ); TIFR1 |= (1 << ICF1); TIMSK1 |= ( 1 << ICIE1 ) ; }
//#define HAL_DISABLE_RADIO_INTERRUPT( ) ( TIMSK1 &= ~( 1 << ICIE1 ) )

//#define HAL_ENABLE_OVERFLOW_INTERRUPT( ) ( TIMSK1 |= ( 1 << TOIE1 ) )
//#define HAL_DISABLE_OVERFLOW_INTERRUPT( ) ( TIMSK1 &= ~( 1 << TOIE1 ) )


// TODO check where it will be used and whether it works right
/** This macro will protect the following code from interrupts.*/
#define HAL_ENTER_CRITICAL_REGION( ) { taskDISABLE_INTERRUPTS();

/** This macro must always be used in conjunction with HAL_ENTER_CRITICAL_REGION
	so that interrupts are enabled again.*/
#define HAL_LEAVE_CRITICAL_REGION( ) taskENABLE_INTERRUPTS(); }



/** \brief  Enable the interrupt from the radio transceiver.
 */
#define hal_enable_trx_interrupt( ) HAL_ENABLE_RADIO_INTERRUPT( )

/** \brief  Disable the interrupt from the radio transceiver.
 *
 *  \retval 0 if the pin is low, 1 if the pin is high.
 */
#define hal_disable_trx_interrupt( ) HAL_DISABLE_RADIO_INTERRUPT( )
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

/* Hack for atmega128rfa1 with integrated radio. Access registers directly, not through SPI */
#if defined(__AVR_ATmega128RFA1__)
//#define hal_register_read(address) _SFR_MEM8((uint16_t)address)
#define hal_register_read(address) address
uint8_t hal_subregister_read( uint16_t address, uint8_t mask, uint8_t position );
void hal_subregister_write( uint16_t address, uint8_t mask, uint8_t position,
                            uint8_t value );

//#define hal_register_write(address, value) _SFR_MEM8((uint16_t)address)=value
#define hal_register_write(address, value) address=value
//#define hal_subregister_read( address, mask, position ) (_SFR_MEM8((uint16_t)address)&mask)>>position
//#define hal_subregister_read1( address, mask, position ) (address&mask)>>position
//#define hal_subregister_write( address, mask, position, value ) address=(address<<position)&mask
#else
uint8_t hal_register_read( uint8_t address );
void hal_register_write( uint8_t address, uint8_t value );
uint8_t hal_subregister_read( uint8_t address, uint8_t mask, uint8_t position );
void hal_subregister_write( uint8_t address, uint8_t mask, uint8_t position,
                            uint8_t value );
#endif



void hal_frame_read(hal_rx_frame_t *rx_frame);
void hal_frame_write( uint8_t *write_buffer, uint8_t length );
void hal_sram_read( uint8_t address, uint8_t length, uint8_t *data );
void hal_sram_write( uint8_t address, uint8_t length, uint8_t *data );
/* Number of receive buffers in RAM. */
#ifndef RF230_CONF_RX_BUFFERS
#define RF230_CONF_RX_BUFFERS 1
#endif

#endif
/** @} */
/*EOF*/

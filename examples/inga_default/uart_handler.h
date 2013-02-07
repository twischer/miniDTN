/* 
 * File:   uart_handler.h
 * Author: enrico
 *
 * Created on 6. Februar 2013, 22:35
 */

#ifndef UART_HANDLER_H
#define	UART_HANDLER_H

/**
 * @addtogroup default_app
 * @{
 */

/**
 * @defgroup uart_handler Handler for UART data
 * 
 * Accepted commands:
 * - \b 0x11 - indicates sending of new configuration file.
 *    Sending file must be terminated with EOT character (0x04).
 * 
 * @{
 */

#include "contiki.h"

extern process_event_t event_uart;

/**
 * Init uart_handler
 * @return 
 */
void uart_handler_init();

/** @} */
/** @} */

#endif	/* UART_HANDLER_H */


/* 
 * File:   uart_handler.h
 * Author: enrico
 *
 * Created on 6. Februar 2013, 22:35
 */

#ifndef UART_HANDLER_H
#define	UART_HANDLER_H

#include "contiki.h"

extern process_event_t event_uart;

/**
 * Init uart_handler
 * @return 
 */
void uart_handler_init();

#endif	/* UART_HANDLER_H */


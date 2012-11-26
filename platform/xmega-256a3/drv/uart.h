#ifndef __UART_H__
#define __UART_H__

#include "avr/io.h"
#include <string.h>
#include <stdio.h>

int uart_putchar(char c, FILE *stream);
void uart_init(void);

#endif

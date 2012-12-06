#include "drv/uart.h"

static FILE mystdout = FDEV_SETUP_STREAM (uart_putchar, NULL, _FDEV_SETUP_WRITE);

void uart_init(void)
{
    // Set the TxD pin high - set PORTC DIR register bit 3 to 1
    PORTC.OUTSET = PIN3_bm;
 
    // Set the TxD pin as an output - set PORTC OUT register bit 3 to 1
    PORTC.DIRSET = PIN3_bm;
 
    // Set baud rate & frame format
/*
	2MHz: 0 / 12
	32 MHz: 0x1011 / 3301
*/
    USARTC0.BAUDCTRLB = 0;			// BSCALE = 0
    USARTC0.BAUDCTRLA = 209;
 
    // Set mode of operation
    USARTC0.CTRLA = 0;				// no interrupts please
    USARTC0.CTRLC = 0x03;			// async, no parity, 8 bit data, 1 stop bit
 
    // Enable transmitter only
	USARTC0.CTRLB = USART_TXEN_bm;

	stdout = &mystdout;
}

int uart_putchar (char c, FILE *stream)
{
    if (c == '\n')
        uart_putchar('\r', stream);
 
    // Wait for the transmit buffer to be empty
    while ( !( USARTC0.STATUS & USART_DREIF_bm) );
 
    // Put our character into the transmit buffer
    USARTC0.DATA = c;
 
    return 0;
}

#include "uart_handler.h"

#include <stdint.h>
#include "rs232.h"
#include "app_config.h"

#define ST_AWAIT_CMD  0
#define ST_AWAIT_SIZE_H 1
#define ST_AWAIT_SIZE_L 1
#define ST_AWAIT_DATA 2

#define CMD_CONFIG 0x11

#define ASCII_EOT 0x04

process_event_t event_uart;

static volatile uint8_t state = ST_AWAIT_CMD;
static volatile uint16_t size;
static volatile uint16_t size_cnt;
//unsigned char buf[1024];
unsigned char *buf_ptr;
static int uart_handler(unsigned char ch);
/*----------------------------------------------------------------------------*/
void
uart_handler_init()
{
  buf_ptr = app_config_buffer;
  event_uart = process_alloc_event();
  rs232_set_input(0, uart_handler);
}
/*----------------------------------------------------------------------------*/
static int
uart_handler(unsigned char ch)
{

  switch (state) {
    case ST_AWAIT_CMD:
      state = ch == CMD_CONFIG ? ST_AWAIT_DATA : ST_AWAIT_CMD;
      break;

    case ST_AWAIT_DATA:
      if (ch == ASCII_EOT) {
        *buf_ptr = '\0';

        process_post(&config_process, event_config_update, 0);

        buf_ptr = app_config_buffer;
        state = ST_AWAIT_CMD;
      } else {
        *(buf_ptr++) = ch;
      }

      break;
  }
}
/*----------------------------------------------------------------------------*/

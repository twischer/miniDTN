#include "uart_handler.h"

#include <stdint.h>
#include "rs232.h"
#include "app_config.h"
#include "config_mapping.h"
#include "logger.h"

#define ST_AWAIT_CMD  0
#define ST_AWAIT_DATA 1

#define CMD_CONFIG 0x11

#define ASCII_EOT 0x04

process_event_t event_uart;

static volatile uint8_t state = ST_AWAIT_CMD;

char *buf_ptr;

static int uart_handler(unsigned char ch);
/*----------------------------------------------------------------------------*/
void
uart_handler_init()
{
  buf_ptr = &app_config_buffer[0];
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
        log_i("UART: Received new configuration\n");
        *buf_ptr = '\0';

        parse_ini(app_config_buffer, &inga_conf_file);

        app_config_update();

        buf_ptr = app_config_buffer;
        state = ST_AWAIT_CMD;
      } else {
        *(buf_ptr++) = ch;
      }
      break;
  }

  return 0;
}
/*----------------------------------------------------------------------------*/

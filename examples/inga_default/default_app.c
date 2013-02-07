/**
 * default app.c
 */
#include "contiki.h"

#include <util/delay.h>
#include <stdlib.h>

#include <avr/io.h>

#define DEBUG                                   1

// standard includes
#include <stdio.h> /* For printf() */
#include <stdint.h>
// app includes
#include "ini_parser.h"
#include "app_config.h"
#include "logger.h"
#include "sd_mount.h"
#include "uart_handler.h"
#include "sensor_fetch.h"

/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Sensor update process");
AUTOSTART_PROCESSES(&default_app_process, &logger_process, &config_process, &mount_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(default_app_process, ev, data)
{
  PROCESS_BEGIN();

  uart_handler_init();

  // we wait until mounted, before loading config
  PROCESS_WAIT_EVENT_UNTIL(ev == event_mount);

  if (app_config_init(*(bool*) data) != 0) {
    log_e("Loading config failed!\n");
  }

  // start sensor process
  process_start(&sensor_update, NULL);

  printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

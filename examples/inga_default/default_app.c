/**
 * default app.c
 */
#include "contiki.h"

#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

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
#include "default_app.h"


// system status flags
uint16_t system_status = {0};
/* Sets flag either */
/*---------------------------------------------------------------------------*/
inline void
sysflag_set_en(uint8_t flag)
{
  system_status |= (1 << flag);
}
/*---------------------------------------------------------------------------*/
inline void
sysflag_set_dis(uint8_t flag)
{
  system_status &= ~(1 << flag);
}
/*---------------------------------------------------------------------------*/
inline uint8_t
sysflag_get(uint8_t flag)
{
  return (system_status & (1 << flag)) ? 1 : 0;
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Sensor update process");
AUTOSTART_PROCESSES(
        &default_app_process,
        &logger_process,
        //        &config_process,
        &mount_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(default_app_process, ev, data)
{
  PROCESS_BEGIN();

  uart_handler_init();
  clock_init();

  // we wait until mounted, before loading config
  PROCESS_WAIT_EVENT_UNTIL(ev == event_mount);

  if (app_config_init() != 0) {
    log_e("Loading saved config failed!\n");
  }

  // start sensor process
  process_start(&sensor_update, NULL);

  //    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

/**
 * default app.c
 */
#include "contiki.h"
//#include "dev/adxl345.h"        //tested
//#include "dev/microSD.h"        //tested
//#include "dev/at45db.h"         //tested
//#include "dev/bmp085.h"         //tested
//#include "dev/l3g4200d.h"       //tested
//#include "dev/mpl115a.h"        //tested (not used)
//#include "dev/adc.h"
#include "cfs.h"
#include "cfs-fat.h"

#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
extern uint8_t __bss_end;

#define PWR_MONITOR_ICC_ADC             3
#define PWR_MONITOR_VCC_ADC             2

#define DEBUG                                   1

#include <stdio.h> /* For printf() */

#include "acc-sensor.h"
#include "adc-sensor.h"
#include "button-sensor.h"
#include "gyro-sensor.h"
#include "pressure-sensor.h"
#include "radio-sensor.h"

#include "config_mapping.h"
#include "ini_parser.h"

int8_t load_configuration();
/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Sensor update process");
AUTOSTART_PROCESSES(&default_app_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(default_app_process, ev, data) {
  PROCESS_BEGIN();

  load_configuration();
  
  SENSORS_ACTIVATE(acc_sensor);
  SENSORS_ACTIVATE(gyro_sensor);
  SENSORS_ACTIVATE(pressure_sensor);

  etimer_set(&timer, CLOCK_SECOND * 0.05);
//  while (1) {
//
//    PROCESS_YIELD();
//    etimer_set(&timer, CLOCK_SECOND * 0.05);
//    printf("X_ACC=%d, Y_ACC=%d, Z_ACC=%d\n",
//            acc_sensor.value(ACC_X),
//            acc_sensor.value(ACC_Y),
//            acc_sensor.value(ACC_Z));
//    printf("X_AS=%d, Y_AS=%d, Z_AS=%d\n",
//            gyro_sensor.value(X_AS),
//            gyro_sensor.value(Y_AS),
//            gyro_sensor.value(Z_AS));
//    printf("PRESS=%u, TEMP=%d\n\n",
//            pressure_sensor.value(PRESS),
//            pressure_sensor.value(TEMP));
//
//  }

  PROCESS_END();
}
#define MAX_FILE_SIZE 512
/*---------------------------------------------------------------------------*/
int8_t
load_configuration() {
  struct diskio_device_info *info = 0;
  int fd;
  int i;
  int initialized = 0;
  printf("Detecting devices and partitions...");
  while ((i = diskio_detect_devices()) != DISKIO_SUCCESS) {
    watchdog_periodic();
  }
  printf("done\n\n");

  info = diskio_devices();
  for (i = 0; i < DISKIO_MAX_DEVICES; i++) {
    print_device_info(info + i);
    printf("\n");

    // use first detected partition
    if ((info + i)->type == (DISKIO_DEVICE_TYPE_SD_CARD | DISKIO_DEVICE_TYPE_PARTITION)) {
      info += i;
      initialized = 1;
      break;
    }
  }

  if (!initialized) {
    printf("No SD card was found\n");
    return -1;
  }

  printf("Mounting device...");
  if (cfs_fat_mount_device(info) == 0) {
    printf("done\n\n");
  } else {
    printf("failed\n\n");
  }

  diskio_set_default_device(info);
  
  // And open it
  fd = cfs_open("inga.cfg", CFS_READ);

  // In case something goes wrong, we cannot save this file
  if (fd == -1) {
    printf("############# STORAGE: open for read failed\n");
    return -1;
  }

  char buf[MAX_FILE_SIZE];
  int size = cfs_read(fd, buf, 511);

  printf("actually read: %d\n", size);

  parse_ini(buf, size, &inga_conf_file);

  cfs_close(fd);


  printf("\n\nOutput: \n");
  printf("sdcard: %d\n", system_config.output.sdcard);
  printf("usb: %d\n", system_config.output.usb);
  printf("radio: %d\n", system_config.output.radio);
  printf("block_size: %d\n", system_config.output.block_size);



}
/*---------------------------------------------------------------------------*/

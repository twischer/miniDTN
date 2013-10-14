
/*
 * inga_demo.c
 *
 *  Created on: 13.10.2009
 *      Author: Kulau

 * 
 *
 */

#include "contiki.h"
#include "dev/adxl345.h"             //tested
#include "dev/microSD.h"           //tested
#include "dev/at45db.h"            //tested
#include "dev/bmp085.h"         //tested
#include "dev/l3g4200d.h"           //tested
#include "dev/mpl115a.h"        //tested (not used)
#include "dev/adc.h"

#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#include <avr/io.h>
extern uint8_t __bss_end;

#define PWR_MONITOR_ICC_ADC             3
#define PWR_MONITOR_VCC_ADC             2

#define DEBUG                                   1

#include <stdio.h> /* For printf() */
void
recursive(int val)
{
  int a = 5, b = 6, c = 7;
  // 	printf("%d%d%d\n", a, b, c);
  printf("SP: %d\n", (uint16_t) SP);
  if (val == 400) return;
  recursive(++val);
}

/*---------------------------------------------------------------------------*/
PROCESS(default_app_process, "Hello world process");
AUTOSTART_PROCESSES(&default_app_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(default_app_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Hello, world\n");

  _delay_ms(100);

#if DEBUG
  printf("Begin initialization:\n");

  if (microSD_init() == 0) {
    printf(":microSD   OK\n");
    microSD_switchoff();
  } else {
    printf(":microSD   FAILURE\n");
  }
  if (adxl345_init() == 0) {
    printf(":ADXL345   OK\n");
  } else {
    printf(":ADXL345   FAILURE\n");
  }
  if (at45db_init() == 0) {
    printf(":AT45DBxx  OK\n");
  } else {
    printf(":AT45DBxx  FAILURE\n");
  }
//  if (bmp085_init() == 0) {
//    printf(":BMP085    OK\n");
//  } else {
//    printf(":BMP085    FAILURE\n");
//  }
//  if (l3g4200d_init() == 0) {
//    printf(":L3G4200D  OK\n");
//  } else {
//    printf(":L3G4200D  FAILURE\n");
//  }
#else
  adxl345_init();
  microSD_switchoff();
  at45db_init();
  microSD_init();
  bmp085_init();
  l3g4200d_init();
#endif

  adc_init(ADC_SINGLE_CONVERSION, ADC_REF_2560MV_INT);
  etimer_set(&timer, CLOCK_SECOND * 0.05);

  while (1) {

    PROCESS_YIELD();
    etimer_set(&timer, CLOCK_SECOND);
    int16_t tmp;
    /*############################################################*/
    //ADXL345 serial Test
    printf("X:%+6d | Y:%+6d | Z:%+6d\n",
            adxl345_get_x_acceleration(),
            adxl345_get_y_acceleration(),
            adxl345_get_z_acceleration());

    /*############################################################*/
    //L3G4200d serial Test
    printf("X:%+6d | Y:%+6d | Z:%+6d\n",
            l3g4200d_get_x_angle(),
            l3g4200d_get_y_angle(),
            l3g4200d_get_z_angle());
    printf("T1:%+3d\n", l3g4200d_get_temp());

    /*############################################################*/
    //BMP085 serial Test
    printf("P:%+6ld\n", (uint32_t) bmp085_read_pressure(BMP085_HIGH_RESOLUTION));
    printf("T2:%+3d\n", bmp085_read_temperature());

    /*############################################################*/
    //Power Monitoring
    printf("V:%d\n", adc_get_value_from(PWR_MONITOR_ICC_ADC));
    //              tmp = adc_get_value_from(PWR_MONITOR_ICC_ADC);
    //             
    //              tmp = adc_get_value_from(PWR_MONITOR_VCC_ADC);
    //             
    /*############################################################*/
    //microSD Test
    //              uint8_t buffer[512];
    //              uint16_t j;
    //              microSD_init();
    //              for (j = 0; j < 512; j++) {
    //                      buffer[j] = 'u';
    //                      buffer[j + 1] = 'e';
    //                      buffer[j + 2] = 'r';
    //                      buffer[j + 3] = '\n';
    //
    //                      j = j + 3;
    //              }
    //
    //              microSD_write_block(2, buffer);
    //
    //              microSD_read_block(2, buffer);
    //
    //              for (j = 0; j < 512; j++) {
    //                      printf(buffer[j]);
    //              }
    //              microSD_deinit();
    //
    /*############################################################*/
    //Flash Test
    //    uint8_t buffer[512];
    //    uint16_t j, i;
    //
    //    for (i = 0; i < 10; i++) {
    //      //at45db_erase_page(i);
    //      for (j = 0; j < 512; j++) {
    //        buffer[j] = i;
    //      }
    //      at45db_write_buffer(0, buffer, 512);
    //
    //      at45db_buffer_to_page(i);
    //
    //      at45db_read_page_bypassed(i, 0, buffer, 512);
    //
    //      for (j = 0; j < 512; j++) {
    //        printf("%02x", buffer[j]);
    //        if (((j + 1) % 2) == 0)
    //          printf(" ");
    //        if (((j + 1) % 32) == 0)
    //          printf("\n");
    //      }
    //    }

  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

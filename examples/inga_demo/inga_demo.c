
/*
 * inga_demo.c
 *
 *  Created on: 13.10.2009
 *      Author: Kulau

 * 
 *
 */
	
#include "contiki.h"
#include "interfaces/acc-adxl345.h"             //tested
#include "interfaces/flash-microSD.h"           //tested
#include "interfaces/flash-at45db.h"            //tested
#include "interfaces/pressure-bmp085.h"         //tested
#include "interfaces/gyro-l3g4200d.h"           //tested
#include "interfaces/pressure-mpl115a.h"        //tested (not used)
#include "drv/adc-drv.h"

#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

#define PWR_MONITOR_ICC_ADC             3
#define PWR_MONITOR_VCC_ADC             2

#define DEBUG                                   0

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(hello_world_process, "Hello world process");
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
static struct etimer timer;
PROCESS_THREAD(hello_world_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Hello, world\n");


#if DEBUG
        printf("Begin initialization:");

        if (adxl345_init() == 0) {
                printf("\n:ADXL345   OK");
        } else {
                printf("\n:ADXL345   FAILURE");
        }
        if (microSD_init() == 0) {
                printf("\n:microSD   OK");
                microSD_deinit();
        } else {
                printf("\n:microSD   FAILURE");
        }
        if (at45db_init() == 0) {
                printf("\n:AT45DBxx  OK");
        } else {
                printf("\n:AT45DBxx  FAILURE");
        }
        if (bmp085_init() == 0) {
                printf("\n:BMP085    OK");
        } else {
                printf("\n:BMP085    FAILURE");
        }
        if (l3g4200d_init() == 0) {
                printf("\n:L3G4200D  OK");
        } else {
                printf("\n:L3G4200D  FAILURE");
        }
#else
        adxl345_init();
        microSD_deinit();
        at45db_init();
        microSD_init();
        bmp085_init();
        l3g4200d_init();
#endif

        adc_init(ADC_SINGLE_CONVERSION, ADC_REF_2560MV_INT);
	etimer_set(&timer,  CLOCK_SECOND*0.05);

        while (1) {
		PROCESS_YIELD();
		etimer_set(&timer,  CLOCK_SECOND);
                int16_t tmp;
                /*############################################################*/
                //ADXL345 serial Test
                printf("\n X:%d", adxl345_get_x_acceleration());
                printf("| Y:%d", adxl345_get_y_acceleration());
                printf("| Z:%d", adxl345_get_z_acceleration());
                //
                //              tmp = adxl345_get_y_acceleration();
                //
                //              tmp = adxl345_get_z_acceleration();

                /*############################################################*/
                //L3G4200d serial Test
                printf("\n X: %d", l3g4200d_get_x_angle());
                printf("| Y: %d", l3g4200d_get_y_angle());
                printf("| Z: %d", l3g4200d_get_z_angle());
                //              tmp = l3g4200d_get_x_angle();
                //
                //              tmp = l3g4200d_get_y_angle();
                //
                //              tmp = l3g4200d_get_z_angle();
                //
                //              tmp = l3g4200d_get_temp();

                /*############################################################*/
                //BMP085 serial Test
                              #define BMP085_MODE_0   0
                              #define BMP085_MODE_1   1
                              #define BMP085_MODE_2   2
                              #define BMP085_MODE_3   3
                //
                              tmp = (uint16_t) bmp085_read_temperature();
                //
                              tmp = (int16_t) bmp085_read_comp_pressure(BMP085_MODE_2);

                /*############################################################*/
                //Power Monitoring
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
                //              uint8_t buffer[512];
                //              uint16_t j, i;
                //
                //              for (i = 0; i < 10; i++) {
                //                      //at45db_erase_page(i);
                //                      for (j = 0; j < 512; j++) {
                //                              buffer[j] = i;
                //                      }
                //                      at45db_write_buffer(0, buffer, 512);
                //
                //                      at45db_buffer_to_page(i);
                //
                //                      at45db_read_page_bypassed(i, 0, buffer, 512);
                //              }

        }

  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

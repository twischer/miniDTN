
/*
 * sensor_demo.c
 *
 *  Created on: 17.10.2011
 *      Author: Georg von Zengen 
 *
 */
	
#include "contiki.h"
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include "acc-sensor.h"
#include "gyro-sensor.h"
#include "pressure-sensor.h"


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
	
	SENSORS_ACTIVATE(acc_sensor);
	SENSORS_ACTIVATE(gyro_sensor);
	SENSORS_ACTIVATE(pressure_sensor);

	etimer_set(&timer,  CLOCK_SECOND*0.05);
        while (1) {
		
		PROCESS_YIELD();
		etimer_set(&timer,  CLOCK_SECOND);
		printf("X_ACC=%d, ",acc_sensor.value(X_ACC));
		printf("Y_ACC=%d, ",acc_sensor.value(Y_ACC));
		printf("Z_ACC=%d\n",acc_sensor.value(Z_ACC));
		printf("X_AS=%d, Y_AS=%d, Z_AS=%d\n",gyro_sensor.value(X_AS), gyro_sensor.value(Y_AS), gyro_sensor.value(Z_AS));
		printf("PRESS=%d, TEMP=%d\n\n", pressure_sensor.value(PRESS), pressure_sensor.value(TEMP));

        }

  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

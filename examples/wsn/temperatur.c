
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

#include "net/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"

#define DEBUG                                   0

#include <stdio.h> /* For printf() */

PROCESS(hello_world_process, "Hello world process");
PROCESS(button_watchdog, "Watches the button");

AUTOSTART_PROCESSES(&hello_world_process, &button_watchdog);
/*---------------------------------------------------------------------------*/
static uint16_t pressure = 0;
static int16_t temperature = 0;

static int16_t temper_data[1000];
static uint16_t data_i = 0;

/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	leds_init();	

	leds_toggle(LEDS_YELLOW);
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

PROCESS_THREAD(button_watchdog, ev, data)
{
	static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

	PROCESS_BEGIN();
	SENSORS_ACTIVATE(button_sensor);//activate button

  broadcast_open(&broadcast, 129, &broadcast_call);

	while (1) {
		PROCESS_YIELD_UNTIL(ev == sensors_event && data == &button_sensor);

		pressure = pressure_sensor.value(PRESS);
		temperature = pressure_sensor.value(TEMP);
		leds_toggle(LEDS_YELLOW);
		printf("saved Press: %u saved temp: %d\n", pressure, temperature);
		//,(uint8_t) gyro_sensor.value(TEMP));
		printf("the last %u Temperature values:\n\n",(uint16_t) data_i);

		static uint16_t i2;
		static uint8_t buffer[3];
		for (i2 = 0; i2 < data_i; i2++) {
			printf("send bc msg [%d]\n", (int16_t) temper_data[i2]);
			sprintf( buffer, "%d", (int16_t) temper_data[i2] );
			packetbuf_copyfrom(buffer, 3);
			broadcast_send(&broadcast);
			//printf("broadcast message sent\n");
		}

		etimer_set(&et,  CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
		//PROCESS_YIELD_UNTIL( etimer_expired(&et) );

		leds_toggle(LEDS_YELLOW);
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_world_process, ev, data)
{

  PROCESS_BEGIN();
	leds_init();	
	
	SENSORS_ACTIVATE(gyro_sensor);
	SENSORS_ACTIVATE(pressure_sensor);

	static uint8_t i = 0;
	static double av_press = 0.0;
	static struct etimer et_print;
	static struct etimer et_meas;
	static struct etimer et_save;

	pressure = pressure_sensor.value(PRESS);
	temperature = pressure_sensor.value(TEMP);

	etimer_set(&et_meas,  CLOCK_SECOND*0.05);
	etimer_set(&et_print,  CLOCK_SECOND);
	etimer_set(&et_save,  CLOCK_SECOND);

  while (1) {
		
		PROCESS_YIELD_UNTIL(etimer_expired(&et_meas) || etimer_expired(&et_save) || etimer_expired(&et_print));

		if (etimer_expired(&et_meas)) {
			/* Measure */
			i++;
			av_press += (pressure_sensor.value(PRESS) - av_press) * (1.0 / i);
			//etimer_set(&et_meas,  CLOCK_SECOND*0.05);
			printf("av %u i %u |", (uint16_t)av_press, (uint16_t) i );

		} else if (etimer_expired(&et_print)) {
			/* text output */
			printf("\nPRESS: abs=%u, rel=%d, TEMP_P: abs=%d, rel=%d, TEMP_G=%u\n",
					(uint16_t)(av_press ),
					(int16_t)(av_press - pressure),
					(int16_t)pressure_sensor.value(TEMP),
					(int16_t)(pressure_sensor.value(TEMP) - temperature),
					(uint8_t) gyro_sensor.value(TEMP_AS));

			etimer_set(&et_print,  CLOCK_SECOND);
			i = av_press = 0;

		} else if (etimer_expired(&et_save)) {
			/* saves the current value in ram */
			printf("debug\n");
			temper_data[(uint16_t)data_i] = (int16_t)pressure_sensor.value(TEMP);

			data_i++;
			etimer_set(&et_save,  CLOCK_SECOND*6);

		} else {
			printf("error\n");
		}


  }

  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

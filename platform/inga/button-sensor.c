/*Button sensor routine 
* Author: Georg von Zengen 
*/

#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include <avr/io.h>
#include <avr/interrupt.h>
const struct sensors_sensor button_sensor;
static int status(int type);
static struct timer debouncetimer;
ISR(INT2_vect){

	if(timer_expired(&debouncetimer)) {
		timer_set(&debouncetimer, CLOCK_SECOND / 40);
		sensors_changed(&button_sensor);
	}

}

static int
value(int type)
{
	return 1-((PINB & (1<<PB2))>>PB2) || !timer_expired(&debouncetimer);
}

static int
configure(int type, int c)
{
	switch (type) {
	case SENSORS_ACTIVE:
		if (c) {
			if(!status(SENSORS_ACTIVE)) {
				timer_set(&debouncetimer, CLOCK_SECOND / 40);
				DDRB &= ~(1<<PB2);
				PORTB &= ~(1<<PB2);
				sei();
				EICRA |= (1<<ISC20);
				EIMSK |= (1<<INT2);
			}
		} else {
			DDRB |= (1<<PB2);
			PORTB |= (1<<PB2);
			EIMSK &= ~(1<<INT2);
		}
		return 1;
	}
	return 0;
}

static int
status(int type)
{
	switch (type) {
	case SENSORS_ACTIVE:
		return 0;
	case SENSORS_READY:
		return ~(DDRB & (1<<PB2));
	}

	return 0;
}

SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
	       value, configure, status);


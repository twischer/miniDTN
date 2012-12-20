/*Button sensor routine 
 * @author: Georg von Zengen 
 * @todo: License here?
 */

#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_INT_vect	INT2_vect
#define BUTTON_INT			INT2
#define BUTTON_PORT     PORTB
#define BUTTON_DDR			DDRB
#define BUTION_PIN			PINB
#define BUTTON_P				PB2

const struct sensors_sensor button_sensor;
static int status(int type);
static struct timer debouncetimer;

ISR(BUTTON_INT_vect){

	if(timer_expired(&debouncetimer)) {
		timer_set(&debouncetimer, CLOCK_SECOND / 40);
		sensors_changed(&button_sensor);
	}

}
/*----------------------------------------------------------------------------*/
static int
value(int type)
{
	return 1-((BUTION_PIN & (1<<BUTTON_P))>>BUTTON_P) || !timer_expired(&debouncetimer);
}
/*----------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
	switch (type) {
	case SENSORS_ACTIVE:
		if (c) {
			if(!status(SENSORS_ACTIVE)) {
				timer_set(&debouncetimer, CLOCK_SECOND / 40);
				BUTTON_DDR &= ~(1<<BUTTON_P);
				BUTTON_PORT &= ~(1<<BUTTON_P);
				sei();
				EICRA |= (1<<ISC20);
				EIMSK |= (1<<BUTTON_INT);
			}
		} else {
			BUTTON_DDR |= (1<<BUTTON_P);
			BUTTON_PORT |= (1<<BUTTON_P);
			EIMSK &= ~(1<<BUTTON_INT);
		}
		return 1;
	}
	return 0;
}
/*----------------------------------------------------------------------------*/
static int
status(int type)
{
	switch (type) {
	case SENSORS_ACTIVE:
		return 0;
	case SENSORS_READY:
		return ~(BUTTON_DDR & (1<<BUTTON_P));
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
	       value, configure, status);


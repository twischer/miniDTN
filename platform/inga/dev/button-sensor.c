/*Button sensor routine 
 * @author: Georg von Zengen 
 *          Enrico Joerns
 * @todo: License here?
 */

#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_INT_vect   INT2_vect
#define BUTTON_ISC        ISC20
#define BUTTON_INT        INT2
#define BUTTON_PORT       PORTB
#define BUTTON_DDR        DDRB
#define BUTION_PIN        PINB
#define BUTTON_P          PB2
#define BUTTON_REG_EICR   EICRA
#define BUTTON_REG_EIMSK  EIMSK

#ifndef DEBOUNCE_TIME
#define DEBOUNCE_TIME     CLOCK_SECOND / 40
#endif

const struct sensors_sensor button_sensor;
static int status(int type);
static struct timer debouncetimer;
/*----------------------------------------------------------------------------*/
ISR(BUTTON_INT_vect) {
  if (timer_expired(&debouncetimer)) {
    timer_reset(&debouncetimer);
    sensors_changed(&button_sensor);
  }
}
static void
init() {
  BUTTON_DDR &= ~(1 << BUTTON_P);
  BUTTON_PORT &= ~(1 << BUTTON_P);
  BUTTON_REG_EICR |= (0x1 << BUTTON_ISC); // 1 = any edge
}
/*----------------------------------------------------------------------------*/
static int
value(int type) {
  return (1 - ((BUTION_PIN & (1 << BUTTON_P)) >> BUTTON_P))
          || !timer_expired(&debouncetimer);
}
/*----------------------------------------------------------------------------*/
static int
configure(int type, int c) {
  switch (type) {
    case SENSORS_HW_INIT:
      init();
      return 1;
    case SENSORS_ACTIVE:
      // activate sensor
      if (c) {
        if (!status(SENSORS_ACTIVE)) {
          timer_set(&debouncetimer, DEBOUNCE_TIME);
          init();
          BUTTON_REG_EIMSK |= (1 << BUTTON_INT);
          sei();
        }
        // deactivate sensor
      } else {
        BUTTON_REG_EIMSK &= ~(1 << BUTTON_INT);
      }
      return 1;
  }
  return 0;
}
/*----------------------------------------------------------------------------*/
static int
status(int type) {
  switch (type) {
    case SENSORS_ACTIVE:
      return (BUTTON_REG_EIMSK & (1 << BUTTON_INT));
    case SENSORS_READY:
      return ~(BUTTON_DDR & (1 << BUTTON_P));
  }

  return 0;
}
/*----------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
        value, configure, status);


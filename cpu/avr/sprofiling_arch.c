#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include <avr/interrupt.h>

#include "sys/sprofiling.h"


/* For the INGA platform */
#if defined (__AVR_ATmega1284P__) && AVR_CONF_USE32KCRYSTAL
ISR(TIMER2_COMPB_vect) __attribute__ ((no_instrument_function));

ISR(TIMER2_COMPB_vect)
{
	uint8_t i;
	void *pc;
	uint8_t tccr;

	/* Move the interrupt around to avoid aliasing effects*/
	OCR2B = (OCR2B + 29)%(32768/8/CLOCK_CONF_SECOND);
	tccr = TCCR2B;

	TCCR2B &= ~(_BV(CS20)|_BV(CS21)|_BV(CS22));
	for (i=0;i<OCR2B%7;i++)
		asm volatile("nop");
	TCCR2B = tccr;

	pc = __builtin_return_address(0);

	/* The AVR stores the word offset - not the address itself
	 * in the PC - beware when editing */
	sprofiling_add_sample(pc);
}

inline void sprofiling_arch_start(void)
{
	/* Clear any pending interrupt and activate the interrupt again */
	TIFR2 |= _BV(OCF2B);
	TIMSK2 |= _BV(OCIE2B);
}

inline void sprofiling_arch_stop(void)
{
	TIMSK2 &= ~_BV(OCIE2B);
}

void sprofiling_arch_init(void)
{
	/* Call the interrupt at the same time the time is updated */
	OCR2B = 32768/8/CLOCK_CONF_SECOND - 1;
}

#else
#error "Please define your own arch specific profiling functions."
#endif

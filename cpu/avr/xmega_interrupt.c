#include "xmega_interrupt.h"

void xmega_interrupt_enable(uint8_t bm)
{
	PMIC.CTRL |= bm;
}

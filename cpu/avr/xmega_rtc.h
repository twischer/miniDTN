#ifndef __XMEGA_RTC_H__
#define __XMEGA_RTC_H__

#include <avr/io.h>
#include <stdint.h>
#include <avrdef.h>

void xmega_rtc_rcosc_enable(void);
void xmega_rtc_wait_sync_busy(void);
void xmega_rtc_wait_sync_cnt(void);
void xmega_rtc_set_cnt(uint16_t cnt);
void xmega_rtc_set_interrupt_levels(uint8_t irq_ovf, uint8_t irq_comp);
void xmega_rtc_enable(void);
void xmega_rtc_set_prescaler(uint8_t prescaler);

#ifndef CLK_RTCSRC_RCOSC32_gc
	#define CLK_RTCSRC_RCOSC32_gc (0x3 << 2)
#endif
	
#endif /* __XMEGA_RTC_H__ */

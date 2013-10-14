#include "xmega_rtc.h"

void xmega_rtc_rcosc_enable(void)
{
#ifdef RTC32
	#warning RTC32 not supported
#else
	// Enable internal osciallator
	OSC.CTRL |= OSC_RC32KEN_bm;

	// Wait for internal RC oscillator to be ready
	while((OSC.STATUS & OSC_RC32KRDY_bm) == 0);
	
	// Use 32.KHz from internal oscillator
	CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc | CLK_RTCEN_bm;
#endif
}

/**
 * In devices with RTC32 the busy flag can be used for CNT and CTRL
 */

void xmega_rtc_wait_sync_busy(void)
{
#ifdef RTC32
	while (RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm);
#else
	while((RTC.STATUS & RTC_SYNCBUSY_bm));
#endif
}

/**
 * In devices with RTC32 the SYNCCNT Flag will be cleared after sync
 */

void xmega_rtc_wait_sync_cnt(void)
{
#ifdef RTC32
	while (RTC32.SYNCCTRL & RTC32_SYNCCNT_bm);
#else
	xmega_rtc_wait_sync_busy();
#endif
}

void xmega_rtc_set_cnt(uint16_t cnt)
{
#ifdef RTC32
	RTC32.CNT = cnt;
	// Start Syncronisation from CNT Register
	RTC32.SYNCCTRL |= RTC32_SYNCCNT_bm;
	// Wait for sync flag to be cleared
	xmega_rtc_wait_sync_cnt();
#else
	RTC.CNT = 0;
#endif
}

// Set compare value
	// If the compare register is 0 only every third interrupt will generate an event
	// If the compare register is 1 only every second interrupt will generate an event
	// if the compare register is greater than or equal two - every interrupt will generate an event
// .PER Register is not sychronized automatically, so we have to change COMP or CNT afterwards
	 // 32kHZ / 32 = 1kHz

void xmega_rtc_set_per(uint16_t per)
{
#ifdef RTC32
	RTC32.PER = per;
#else
	RTC.PER = per;
#endif
}

void xmega_rtc_set_comp(uint16_t cnt)
{
#ifdef RTC32
	RTC32.COMP = cnt;
#else
	RTC.COMP = 0;
#endif
}

void xmega_rtc_set_interrupt_levels(uint8_t irq_ovf, uint8_t irq_comp)
{
#ifdef RTC32
	// Clear Interrupts first and then add them
	RTC32.INTCTRL = (RTC32.INTCTRL & ~(RTC32_COMPINTLVL_gm | RTC32_OVFINTLVL_gm)) | irq_ovf | irq_comp;
#else
	RTC.INTCTRL = (RTC.INTCTRL & ~(RTC_COMPINTLVL_gm | RTC_OVFINTLVL_gm)) | irq_ovf | irq_comp;
#endif
}

void xmega_rtc_enable(void)
{
#ifdef RTC32
	RTC32.CTRL |= RTC32_ENABLE_bm;
#endif
}

void xmega_rtc_set_prescaler(uint8_t prescaler)
{
#ifdef RTC
	RTC.CTRL = (RTC.CTRL & ~RTC_PRESCALER_gm) | (prescaler & RTC_PRESCALER_gm);
#endif
}

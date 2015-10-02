/*
 * Use -finstrument-functions to enable this functionality
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

#include "debugging.h"


#define MESSAGE_COUNT	5

typedef struct {
	bool enter;
	const void* this_fn;
	const void* call_site;
} message_t;

static uint8_t next_message_index = 0;
static message_t messages[MESSAGE_COUNT];
static const char* fault_name = NULL;



static inline void message_add(const bool type, void *this_fn, void *call_site)
{
	messages[next_message_index].enter = type;
	messages[next_message_index].this_fn = this_fn;
	messages[next_message_index].call_site = call_site;

	next_message_index = (next_message_index + 1) % MESSAGE_COUNT;

	vTaskCheckForStackOverflow();
}


void __cyg_profile_func_enter(void *this_fn, void *call_site) __attribute__((no_instrument_function));
void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
	message_add(true, this_fn, call_site);
}


void __cyg_profile_func_exit(void *this_fn, void *call_site)  __attribute__((no_instrument_function));
void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
	message_add(false, this_fn, call_site);
}


static void Unexpected_Interrupt(const char* const name)
{
	printf("Unexpected interrupt %s\n", name);

	print_stack_trace();
}


void print_stack_trace()
{
	uint8_t message_index = next_message_index;
	for (int i=0; i<MESSAGE_COUNT; i++) {
		const char* const type = messages[message_index].enter ? "ENTER" : "EXIT ";
		printf("%s function %p, from call %p\n", type, messages[message_index].this_fn, messages[message_index].call_site);
		message_index = (message_index + 1) % MESSAGE_COUNT;
	}
	printf("Enable function instrumentation (gcc -finstrument-functions), if the function call trace is undefined.");

	for(;;);
}


/**
 * @see http://www.freertos.org/Debugging-Hard-Faults-On-Cortex-M-Microcontrollers.html
 */
void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress)  __attribute__((no_instrument_function));
void prvGetRegistersFromStack(uint32_t *pulFaultStackAddress)
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used. If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */
  volatile uint32_t r0;
  volatile uint32_t r1;
  volatile uint32_t r2;
  volatile uint32_t r3;
  volatile uint32_t r12;
  volatile uint32_t lr; /* Link register. */
  volatile uint32_t pc; /* Program counter. */
  volatile uint32_t psr;/* Program status register. */

  r0 = pulFaultStackAddress[0];
  r1 = pulFaultStackAddress[1];
  r2 = pulFaultStackAddress[2];
  r3 = pulFaultStackAddress[3];

  r12 = pulFaultStackAddress[4];
  lr = pulFaultStackAddress[5];
  pc = pulFaultStackAddress[6];
  psr = pulFaultStackAddress[7];

  /* When the following line is hit, the variables contain the register values. */
  printf("r0 0x%lx, r1 0x%lx, r2 0x%lx, r3 0x%lx, r12 0x%lx, lr 0x%lx, pc 0x%lx, psr 0x%lx\n", r0, r1, r2, r3, r12, lr, pc, psr);

  Unexpected_Interrupt(fault_name);
}


/* The prototype shows it is a naked function - in effect this is just an
assembly function. */
void HardFault_Handler( void ) __attribute__( ( naked ) );

/* The fault handler implementation calls a function called
prvGetRegistersFromStack(). */
void HardFault_Handler()
{
	fault_name = __func__;

	__asm volatile (
		" tst lr, #4                                                \n"
		" ite eq                                                    \n"
		" mrseq r0, msp                                             \n"
		" mrsne r0, psp                                             \n"
		" ldr r1, [r0, #24]                                         \n"
		" ldr r2, handler2_address_const                            \n"
		" bx r2                                                     \n"
		" handler2_address_const: .word prvGetRegistersFromStack    \n"
		);
}


void NMI_Handler() { Unexpected_Interrupt(__func__); }
void MemManage_Handler() { Unexpected_Interrupt(__func__); }
void BusFault_Handler() { Unexpected_Interrupt(__func__); }
void UsageFault_Handler() { Unexpected_Interrupt(__func__); }
/*void SVC_Handler() { Unexpected_Interrupt(__func__); }*/
void DebugMon_Handler() { Unexpected_Interrupt(__func__); }
/*void PendSV_Handler() { Unexpected_Interrupt(__func__); }*/
/*void SysTick_Handler() { Unexpected_Interrupt(__func__); }*/
void WWDG_IRQHandler() { Unexpected_Interrupt(__func__); }
void PVD_IRQHandler() { Unexpected_Interrupt(__func__); }
void TAMP_STAMP_IRQHandler() { Unexpected_Interrupt(__func__); }
void RTC_WKUP_IRQHandler() { Unexpected_Interrupt(__func__); }
void FLASH_IRQHandler() { Unexpected_Interrupt(__func__); }
void RCC_IRQHandler() { Unexpected_Interrupt(__func__); }
void EXTI0_IRQHandler() { Unexpected_Interrupt(__func__); }
void EXTI1_IRQHandler() { Unexpected_Interrupt(__func__); }
void EXTI2_IRQHandler() { Unexpected_Interrupt(__func__); }
void EXTI3_IRQHandler() { Unexpected_Interrupt(__func__); }
/*void EXTI4_IRQHandler() { Unexpected_Interrupt(__func__); }*/
void DMA1_Stream0_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream1_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream2_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream3_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream4_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream5_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream6_IRQHandler() { Unexpected_Interrupt(__func__); }
void ADC_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN1_TX_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN1_RX0_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN1_RX1_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN1_SCE_IRQHandler() { Unexpected_Interrupt(__func__); }
void EXTI9_5_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM1_BRK_TIM9_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM1_UP_TIM10_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM1_TRG_COM_TIM11_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM1_CC_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM2_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM3_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM4_IRQHandler() { Unexpected_Interrupt(__func__); }
void I2C1_EV_IRQHandler() { Unexpected_Interrupt(__func__); }
void I2C1_ER_IRQHandler() { Unexpected_Interrupt(__func__); }
void I2C2_EV_IRQHandler() { Unexpected_Interrupt(__func__); }
void I2C2_ER_IRQHandler() { Unexpected_Interrupt(__func__); }
void SPI1_IRQHandler() { Unexpected_Interrupt(__func__); }
void SPI2_IRQHandler() { Unexpected_Interrupt(__func__); }
void USART1_IRQHandler() { Unexpected_Interrupt(__func__); }
void USART2_IRQHandler() { Unexpected_Interrupt(__func__); }
void USART3_IRQHandler() { Unexpected_Interrupt(__func__); }
void EXTI15_10_IRQHandler() { Unexpected_Interrupt(__func__); }
void RTC_Alarm_IRQHandler() { Unexpected_Interrupt(__func__); }
void OTG_FS_WKUP_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM8_BRK_TIM12_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM8_UP_TIM13_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM8_TRG_COM_TIM14_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM8_CC_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA1_Stream7_IRQHandler() { Unexpected_Interrupt(__func__); }
void FSMC_IRQHandler() { Unexpected_Interrupt(__func__); }
void SDIO_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM5_IRQHandler() { Unexpected_Interrupt(__func__); }
void SPI3_IRQHandler() { Unexpected_Interrupt(__func__); }
void UART4_IRQHandler() { Unexpected_Interrupt(__func__); }
void UART5_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM6_DAC_IRQHandler() { Unexpected_Interrupt(__func__); }
void TIM7_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream0_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream1_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream2_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream3_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream4_IRQHandler() { Unexpected_Interrupt(__func__); }
/*void ETH_IRQHandler() { Unexpected_Interrupt(__func__); }*/
void ETH_WKUP_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN2_TX_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN2_RX0_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN2_RX1_IRQHandler() { Unexpected_Interrupt(__func__); }
void CAN2_SCE_IRQHandler() { Unexpected_Interrupt(__func__); }
void OTG_FS_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream5_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream6_IRQHandler() { Unexpected_Interrupt(__func__); }
void DMA2_Stream7_IRQHandler() { Unexpected_Interrupt(__func__); }
void USART6_IRQHandler() { Unexpected_Interrupt(__func__); }
void I2C3_EV_IRQHandler() { Unexpected_Interrupt(__func__); }
void I2C3_ER_IRQHandler() { Unexpected_Interrupt(__func__); }
void OTG_HS_EP1_OUT_IRQHandler() { Unexpected_Interrupt(__func__); }
void OTG_HS_EP1_IN_IRQHandler() { Unexpected_Interrupt(__func__); }
void OTG_HS_WKUP_IRQHandler() { Unexpected_Interrupt(__func__); }
void OTG_HS_IRQHandler() { Unexpected_Interrupt(__func__); }
void DCMI_IRQHandler() { Unexpected_Interrupt(__func__); }
void HASH_RNG_IRQHandler() { Unexpected_Interrupt(__func__); }
void FPU_IRQHandler() { Unexpected_Interrupt(__func__); }

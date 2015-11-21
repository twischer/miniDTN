/*
 * Use -finstrument-functions to enable this functionality
 */

#include "debugging.h"

#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "lib/mmem.h"
#include "delay.h"




#define MESSAGE_COUNT	32
#define TASKS_COUNT		8

static const char IDLE_TASK_NAME[] = "IDLE";

typedef struct {
	bool enter;
	const void* this_fn;
	const void* call_site;
	const char* task_name;
} message_t;

static volatile bool disable_stack_tracing = false;
static size_t next_message_index = 0;
static message_t messages[MESSAGE_COUNT];
static const char* fault_name = NULL;


#if (REMEMBER_TASKS == 1)
typedef struct {
	TickType_t time_stamp;
	const char* task_name;
} task_time_t;

static task_time_t remembered_tasks[TASKS_COUNT];
#endif


#if (PRINT_CPU_USAGE == 1)
static uint64_t usage_time = 0;
static TickType_t task_in_time = 0;
#endif


static inline bool is_scheduler_running()
{
	static bool is_scheduler_running = false;
	if (!is_scheduler_running) {
		is_scheduler_running = (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
	}

	return is_scheduler_running;
}


static inline const char* const get_task_name()
{
	/* only try to determine the task name, if the scheduler was started once */
	const char* const task_name = is_scheduler_running() ? pcTaskGetTaskName(NULL) : "INIT";

	return task_name;
}


static TickType_t time_diff_start_ticks = 0;
void time_diff_start(const char* const file, const int line)
{
	if (time_diff_start_ticks > 0) {
		printf("ERR: Last time diff not already stopped from %s:%u\n", file, line);
		time_diff_stop(file, line);
	}

	time_diff_start_ticks = xTaskGetTickCount();
}

void time_diff_stop(const char* const file, const int line)
{
	if (time_diff_start_ticks <= 0) {
		printf("ERR: Time diff not started from %s:%u\n", file, line);
		return;
	}
	const TickType_t diff = xTaskGetTickCount() - time_diff_start_ticks;
	time_diff_start_ticks = 0;

	printf("TIME %lu from %s:%u\n", diff / portTICK_PERIOD_MS, file, line);
}

void time_diff_assert_too_big(const TickType_t max_time_diff, const char* const file, const int line)
{
	if (time_diff_start_ticks <= 0) {
		printf("ERR: Time diff not started from %s:%u\n", file, line);
		return;
	}

	const TickType_t diff = xTaskGetTickCount() - time_diff_start_ticks;
	if (diff > max_time_diff) {
		printf("ASSERTION: Time bigger than %lu from %s:%u\n", max_time_diff, file, line);
		time_diff_stop(file, line);
		for(;;);
	}

	/* reset time diff for next start call */
	time_diff_start_ticks = 0;
}


#if (REMEMBER_TASKS == 1)
void remember_task()
{
	const char* const task_name = get_task_name();

	/* look for already existing task entry */
	for (int i=0; i<TASKS_COUNT; i++) {
		/* the task names are static string variables
		 * So only comparing the pointers is enough.
		 */
		if (remembered_tasks[i].task_name == task_name) {
			/* task entry already exists */
			remembered_tasks[i].time_stamp = xTaskGetTickCount();
			return;
		}
	}

	/* look for an empty task entry */
	for (int i=0; i<TASKS_COUNT; i++) {
		if (remembered_tasks[i].task_name == NULL) {
			remembered_tasks[i].time_stamp = xTaskGetTickCount();
			remembered_tasks[i].task_name = task_name;
			return;
		}
	}

	printf("WRN: Not remembering task %s. List is full.\n", task_name);
}

void print_remembered_tasks()
{
	for (int i=0; i<TASKS_COUNT; i++) {
		if (remembered_tasks[i].task_name == NULL) {
			break;
		}

		printf("Task %s time %lu\n", remembered_tasks[i].task_name, remembered_tasks[i].time_stamp);
	}
}
#endif

/* this variables are defined in the linker script */
extern uint8_t _estack;
extern uint8_t _Min_Stack_Size;

static uint32_t highes_stack_usage = 0;
void check_for_stack_overflow()
{
	/* check user stack until FreeRTOS tasks were started */
	if (is_scheduler_running()) {
#if (CHECK_FREERTOS_STACK_OVERFLOW == 1)
		vTaskCheckForStackOverflow();
#endif
	} else {
//		volatile uint32_t stack_pointer = 0;
//		__asm volatile(
//			"ldr r0, =stack_pointer  \n"		/* put address of C variable stackptr in r0 */
//			"mov r1, sp         \n"		/* move value of sp to r1 */
//			"str r1, [r0]       \n"		/* put value of r1 in address contained in r0 (stackptr) */
//		);
		const uint8_t last_stack_entry = 0;
		const uint32_t stack_pointer = (uint32_t)&last_stack_entry;

		const uint32_t stack_begin = (uint32_t)&_estack;
		const uint32_t stack_usage = stack_begin - stack_pointer;
		if (stack_usage > highes_stack_usage) {
			highes_stack_usage = stack_usage;

			const uint32_t stack_size = (uint32_t)&_Min_Stack_Size;
			if (stack_usage > stack_size) {
				printf("USER STACK OVERFLOW (usage 0x%lx)\n", stack_usage);
				print_stack_trace();
			}
		}
	}
}


static inline void message_add_task(const bool type, void *this_fn, void *call_site, const char* const task_name)
		__attribute__((no_instrument_function));
static inline void message_add_task(const bool type, void *this_fn, void *call_site, const char* const task_name)
{
	if (disable_stack_tracing) {
		return;
	}

	messages[next_message_index].enter = type;
	messages[next_message_index].this_fn = this_fn;
	messages[next_message_index].call_site = call_site;
	messages[next_message_index].task_name = task_name;

	next_message_index = (next_message_index + 1) % MESSAGE_COUNT;
}


static inline void message_add(const bool type, void *this_fn, void *call_site) __attribute__((no_instrument_function));
static inline void message_add(const bool type, void *this_fn, void *call_site)
{
	if (disable_stack_tracing) {
		return;
	}

	static uint32_t recursion_deeps = 0;
	/* ignore recursive calls */
	if (recursion_deeps > 0) {
		return;
	}
	recursion_deeps++;


	const char* const task_name = get_task_name();
	message_add_task(type, this_fn, call_site, task_name);


	check_for_stack_overflow();

#if (CHECK_MMEM_CONSISTENCY == 1)
	mmem_check();
#endif

	recursion_deeps--;
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


static void Unexpected_Interrupt(const char* const name)  __attribute__((no_instrument_function));
static void Unexpected_Interrupt(const char* const name)
{
	printf("Unexpected interrupt %s\n", name);

	print_stack_trace();
}


#if (PRINT_CPU_USAGE == 1)
static inline void print_cpu_usage()
{
	const TickType_t current_time = xTaskGetTickCountFromISR();
	static TickType_t last_print_time = 0;

	const TickType_t diff = current_time - last_print_time;
	if ( diff > pdMS_TO_TICKS(CPU_USAGE_INTERVAL_MS) ) {
		last_print_time = current_time;

		const uint64_t running_time_ms = ((uint64_t)diff) / portTICK_PERIOD_MS;
		const uint64_t running_time_hsys_clk = running_time_ms * (SystemCoreClock / 1000 / 2);
		const uint8_t usage = (usage_time * 100) / running_time_hsys_clk;
		usage_time = 0;

		printf("\nCPU %u%%\n\n", usage);
	}
}
#endif


void task_switch_in(const TaskHandle_t task, const char* const name)
{
	/* ignore idle task switches */
	if (task == xTaskGetIdleTaskHandle()) {
#if (PRINT_CPU_USAGE == 1)
		print_cpu_usage();
#endif
		return;
	}

#if (PRINT_CPU_USAGE == 1)
	// TODO measure time between switch outs for non idle tasks.
	// So the sceduling time will be measured, too.
	/* reset usage time */
	TIM5->CNT = 0;
	/* reset the overflow bit */
	TIM5->SR &= ~TIM_SR_UIF;
	task_in_time = xTaskGetTickCountFromISR();
#endif


#if (LOG_TASK_SWITCHING == 1)
	message_add_task(true, NULL, NULL, name);
#endif

#if (PRINT_TASK_SWITCHING == 1)
	printf("IN %s\n", name);
#endif
}


void task_switch_out(const TaskHandle_t task, const char* const name)
{
	/* ignore idle task switches */
	if (task == xTaskGetIdleTaskHandle()) {
		return;
	}

#if (PRINT_CPU_USAGE == 1)
	/* check for overflow */
	if ((TIM5->SR & TIM_SR_UIF) == TIM_SR_UIF ) {
		const TickType_t diff = xTaskGetTickCountFromISR() - task_in_time;
		const uint64_t diff_ms = ((uint64_t)diff) / portTICK_PERIOD_MS;
		usage_time += diff_ms * (SystemCoreClock / 1000 / 2);
	} else {
		const uint32_t last_usage_time = TIM3->CNT;
		usage_time += last_usage_time;
	}
#endif

#if (PRINT_TASK_SWITCHING == 1)
	printf("OUT %s\n", name);
#endif
}


void task_yield()
{
	static int i=0;
	i++;
}


#if (PRINT_BLOCKING_TASKS == 1)
void task_blocked(QueueHandle_t queue)
{
	/* do not process queues onyl semaphores and mutex */
	const uint8_t type = ucQueueGetQueueType(queue);
	if (type == queueQUEUE_TYPE_SET) {
		return;
	}

	const char* const blocked_task_name = pcTaskGetTaskName(NULL);

	const TaskHandle_t holder = xSemaphoreGetMutexHolder(queue);
	const char* const blocking_task_name = (holder != NULL) ? pcTaskGetTaskName(holder) : "UNKNOWN";

	print_stack_trace_part_not_blocking(5);
	printf("Task '%s' is blocking on queue %p with type %u. Task '%s' has the lock.\n",
		   blocked_task_name, queue, type, blocking_task_name);
}
#endif


void print_stack_trace_part_not_blocking(const size_t count)
{
	/* disabling adding function calls to the stack trace,
	 * becasue the following functions only called for debugging
	 * output.
	 */
	disable_stack_tracing = true;

	/* calulate the beginning of the reqested stack trace entries.
	 * <next_message_index> points to the oldes entry.
	 * To get the <count> last entries loop in the FIFO,
	 * becasue modulo would wrongly interret negativ values.
	 */
	size_t message_index = (next_message_index + (MESSAGE_COUNT - count)) % MESSAGE_COUNT;
	for (size_t i=0; i<count; i++) {
		const char* const type = messages[message_index].enter ? "ENTER" : "EXIT ";
		const char* task_name = messages[message_index].task_name;
		if (task_name == NULL) {
			task_name = "UNKNOWN";
		}
		printf("%s function %p, from call %p, in task '%s'\n", type, messages[message_index].this_fn,
			   messages[message_index].call_site, task_name);
		message_index = (message_index + 1) % MESSAGE_COUNT;
	}

	/* enable stack tracing again,
	 * becasue this function can return
	 */
	disable_stack_tracing = false;
}


void print_stack_trace_part(const size_t count)
{
	taskDISABLE_INTERRUPTS();


#if (REMEMBER_TASKS == 1)
	print_remembered_tasks();
#endif

	print_stack_trace_part_not_blocking(count);

	printf("Enable function instrumentation (gcc -finstrument-functions), if the function call trace is undefined.\n");
	for(;;);
}


void print_stack_trace()
{
	print_stack_trace_part(MESSAGE_COUNT);
}


void reset_stack_trace()
{
	memset(messages, 0, sizeof(messages));
}


void print_memory(const uint8_t* const data, const size_t length)
{
	for (size_t i=0; i<length;) {
		printf("%p: ", &data[i]);

		const size_t next_new_line = i + 16;
		for (; i<next_new_line; i++) {
			printf(" %02x", data[i]);
		}
		printf("\n");
	}
}


void delay_us_check(void)
{
	const uint32_t delay_time_us = 50 * 1000;
	const uint32_t delay_repeats = 20;

	const TickType_t start = xTaskGetTickCount();
	for (uint32_t i=0; i<delay_repeats; i++) {
		delay_us(delay_time_us);
	}

	const TickType_t end = xTaskGetTickCount();
	const uint32_t tick_diff_ms = (end - start) / portTICK_PERIOD_MS;
	const uint32_t delay_diff_ms = delay_time_us * delay_repeats / 1000;
	printf("delay_us_check %lu - %lu = %lu\n", tick_diff_ms, delay_diff_ms, (tick_diff_ms - delay_diff_ms));
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
void HardFault_Handler( void ) __attribute__((naked,no_instrument_function));

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

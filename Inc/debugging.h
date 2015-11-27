#ifndef DEBUGGING_H
#define DEBUGGING_H

#include <stdint.h>
#include <stdlib.h>

/* source has to be compiled with enabled function instrumentation
 * to use one of this validation checks.
 * Uncomment
 * INSTRUMENTATION := -finstrument-functions
 * in Makefile.include to enable function instrumentation
 * for nearly all source files.
 */
#define configCHECK_FOR_STACK_OVERFLOW	0
#define CHECK_FREERTOS_STACK_OVERFLOW	0
#define CHECK_MMEM_CONSISTENCY			0

/* if enabling logging of FreeRTOS task switching,
 * the tracing of FreeRTOS has to be enabled.
 * #define traceTASK_SWITCHED_IN() task_switch_in(pxCurrentTCB->pcTaskName);
 * #define traceTASK_SWITCHED_OUT() task_switch_out(pxCurrentTCB->pcTaskName);
 */
#define PRINT_TASK_SWITCHING			0
#define LOG_TASK_SWITCHING				0
#define PRINT_CPU_USAGE					0
#define CPU_USAGE_INTERVAL_MS			1000

/* if enabling logging of FreeRTOS task blocking,
 * the tracing of FreeRTOS has to be enabled.
 * #define traceBLOCKING_ON_QUEUE_RECEIVE(xQueue) task_blocked(xQueue);
 */
#define PRINT_BLOCKING_TASKS            0

#define REMEMBER_TASKS                  0


/* copied from task.h and queue.h.
 * These files can not include here,
 * becasue there would be an include recursion
 */
typedef uint32_t TickType_t;
typedef void * TaskHandle_t;
typedef void * QueueHandle_t;


void task_switch_in(const TaskHandle_t task, const char* const name)  __attribute__((no_instrument_function));
void task_switch_out(const TaskHandle_t task, const char* const name)  __attribute__((no_instrument_function));
void task_yield()  __attribute__((no_instrument_function));

#if (PRINT_BLOCKING_TASKS == 1)
void task_blocked(QueueHandle_t queue)  __attribute__((no_instrument_function));
#endif

void check_for_stack_overflow();

void print_stack_trace(void)  __attribute__((no_instrument_function));
void print_stack_trace_part(const size_t count)  __attribute__((no_instrument_function));
void print_stack_trace_part_not_blocking(const size_t count)  __attribute__((no_instrument_function));
void reset_stack_trace(void);
void print_memory(const uint8_t* const data, const size_t length);
void delay_us_check(void);


#define TIME_DIFF_START() time_diff_start(__func__, __LINE__);
#define TIME_DIFF_STOP() time_diff_stop(__func__, __LINE__);
#define TIME_DIFF_ASSERT(max_diff) time_diff_assert_too_big(max_diff, __func__, __LINE__);

void time_diff_start(const char* const file, const int line);
void time_diff_stop(const char* const file, const int line);
void time_diff_assert_too_big(const TickType_t max_time_diff, const char* const file, const int line);

#if (REMEMBER_TASKS == 1)
void print_remembered_tasks();
void remember_task();
#endif

#endif // DEBUGGING_H


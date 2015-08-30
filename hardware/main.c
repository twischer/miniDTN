#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx.h"
#include "stm32f4xx_usart.h"
#include "stm32_ub_led.h"
#include "delay.h"

#include "net/netstack.h"
#include "rf230bb.h"
#include "dtn_network.h"

static unsigned long current_seconds = 0;

void init_USART(void);

bool init();


int main(void) {
  SystemInit();
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  init_USART();
  delay_init();

  UB_Led_Init(); // Init der LEDs
  UB_Led_On(LED_GREEN); // gruene LED einschalten

  /* init the network stack */
  // TODO should be moved to own file
  linkaddr_t inv_id = { {0x05, 0xB9} };
  linkaddr_set_node_addr(&inv_id);

  rf230_driver.init();
//  if (eui64_is_null(inga_cfg.eui64_addr)) {
	rf230_set_pan_addr(0x0780, 0x05B9, NULL);
//	const uint8_t ieee_addr[8] = { 0xd3, 0xbb, 0xf5, 0xff, 0xfe, 0x4c, 0x22, 0xcb };
//	rf230_set_pan_addr(0x0780, 0x05B9, ieee_addr);
//  } else {
//    rf230_set_pan_addr(inga_cfg.pan_id, inga_cfg.pan_addr, inga_cfg.eui64_addr);
//  }

  rf230_set_channel(26);
  rf230_set_txpower(0);

  /* Initialize stack protocols */
  queuebuf_init();
  dtn_network_driver.init();

//  // TODO only for testing
//  int counter = 0;
//  while (1) {
//  const u8 data[] = { 0x41, 0x98, 0x6a, 0x80, 0x07, 0x02, 0x00, 0xb9, 0x05, 0x17, 0x06, 0x10, 0x11, 0x02, 0x0b, 0x8b, 0x39, 0x0c, 0x8b, 0x39, 0x8b, 0x39, 0x8b,
//					  0x39, 0x8b, 0x39, 0x00, 0x01, 0x00, 0x00, 0x0a, 0x01, 0x03, 0x83, 0xfb, 0x68, 0x01, 0x08, 0x40, 0x02, 0x00, 0x00, 0x00, 0x04, 0x05, 0x06,
//					  0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
//					  0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34,
//					  0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f };
//  const int ret = rf230_driver.send(data, sizeof(data));
//  printf("Send finished with %d\n", ret);

//  /* wait 2 sec */
//  for (int i=0; i<40; i++) {
//	delay_us(50000);
//  }
//  printf("wait %d\n", counter);
//  counter++;

//  }

  const bool successful = init();
  if (successful) {
    printf("System Started!\n");
    vTaskStartScheduler();  // should never return
  } else {
    printf("System Error!\n");
    // --TODO blink some LEDs to indicate fatal system error
	UB_Led_On(LED_RED);
  }

  for (;;);
}

void vApplicationTickHook(void)
{
	static TickType_t ticks_over_second = 0;
	ticks_over_second++;

	if (ticks_over_second >= pdMS_TO_TICKS(1000)) {
		ticks_over_second = 0;
		current_seconds++;
	}
}

unsigned long clock_seconds()
{
	return current_seconds;
}

/* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created.  It is also called by various parts of the
   demo application.  If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
void vApplicationMallocFailedHook(void) {
  taskDISABLE_INTERRUPTS();
  for(;;);
}

/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
   task.  It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()).  If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
void vApplicationIdleHook(void) {
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, signed char *pcTaskName) {
  /* Run time stack overflow checking is performed if
     configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
     function is called if a stack overflow is detected. */
  taskDISABLE_INTERRUPTS();
  printf("STACK OVERFLOW in task '%s (handle %p)\n", pcTaskName, pxTask);
  for(;;);
}

/*
 * Configure USART6(TxD -> PC6) to redirect printf data to host PC.
 */
void init_USART(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);

	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 115200;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx;
	USART_Init(USART6, &USART_InitStruct);
	USART_Cmd(USART6, ENABLE);
}


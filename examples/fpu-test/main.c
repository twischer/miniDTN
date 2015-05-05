#include "FreeRTOS.h"
#include "task.h"
#include "math.h"
#include "stdio.h"
#include "stm32_ub_led.h"

void test_FPU_test(void* p);

int init(void) {

  // Create a task
  int ret = xTaskCreate(test_FPU_test, "FPU", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  return ret;
}

void test_FPU_test(void* p) {
  float ff = 1.0f;
  printf("Start FPU test task.\n");
  for (;;) {
	UB_Led_Toggle(LED_BLUE);
    float s = sinf(ff);
    ff += s;
    // TODO some other test
    vTaskDelay(1000);
  }
  vTaskDelete(NULL);
}

#include <stdio.h>
#include <contiki.h>
#include "test.h"
#include "fat_test.h"
#include "sensor-tests.h"

#define DEBUG 0
#if DEBUG
#define PRINTD(...) printf(__VA_ARGS__)
#else
#define PRINTD(...)
#endif

static char * run_tests();
static struct etimer timer;
int tests_run = 0;

/*---------------------------------------------------------------------------*/
PROCESS(test_process, "Test process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  // Wait a second...
  etimer_set(&timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&timer));

  PRINTD("Starting test...\n");

  // Run tests...
  char *result = run_tests();
  if (result != 0) {
    printf("%s\n", result);
  } else {
    printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);

  return result != 0;

  cfs_fat_umount_device();
  
//  printf("#################################################################");

  while (1);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static char *
run_tests()
{
  RUN_TEST(acc_tests);
  RUN_TEST(gyro_tests);
  RUN_TEST(battery_tests);
  RUN_TEST(fat_tests);
  return 0;
}

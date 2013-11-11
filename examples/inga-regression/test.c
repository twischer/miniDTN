#include "test.h"

#define ABORT() while(1) {}

static void report_fail(int a, int b, char *msg);

void
test_eq(uint32_t a, uint32_t b, char* test) {
  if (!(a == b)) {
    suite.errors++;
    report_fail(a, b, test);
    ABORT();
  }
}


void
test_neq(uint32_t a, uint32_t b, char* test) {
  if (!(a != b)) {
    suite.errors++;
    report_fail(a, b, test);
    ABORT();
  }
}

void
test_leq(uint32_t a, uint32_t b, char* test) {
  if (!(a <= b)) {
    suite.errors++;
    report_fail(a, b, test);
    ABORT();
  }
}

void
test_geq(uint32_t a, uint32_t b, char* test) {
  if (!(a >= b)) {
    suite.errors++;
    report_fail(a, b, test);
    ABORT();
  }
}

static void report_fail(int a, int b, char *msg) {
  static char buffer_[128];
  sprintf(buffer_, "%s@%s=>%s, %d, %d", suite.test_name, suite.test_type, msg, a, b);
  TEST_FAIL(buffer_); \
}


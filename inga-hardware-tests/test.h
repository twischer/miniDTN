/** 
 * \file 
 *        Test framework
 * \author
 *        Enrico Joerns <joerns@ibr.cs.tu-bs.de>
 */

#ifndef TEST_H
#define	TEST_H

#include <sys/test.h>

//static uint8_t errors;
static const char *precond_ = "PRE";
static const char *postcond_ = "POST";
static const char *testcode_ = "EXEC";
//static char *test_name = "unknwon";
//static const char *test_type = "unknown";
//static char *suite_name = "unkown";

struct test_suite {
  char *suite_name;
  char *test_name;
  char *test_type;
  uint8_t errors;
  uint16_t overall_errors;
};

extern struct test_suite suite;

#define TEST_SUITE(name) \
  struct test_suite suite = {"unknown", "unknown", "unknown",0};

#define TEST_ASSERT(a, b)  if (!(b)) { \
  TEST_REPORT(a, clock_seconds(), CLOCK_SECOND, "s"); \
  suite.errors++; \
  }

/** @depcrecated */
#define ASSERT_CRITICAL(a, b) if (!(b)) { \
  ASSERT(a, b) \
  sprintf(&noe, "Failed with %d errors", suite.errors); \
  TEST_FAIL(err_msg); \
  }

/** Start of test with given name. */
#define TEST_BEGIN(a) \
  suite.errors = 0; \
  suite.test_type = testcode_; \
  suite.test_name = a

/** End of test. */
#define TEST_END()  \
  if (suite.errors != 0 || suite.overall_errors != 0) {  \
    TEST_FAIL(suite.test_name); \
  }
  

#define REPORT_WARNING  0
#define REPORT_FAIL     1

/** \name Code sections
 * @{ */
/** Starts precondition section. */
#define TEST_PRE() \
  suite.test_type = precond_
/** Starts test section. */
#define TEST_CODE() \
  suite.test_type = testcode_
/** Starts postcondition section. */
#define TEST_POST() \
  suite.test_type = postcond_
/** @} */

/** Tests a, b for equality.
 * Reports error if failed.
 */
#define TEST_EQUALS(a, b) \
  if ((a) != (b)) { \
    suite.errors++; \
    report_fail(a, b, #a " != " #b); \
    return; \
  }

/** Tests a, b for unequality.
 * Reports error if failed.
 */
#define TEST_NEQ(a, b) \
  if ((a) == (b)) { \
    suite.errors++; \
    report_fail(a, b, #a " == " #b); \
    return; \
  }

/** Tests for a <= b .
 * Reports error if failed.
 */
#define TEST_LEQ(a, b) \
  if ((a) > (b)) { \
    suite.errors++; \
    report_fail(a, b, #a " > " #b); \
    return; \
  }

/** Tests for a >= b .
 * Reports error if failed.
 */
#define TEST_GEQ(a, b) \
  if ((a) < (b)) { \
    suite.errors++; \
    report_fail(a, b, #a " < " #b); \
    return; \
  }

static void report_fail(int a, int b, char *msg) {
  static char buffer_[128];
  sprintf(buffer_, "%s@%s=>%s, %d, %d", suite.test_name, suite.test_type, msg, a, b);
  TEST_FAIL(buffer_); \
}

#define TESTS_PRINT_RESULT(desc) \
  if (suite.errors == 0) {  \
    TEST_RESULT(desc, "\x1b[0;32mOK\x1b[;0m");\
  }else{\
    TEST_RESULT(desc, "\x1b[0;31mFAILED\x1b[;0m");\
  }\
  suite.overall_errors+=suite.errors;\
  suite.errors=0;

/** Indicates end of test bundle. */
#define TESTS_DONE() \
  if (suite.errors == 0 && suite.overall_errors == 0) {  \
    TEST_PASS();  \
  }

/** Runs test if previous finished without failure. */
#define RUN_TEST(name, test) \
  if (suite.errors != 0 || suite.overall_errors != 0) return 1; \
  TEST_BEGIN(name); \
  test(); \
  TEST_END()

//extern int tests_run;

#endif	/* TEST_H */


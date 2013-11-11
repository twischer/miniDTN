/** 
 * \file 
 *        Test framework
 * \author
 *        Enrico Joerns <joerns@ibr.cs.tu-bs.de>
 */

#ifndef TEST_H
#define	TEST_H

#include <sys/test.h>

void test_eq(uint32_t a, uint32_t b, char* test);
void test_neq(uint32_t a, uint32_t b, char* test);
void test_leq(uint32_t a, uint32_t b, char* test);
void test_geq(uint32_t a, uint32_t b, char* test);

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
};

extern struct test_suite suite;

#define TEST_SUITE(name) \
  struct test_suite suite = {"unknown", "unknown", "unknown",0};

/** Start of test with given name. */
#define TEST_BEGIN(a) \
  suite.errors = 0; \
  suite.test_type = testcode_; \
  suite.test_name = a

/** End of test. */
#define TEST_END()  \
  if (suite.errors != 0) {  \
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
#define TEST_EQUALS(a, b) test_eq(a, b, #a " != " #b)

/** Tests a, b for unequality.
 * Reports error if failed.
 */
#define TEST_NEQ(a, b) test_neq(a, b, #a " == " #b)

/** Tests for a <= b .
 * Reports error if failed.
 */
#define TEST_LEQ(a, b) test_leq(a, b, #a " > " #b)

/** Tests for a >= b .
 * Reports error if failed.
 */
#define TEST_GEQ(a, b) test_geq(a, b, #a " < " #b)

/** Indicates end of test bundle. */
#define TESTS_DONE() \
  if (suite.errors == 0) {  \
    TEST_PASS();  \
  }

/** Runs test if previous finished without failure. */
#define RUN_TEST(name, test) \
  if (suite.errors != 0) return 1; \
  TEST_BEGIN(name); \
  test(); \
  TEST_END()

//extern int tests_run;

#endif	/* TEST_H */


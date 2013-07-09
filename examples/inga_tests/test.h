/* 
 * File:   test.h
 * Author: enrico
 *
 * Created on 13. März 2013, 01:13
 */

#ifndef TEST_H
#define	TEST_H

static uint8_t errors;

#define ASSERT(a, b)  if (!(b)) {TEST_REPORT(a, 0, CLOCK_SECOND, "s"); errors++;}
//#define RUN_TEST(test) do { char *message = test(); tests_run++; \
                                if (message) return message; } while (0)
extern int tests_run;

#endif	/* TEST_H */


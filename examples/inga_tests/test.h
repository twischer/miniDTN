/* 
 * File:   test.h
 * Author: enrico
 *
 * Created on 13. März 2013, 01:13
 */

#ifndef TEST_H
#define	TEST_H

#define ASSERT(message, test) do { if (!(test)) return message; } while (0)
#define RUN_TEST(test) do { char *message = test(); tests_run++; \
                                if (message) return message; } while (0)
extern int tests_run;

#endif	/* TEST_H */


#ifndef __TEST_H__
#define __TEST_H__

#include <stdio.h>

#define TEST_FAIL(reason) printf("TEST:FAIL:%s\n", (reason))
#define TEST_PASS() printf("TEST:PASS\n")
#define TEST_REPORT(desc, value, scale, unit) printf("TEST:REPORT:%s:%li:%li:%s\n", (desc), ((uint32_t)value), ((uint32_t)scale), (unit))

uint32_t test_precise_timestamp(uint8_t * is_valid);

#endif /* __TEST_H__ */

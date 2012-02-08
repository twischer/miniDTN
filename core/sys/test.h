#ifndef __TEST_H__
#define __TEST_H__

#include <avr/pgmspace.h>
#include <stdio.h>

#define TEST_FAIL(reason) printf_P(PSTR("TEST:FAIL:%s\n"), (reason))
#define TEST_PASS() printf_P(PSTR("TEST:PASS\n"))
#define TEST_REPORT(desc, value, scale, unit) printf_P(PSTR("TEST:REPORT:%s:%li:%li:%s\n"), (desc), ((uint32_t)value), ((uint32_t)scale), (unit))

#endif /* __TEST_H__ */

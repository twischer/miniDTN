#ifndef __TEST_H__
#define __TEST_H__

#include <avr/pgmspace.h>
#include <stdio.h>

#define TEST_FAIL(value, scale, unit) printf_P(PSTR("TEST:FAIL:%li:%li:%s\n"), value, scale, unit)
#define TEST_PASS(value, scale, unit) printf_p(PSTR("TEST:PASS:%li:%li:%s\n"), value, scale, unit)

#endif /* __TEST_H__ */

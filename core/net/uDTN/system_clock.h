/*
 * clock.h
 *
 *  Created on: 17.12.2013
 *      Author: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 */

#include "contiki.h"

#ifndef SYSTEM_CLOCK_H_
#define SYSTEM_CLOCK_H_

typedef struct {
	long int tv_sec;
	long int tv_usec;
} udtn_timeval_t;

typedef unsigned long udtn_time_t;

/* Convenience macros for operations on timevals.
   NOTE: `timercmp' does not work for >= or <=.  */
# define udtn_timerisset(tvp)	((tvp)->tv_sec || (tvp)->tv_usec)
# define udtn_timerclear(tvp)	((tvp)->tv_sec = (tvp)->tv_usec = 0)
# define udtn_timercmp(a, b, CMP) 						      \
  (((a)->tv_sec == (b)->tv_sec) ? 					      \
   ((a)->tv_usec CMP (b)->tv_usec) : 					      \
   ((a)->tv_sec CMP (b)->tv_sec))
# define udtn_timeradd(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;			      \
    if ((result)->tv_usec >= 1000000)					      \
      {									      \
	++(result)->tv_sec;						      \
	(result)->tv_usec -= 1000000;					      \
      }									      \
  } while (0)
# define udtn_timersub(a, b, result)						      \
  do {									      \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;			      \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;			      \
    if ((result)->tv_usec < 0) {					      \
      --(result)->tv_sec;						      \
      (result)->tv_usec += 1000000;					      \
    }									      \
  } while (0)


/**
 * initialize the system clock
 */
void udtn_clock_init();

/**
 * returns the time as the number of seconds since the Epoch,
 * 1970-01-01 00:00:00 +0000 (UTC).
 *
 * If t is non-NULL, the return value is also stored in the memory
 * pointed to by t.
 */
udtn_time_t udtn_time(udtn_time_t *t);

void udtn_settimeofday(udtn_timeval_t *tv);

void udtn_gettimeofday(udtn_timeval_t *tv);

void udtn_uptime(udtn_timeval_t *tv);

#endif /* SYSTEM_CLOCK_H_ */

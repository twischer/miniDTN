/*
 * clock.h
 *
 *  Created on: 17.12.2013
 *      Author: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 */

#include "contiki.h"

#ifndef SYSTEM_CLOCK_H_
#define SYSTEM_CLOCK_H_

typedef enum {
	UDTN_CLOCK_STATE_UNKNOWN,
	UDTN_CLOCK_STATE_POOR,
	UDTN_CLOCK_STATE_GOOD,
	UDTN_CLOCK_STATE_ACCURATE,
} udtn_clock_state_t;

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

#define UDTN_CLOCK_DTN_EPOCH_OFFSET 946684800ul

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

/**
 * This method sets the global clock offset used by
 * udtn_gettimeofday() to calculate the correct global
 * time.
 */
void udtn_settimeofday(udtn_timeval_t *tv);

/**
 * This method adds the global clock offset to the uptime
 * of the node and put that into the given timeval struct.
 * If udtn_settimeofday() has not been called before, this
 * method returns the same value as udtn_uptime().
 *
 * The caller of this method should be aware of the clock
 * state returned by udtn_getstate() to assess the
 * statement of the current clock value.
 */
void udtn_gettimeofday(udtn_timeval_t *tv);

/**
 * Stores the time since the boot-up of the node in
 * the timeval struct.
 */
void udtn_uptime(udtn_timeval_t *tv);

/**
 * Set the clock state
 */
void udtn_setclockstate(udtn_clock_state_t s);

/**
 * Get the clock state
 */
udtn_clock_state_t udtn_getclockstate();

#endif /* SYSTEM_CLOCK_H_ */

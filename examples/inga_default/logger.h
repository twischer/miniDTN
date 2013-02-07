/* 
 * File:   logger.h
 * Author: enrico
 *
 * Created on 5. Februar 2013, 19:11
 */

#ifndef LOGGER_H
#define	LOGGER_H

#include "contiki.h"
#include <stdio.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 4
#endif

#ifndef LOG_PROCESS
#define LOG_PROCESS 0
#endif

#if LOG_LEVEL

extern char log_buffer[64];
extern process_event_t event_log_data;

typedef enum {
  ERROR = 1,
  WARNING = 2,
  INFO = 3,
  VERBOSE = 4
} level;

/**
 * Logging process.
 */

#if LOG_PROCESS
#define log(lvl, args...) \
  sprintf(&log_buffer[0], lvl); \
  sprintf(&log_buffer[4], args); \
  process_post(&logger_process, event_log_data, &log_buffer);
#else
#define log_(lvl, args...) \
  sprintf(&log_buffer[0], lvl); \
  sprintf(&log_buffer[4], args); \
  printf(log_buffer);
#endif

#if LOG_LEVEL >= 1
#define log_e(args...) log_("EE: ", args)
#else
#define log_e(args...)
#endif
#if LOG_LEVEL >= 2
#define log_w(args...) log_("WW: ", args)
#else
#define log_w(args...)
#endif
#if LOG_LEVEL >= 3
#define log_i(args...) log_("II: ", args)
#else
#define log_i(args...)
#endif
#if LOG_LEVEL >= 4
#define log_v(args...) log_("VV: ", args)
#else
#define log_v(args...)
#endif

/**
 * 
 */
extern struct process logger_process;

#endif

#endif	/* LOGGER_H */


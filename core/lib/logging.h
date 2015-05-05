/*
 * Copyright (c) 2012, Daniel Willmann <daniel@totalueberwachung.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: $
 */
/**
 * \addtogroup logging
 * @{
 */

/**
 * \defgroup logging Logging subsystem
 *
 * The logging subsystem allows different logdomains to be configured.
 * For each of these logdomains the verbosity of the output can be set
 * during runtime.
 *
 * @{
 */

/**
 * \file
 *         Header file for the logging subsystem
 * \author
 *         Daniel Willmann <daniel@totalueberwachung.de>
 *
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>
#include <contiki.h>

#define LOGL_DBG 0
#define LOGL_INF 1
#define LOGL_WRN 2
#define LOGL_ERR 3
#define LOGL_CRI 4
#define LOGL_NUM 5 /* Always last! */

#define LOGD_CORE 0
#define LOGD_CPU  1
#define LOGD_INGA 2
#define LOGD_APP  3
#define LOGD_DTN  4
#define LOGD_NUM  5 /* Always last! */

#define SUBDOMS 10

struct log_cfg {
	uint8_t subl[SUBDOMS];
	uint8_t subdom_next;
};

extern struct log_cfg log_d[LOGD_NUM];

#ifdef ENABLE_LOGGING
/*---------------------------------------------------------------------------*/
/**
 * \brief         Log a message
 * \param logdom  log domain of the message
 * \param sdom    subdomain of the message
 * \param logl	  log level of the message
 * \author     Daniel Willmann
 *
 *             This macro is used to pring a log message of a given level and domain
 *
 * \hideinitializer
 */
#define LOG(logdom, sdom, logl, fmt, ...) do { \
		logging_logfn(logdom, sdom, logl, "[%s:%s](%s:%d): " fmt, logging_level2str(logl), \
				logging_dom2str(logdom), __func__, __LINE__, ## __VA_ARGS__); \
	} while (0)
#else
#define LOG(...)
#endif

void logging_init(void);
void logging_domain_level_set(uint8_t logdom, uint8_t sdom, uint8_t logl);
void logging_logfn(uint8_t logdom, uint8_t logl, uint8_t sdom, const char *fmt, ...);
const char *logging_dom2str(uint8_t logdom);
const char *logging_level2str(uint8_t logl);
void logging_hexdump(uint8_t *data, unsigned int len);

#endif
/** @} */
/** @} */

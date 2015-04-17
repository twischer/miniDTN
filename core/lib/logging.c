/*
 * Copyright (c) 2012, Daniel Willmann
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
 * @(#)$Id:$
 */

/**
 * \addtogroup logging
 * @{
 */

/**
 * \file
 *         Implementation of the logging subsystem
 * \author
 *         Daniel Willmann <daniel@totalueberwachung.de>
 * 
 */


#include "logging.h"
#include "list.h"
#include "contiki-conf.h"
#include <string.h>
#include <stdarg.h>

#ifndef LOGL_DEFAULT
#define LOGL_DEFAULT LOGL_WRN
#endif

struct log_cfg log_d[LOGD_NUM];

static char *logdomains[LOGD_NUM] = {
	[LOGD_CORE] = "COR",   /* Contiki core */
	[LOGD_CPU]  = "CPU",   /* CPU */
	[LOGD_INGA] = "PLA",   /* Platform */
	[LOGD_DTN]  = "DTN",   /* uDTN */
	[LOGD_APP]  = "APP",   /* Application */
};

static char *loglevels[LOGL_NUM] = {
	[LOGL_DBG] = "DBG",
	[LOGL_INF] = "INF",
	[LOGL_WRN] = "WRN",
	[LOGL_ERR] = "ERR",
	[LOGL_CRI] = "CRI",
};

struct log_cfg log_d[LOGD_NUM];
static uint8_t inited = 0;

/*---------------------------------------------------------------------------*/
/**
 * \brief      Convert the log domain symbol to a string
 * \param      logdom the log domain
 * \return     String of the log domain
 * \author     Daniel Willmann
 *
 *             The function returns a three letter acronym
 *             used in the log messages to identify the
 *             different log domains.
 *
 */
const char *logging_dom2str(uint8_t logdom)
{
	if (logdom >= LOGD_NUM)
		return "UNK";
	return logdomains[logdom];
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Convert the log level symbol to a string
 * \param      logl the log level
 * \return     String of the log level
 * \author     Daniel Willmann
 *
 *             The function returns a three letter acronym
 *             used in the log messages to identify the
 *             different log levels.
 *
 */
const char *logging_level2str(uint8_t logl)
{
	if (logl >= LOGL_NUM)
		return "UNK";
	return loglevels[logl];
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Initialize the logging subsystem
 * \author     Daniel Willmann
 *
 *             This function initializes the logging subsystem and
 *             should be called before any other function from the
 *             module.
 *
 */
void logging_init(void)
{
	int i, j;
	if (inited)
		return;

	for(i=0; i<LOGD_NUM; i++) {
		for(j=0;j<SUBDOMS;j++) {
			log_d[i].subl[j] = LOGL_DEFAULT;
		}
		log_d[i].subdom_next = 1;
	}
	inited = 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Set the verbosity of the logging domain
 * \param      logdom the domain for which to set the verbosity
 * \param      sdom the subdomain
 * \param      logl the minimal severity of the mesages that are
 *             logged
 * \author     Daniel Willmann
 *
 *             This function sets the loglevel above which messages
 *             are logged (printed). Possible values are in order:
 *             * LOGL_DBG - Debug messages. Very verbose output
 *             * LOGL_INF - Informational messages. General information
 *               that might be useful
 *             * LOGL_WRN - Warnings. Abnormal program conditions that
 *               do not impair normal operation of the program
 *             * LOGL_ERR - Errors. The program can recover from these,
 *               but some functionality is impaired
 *             * LOGL_CRI - Critical errors. The program usually cannot
 *               recover from these
 *
 */
void logging_domain_level_set(uint8_t logdom, uint8_t sdom, uint8_t logl)
{
	if ((logdom >= LOGD_NUM)||(logl >= LOGL_NUM)||(sdom >= SUBDOMS))
		return;

	log_d[logdom].subl[sdom] = logl;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Log a message
 * \param      logdom the logdomain of the message
 * \param      sdom the subdomain
 * \param      logl the loglevel of the mesage
 * \param      fmt the format string of the message
 * \param      ... any parameters that are needed for the format string
 * \author     Daniel Willmann
 *
 *             This is the actual logging function that decides whether
 *             the message should be printed or not. Do not use this
 *             function, instead use the LOGGING_LOG macro as this
 *             includes source file and line number inside the message.
 *
 */
void logging_logfn(uint8_t logdom, uint8_t sdom, uint8_t logl, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	if (logdom >= LOGD_NUM) {
		printf("Unknown log domain %d (", logdom);
		vprintf(fmt, ap);
		printf(")\r\n");
		va_end(ap);
		return;
	}

	if (logl >= LOGL_NUM)
		logl = LOGL_ERR;
	if (sdom >= SUBDOMS)
		sdom = 0;
	if (logl >= log_d[logdom].subl[sdom]) {
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end(ap);
}

/*---------------------------------------------------------------------------*/

void logging_hexdump(uint8_t *data, unsigned int len)
{
	unsigned int i;

	for (i=0; i < len; i++) {
		if (i % 16 == 0)
			printf("\n%02X: ", i);

		printf(" %02X", data[i]);
	}
	printf("\n");
}

/** @} */

/*
 * dtntp.h
 *
 * Copyright (c) 2013, Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>.
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
 */

#ifndef DTNTP_H_
#define DTNTP_H_

#include "contiki.h"
#include "net/uDTN/system_clock.h"

/* service tag name for the IPND */
#define DTNTP_SERVICE_TAG "dtntp"

/* contiki process for DT-NTP */
PROCESS_NAME(dtntp_process);

/* Event thrown to the discovery agent by network layer */
process_event_t dtntp_trigger_sync_event;

/* type to define a clock rating */
typedef float dtntp_rating_t;

/**
 * initialize the DT-NTP module
 */
int dtntp_init(void);

/**
 * Hand-over discovered data of the IPND to the DT-NTP module
 */
void dtntp_discovery_parse_service(uint32_t eid, uint8_t * name, uint8_t name_length, uint8_t *data, uint8_t length);

/**
 * Add IPND service block to the ipnd buffer
 */
int dtntp_discovery_add_service(uint8_t * ipnd_buffer, int length, int * offset);

/**
 * Calculates the current clock rating
 */
dtntp_rating_t dtntp_get_rating();

/**
 * Get the number of seconds since the last synchronization
 */
float dtntp_timepast();

/**
 * Convert a timeval into a float
 */
float dtntp_tvtof(udtn_timeval_t *tv);

#endif /* DTNTP_H_ */

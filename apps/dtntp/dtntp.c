/*
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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "contiki.h"
#include "process.h"
#include "logging.h"

#include "dtn_apps.h"
#include "net/uDTN/api.h"
#include "net/uDTN/agent.h"
#include "net/uDTN/sdnv.h"
#include "net/uDTN/system_clock.h"

#include "dtntp.h"

#define DTN_DTNTP_ENDPOINT	60
#define DTN_DTNTP_PSI 0.99f
#define DTN_DTNTP_SIGMA 1.01f
#define DTN_DTNTP_THRESHOLD 0.15f

// local clock rating of the last sync
static struct dtntp_rating {
	dtntp_rating_t value;
	float sigma;
	udtn_timeval_t last_update;
} dtntp_clock_rating;

// if synchronization is initiated, the peer to sync
// with is stored here
static struct dtntp_peer_data {
	uint32_t id;
	uint8_t version;
	float rating;
	uint32_t timestamp;
} dtntp_ongoing_peer;


void dtntp_peerclear(struct dtntp_peer_data * peer) {
	peer->id = 0;
	peer->version = 0;
	peer->rating = 0;
	peer->timestamp = 0;
}

/*---------------------------------------------------------------------------*/
PROCESS(dtntp_process, "DT-NTP");
static struct registration_api reg;
/*---------------------------------------------------------------------------*/
int dtntp_init(void) {
	// initialize the clock rating to zero
	dtntp_clock_rating.value = 0.0f;
	dtntp_clock_rating.sigma = DTN_DTNTP_SIGMA;
	udtn_timerclear(&dtntp_clock_rating.last_update);

	// clear peer values
	dtntp_peerclear(&dtntp_ongoing_peer);

	process_start(&dtntp_process, NULL);
	return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(dtntp_process, ev, data)
{
	udtn_timeval_t tv;

	float t_stable, time_diff, time_peer, time_local;

	PROCESS_BEGIN();

	// initialize DT-NTP
	dtntp_init();

	/* Give agent time to initialize */
	PROCESS_PAUSE();

	/* Register ping endpoint */
	reg.status = APP_ACTIVE;
	reg.application_process = PROCESS_CURRENT();
	reg.app_id = DTN_DTNTP_ENDPOINT;
	process_post(&agent_process, dtn_application_registration_event,&reg);

	// report DT-NTP capability
	LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "DT-NTP enabled");

	while (1) {
		PROCESS_WAIT_EVENT_UNTIL(ev == dtntp_trigger_sync_event);

		// synchronization triggered
		if (ev == dtntp_trigger_sync_event) {
			// print received timestamp
			LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "DT-NTP: sync with ipn:%lu, timestamp: %lu", (unsigned long)dtntp_ongoing_peer.id, (unsigned long)dtntp_ongoing_peer.timestamp);

			// adjust sigma if we sync'd at least twice
			if (udtn_timerisset(&dtntp_clock_rating.last_update)) {
				t_stable = dtntp_timepast();

				if (t_stable > 0.0f) {
					time_peer = (float)dtntp_ongoing_peer.timestamp;
					time_local = (float)udtn_time(NULL);

					// get right difference between the clocks
					if (time_peer > time_local) 		time_diff = time_peer - time_local;
					else if (time_peer < time_local)	time_diff = time_local - time_peer;
					else								time_diff = 0.0f;

					dtntp_clock_rating.sigma = (1 / pow(DTN_DTNTP_PSI, 1/t_stable)) + \
							+ (time_diff / t_stable * dtntp_ongoing_peer.rating);
				}
			}

			// set correct clock
			udtn_timerclear(&tv);
			tv.tv_sec = dtntp_ongoing_peer.timestamp;
			udtn_settimeofday(&tv);

			// set clock state
			udtn_setclockstate(UDTN_CLOCK_STATE_GOOD);

			// update time of last sync
			udtn_uptime(&dtntp_clock_rating.last_update);

			// set new clock rating
			dtntp_clock_rating.value = dtntp_ongoing_peer.rating * (1.0 / powf(dtntp_clock_rating.sigma, 0.001));

			// clear ongoing values
			dtntp_peerclear(&dtntp_ongoing_peer);
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct dtn_app dtntp = {
		.name = "DT-NTP",
		.init = dtntp_init,
		.parse_ipnd_service_block = dtntp_discovery_parse_service,
		.add_ipnd_service_block = dtntp_discovery_add_service
};
/*---------------------------------------------------------------------------*/

int dtntp_discovery_add_service(uint8_t * ipnd_buffer, int length, int * offset) {
	char string_buffer[60];
	int len;
	udtn_timeval_t tv;
	float rating;

	// get local clock rating
	rating = dtntp_get_rating();

	// do not send sync beacons if clock rating is zero
	if (rating == 0.0) return 0;

	// get local time
	udtn_gettimeofday(&tv);

	// do not send sync beacons if the clock timestamp is wrong
	if (tv.tv_sec < UDTN_CLOCK_DTN_EPOCH_OFFSET) return 0;

	// Convert timestamp into DTN time
	tv.tv_sec -= UDTN_CLOCK_DTN_EPOCH_OFFSET;

	len = sprintf(string_buffer, DTNTP_SERVICE_TAG);
	(*offset) += sdnv_encode(len, ipnd_buffer + (*offset), length - (*offset));
	memcpy(ipnd_buffer + (*offset), string_buffer, len);
	(*offset) += len;

	if (rating < 1.0f) {
		len = sprintf(string_buffer, "version=2;quality=0.%lu;timestamp=%li;", \
				(unsigned long)(rating * 1000000), tv.tv_sec);
	}
	else {
		len = sprintf(string_buffer, "version=2;quality=1.0;timestamp=%li;", \
				tv.tv_sec);
	}

	(*offset) += sdnv_encode(len, ipnd_buffer + (*offset), length - (*offset));
	memcpy(ipnd_buffer + (*offset), string_buffer, len);
	(*offset) += len;

	return 1;
}

void dtntp_discovery_parse_service(uint32_t eid, uint8_t * tag_buf, uint8_t tag_len, uint8_t *data, uint8_t length) {
	char * pch;

	// if the service tag is not equal to DISCOVERY_IPND_SERVICE_DTNTP, skip this block
	if (strncmp((char*)tag_buf, DTNTP_SERVICE_TAG, tag_len) != 0) return;

	if (dtntp_ongoing_peer.id == 0) {
		// set peer id
		dtntp_ongoing_peer.id = eid;

		pch = strtok((char*)data, "=;");

		while ((pch != NULL) && (pch < ((char*)data + length))) {
			if (strcmp(pch, "version") == 0) {
				// get version number
				pch = strtok(NULL, ";");
				dtntp_ongoing_peer.version = atoi(pch);
			}
			else if (strcmp(pch, "quality") == 0) {
				// get version number
				pch = strtok(NULL, ";");
				dtntp_ongoing_peer.rating = atof(pch);
			}
			else if (strcmp(pch, "timestamp") == 0) {
				// get version number
				pch = strtok(NULL, ";");
				dtntp_ongoing_peer.timestamp = atol(pch) + UDTN_CLOCK_DTN_EPOCH_OFFSET;
			}

			pch = strtok(NULL, "=");
		}

		/* check version number (1 = DT-NTP v1, 2 = DT-NTP light) */
		if( dtntp_ongoing_peer.version != 1 && dtntp_ongoing_peer.version != 2 ) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Wrong version %iu", (unsigned int)dtntp_ongoing_peer.version);

			// clear peer values
			dtntp_peerclear(&dtntp_ongoing_peer);
			return;
		}

		// do not sync if the timestamps are equal in seconds
		if (dtntp_ongoing_peer.timestamp == udtn_time(NULL)) {
			// clear peer values
			dtntp_peerclear(&dtntp_ongoing_peer);
			return;
		}

		// do not sync if the quality is worse than ours
		if ((dtntp_ongoing_peer.rating * (1-DTN_DTNTP_THRESHOLD)) <= dtntp_get_rating()) {
			// clear peer values
			dtntp_peerclear(&dtntp_ongoing_peer);
			return;
		}

		// generate an event to sync with this peer
		process_post(&dtntp_process, dtntp_trigger_sync_event, NULL);
	}
}

float dtntp_timepast() {
	udtn_timeval_t uptime, time_diff;

	// get the uptime of the node
	udtn_uptime(&uptime);

	// get the seconds since the last sync
	udtn_timersub(&uptime, &dtntp_clock_rating.last_update, &time_diff);

	// return the time past since the last sync
	return dtntp_tvtof(&time_diff);
}

float dtntp_tvtof(udtn_timeval_t *tv) {
	return (float)tv->tv_sec + ((float)tv->tv_usec / 1000000.0);
}

dtntp_rating_t dtntp_get_rating() {
	return dtntp_clock_rating.value * (1.0 / powf(dtntp_clock_rating.sigma, dtntp_timepast()));
}

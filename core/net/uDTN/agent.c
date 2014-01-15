/**
 * \addtogroup bprocess
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Daniel Willmann <daniel@totalueberwachung.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "timer.h"
#include "net/netstack.h"
#include "net/rime/rimeaddr.h"
#include "mmem.h"
#include "lib/memb.h"
#include "logging.h"
#include "node-id.h"

#include "api.h"
#include "registration.h"
#include "bundle.h"
#include "storage.h"
#include "sdnv.h"
#include "redundancy.h"
#include "dispatching.h"
#include "routing.h"
#include "dtn_network.h"
#include "custody.h"
#include "discovery.h"
#include "discovery_scheduler.h"
#include "statistics.h"
#include "convergence_layer.h"
#include "hash.h"
#include "system_clock.h"
#include "dtn_apps.h"

#include "agent.h"

uint32_t dtn_node_id;
uint32_t dtn_seq_nr;
uint32_t dtn_seq_nr_ab;
uint32_t dtn_last_time_stamp;

PROCESS(agent_process, "AGENT process");
AUTOSTART_PROCESSES(&agent_process);

void agent_init(void) {
	// if the agent process is already running, to nothing
	if( process_is_running(&agent_process) ) {
		return;
	}

	// Otherwise start the agent process
	process_start(&agent_process, NULL);
}

/*  Bundle Protocol Prozess */
PROCESS_THREAD(agent_process, ev, data)
{
	uint32_t * bundle_number_ptr = NULL;
	struct registration_api * reg;
	udtn_timeval_t tv;
	uint32_t tmp = 0;

	PROCESS_BEGIN();

	/* We obtain our dtn_node_id from the RIME address of the node */
	dtn_node_id = convert_rime_to_eid(&rimeaddr_node_addr);
	dtn_seq_nr = 0;
	dtn_seq_nr_ab = 0;
	dtn_last_time_stamp = 0;

	/* We are initialized quite early - give Contiki some time to do its stuff */
	process_poll(&agent_process);
	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

	mmem_init();
	udtn_clock_init();
	convergence_layer_init();
	BUNDLE_STORAGE.init();
	REDUNDANCE.init();
	CUSTODY.init();
	ROUTING.init();
	DISCOVERY_SCHEDULER.init();
	DISCOVERY.init();
	registration_init();

	dtn_application_remove_event  = process_alloc_event();
	dtn_application_registration_event = process_alloc_event();
	dtn_application_status_event = process_alloc_event();
	dtn_receive_bundle_event = process_alloc_event();
	dtn_send_bundle_event = process_alloc_event();
	submit_data_to_application_event = process_alloc_event();
	dtn_beacon_event = process_alloc_event();
	dtn_send_admin_record_event = process_alloc_event();
	dtn_bundle_in_storage_event = process_alloc_event();
	dtn_send_bundle_to_node_event = process_alloc_event();
	dtn_processing_finished = process_alloc_event();
	dtn_bundle_stored = process_alloc_event();

	// We use printf here, to make this message visible in every case!
	printf("Starting DTN Bundle Protocol Agent with EID ipn:%lu\n", dtn_node_id);

	// Start all dtn apps
	dtn_apps_start();

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev);

		if(ev == dtn_application_registration_event) {
			reg = (struct registration_api *) data;

			registration_new_application(reg->app_id, reg->application_process, reg->node_id);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "New Service registration for endpoint %lu", reg->app_id);
			continue;
		}

		if(ev == dtn_application_status_event) {
			int status;
			reg = (struct registration_api *) data;
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Service switching status to %i", reg->status);
			if(reg->status == APP_ACTIVE)
				status = registration_set_active(reg->app_id, reg->node_id);
			else if(reg->status == APP_PASSIVE)
				status = registration_set_passive(reg->app_id, reg->node_id);

			if(status == -1) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "no registration found to switch");
			}

			continue;
		}

		if(ev == dtn_application_remove_event) {
			reg = (struct registration_api *) data;
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Unregistering service for endpoint %lu", reg->app_id);
			registration_remove_application(reg->app_id, reg->node_id);
			continue;
		}

		if(ev == dtn_send_bundle_event) {
			uint8_t n = 0;
			struct bundle_t * bundle = NULL;
			struct process * source_process = NULL;
			struct mmem * bundleptr;
			uint32_t bundle_flags = 0;

			bundleptr = (struct mmem *) data;
			if( bundleptr == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid pointer");
				process_post(source_process, dtn_bundle_store_failed, NULL);
				continue;
			}

			bundle = (struct bundle_t *) MMEM_PTR(bundleptr);
			if( bundle == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid MMEM structure");
				process_post(source_process, dtn_bundle_store_failed, NULL);
				continue;
			}

			/* Go and find the process from which the bundle has been sent */
			uint32_t app_id = registration_get_application_id(bundle->source_process);
			if( app_id == REGISTRATION_EID_UNDEFINED  && bundle->source_process != &agent_process) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "Unregistered process %s tries to send a bundle", PROCESS_NAME_STRING(bundle->source_process));
				process_post(source_process, dtn_bundle_store_failed, NULL);
				bundle_decrement(bundleptr);
				continue;
			}

			/* Find out, if the source process has set an app id */
			uint32_t service_app_id;
			bundle_get_attr(bundleptr, SRC_SERV, &service_app_id);

			/* If the service did not set an app id, do it now */
			if( service_app_id == 0 ) {
				bundle_set_attr(bundleptr, SRC_SERV, &app_id);
			}

			/* Set the source node */
			bundle_set_attr(bundleptr, SRC_NODE, &dtn_node_id);

			/* Check for report-to and set node and service accordingly */
			bundle_get_attr(bundleptr, FLAGS, &bundle_flags);
			if( bundle_flags & BUNDLE_FLAG_REPORT ) {
				uint32_t report_to_node = 0;
				bundle_get_attr(bundleptr, REP_NODE, &report_to_node);

				if( report_to_node == 0 ) {
					bundle_set_attr(bundleptr, REP_NODE, &dtn_node_id);
				}

				uint32_t report_to_service = 0;
				bundle_get_attr(bundleptr, REP_SERV, &report_to_service);

				if( report_to_service ) {
					bundle_set_attr(bundleptr, REP_SERV, &app_id);
				}
			}

			// Set time-stamp if clock state is at least "good"
			if (udtn_getclockstate() >= UDTN_CLOCK_STATE_GOOD) {
				// Get the global time-stamp
				udtn_gettimeofday(&tv);

				// Set the time-stamp in the bundle
				// FIXME: uint32_t is too small after year 2030 ;)
				tmp = tv.tv_sec - UDTN_CLOCK_DTN_EPOCH_OFFSET;
				bundle_set_attr(bundleptr, TIME_STAMP, &tmp);

				// Reset sequence number if time-stamp has changed since the last call
				if (dtn_last_time_stamp != tv.tv_sec) {
					dtn_seq_nr = 0;
					dtn_last_time_stamp = tv.tv_sec;
				}

				LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_send_bundle_event(%p) with seqNo %lu", bundleptr, dtn_seq_nr);

				// Set the outgoing sequence number
				bundle_set_attr(bundleptr, TIME_STAMP_SEQ_NR, &dtn_seq_nr);
				dtn_seq_nr++;
			} else {
				// clock state is not sufficient
				// use age block approach and leave time-stamp set to 0
				LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_send_bundle_event(%p) with seqNo %lu", bundleptr, dtn_seq_nr_ab);

				// Set the outgoing sequence number
				bundle_set_attr(bundleptr, TIME_STAMP_SEQ_NR, &dtn_seq_nr_ab);
				dtn_seq_nr_ab++;
			}

			// Copy the sending process, because 'bundle' will not be accessible anymore afterwards
			source_process = bundle->source_process;

			// Calculate the bundle number
			bundle->bundle_num = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->frag_offs, bundle->app_len);

			// Save the bundle in storage
			n = BUNDLE_STORAGE.save_bundle(bundleptr, &bundle_number_ptr);

			// Reset our pointers to avoid using invalid ones
			bundle = NULL;
			bundleptr = NULL;

			// Notify the sender process
			if( n ) {
				/* Bundle has been successfully saved, send event to service */
				process_post(source_process, dtn_bundle_stored, NULL);
			} else {
				/* Bundle could not be saved, notify service */
				process_post(source_process, dtn_bundle_store_failed, NULL);
			}

			// Now emulate the event to our agent
			if( n ) {
				data = (void *) bundle_number_ptr;
				ev = dtn_bundle_in_storage_event;
			} else {
				continue;
			}
		}

		if(ev == dtn_send_admin_record_event) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "Send admin record currently not implemented");
			continue;
		}

		if(ev == dtn_beacon_event){
			rimeaddr_t* src =(rimeaddr_t*) data;
			ROUTING.new_neighbor(src);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_beacon_event for %u.%u", src->u8[0], src->u8[1]);
			continue;
		}

		if(ev == dtn_bundle_in_storage_event){
			uint32_t * bundle_number = (uint32_t *) data;

			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle %lu in storage", *bundle_number);

			if(ROUTING.new_bundle(bundle_number) < 0){
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "routing reports error when announcing new bundle %lu", *bundle_number);
				continue;
			}

			continue;
		}

		if(ev == dtn_processing_finished) {
			// data should contain the bundlemem ptr
			struct mmem * bundlemem = NULL;
			struct bundle_t * bundle = NULL;

			bundlemem = (struct mmem *) data;
			if( bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_processing_finished with invalid pointer");
				continue;
			}

			bundle = (struct bundle_t *) MMEM_PTR(bundlemem);
			if( bundle == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid MMEM structure");
				continue;
			}

			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "service has processed bundle %lu", bundle->bundle_num);

			// Notify routing, that service has finished processing a bundle
			ROUTING.locally_delivered(bundlemem);

			continue;
		}
	}

	PROCESS_END();
}

uint32_t agent_get_sequence_number()
{
	return dtn_seq_nr_ab;
}

void agent_set_sequence_number(uint32_t seqno)
{
	dtn_seq_nr_ab = seqno;
}

void agent_delete_bundle(uint32_t bundle_number){
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Agent deleting bundle no %lu", bundle_number);

	convergence_layer_delete_bundle(bundle_number);
	ROUTING.del_bundle(bundle_number);
	CUSTODY.del_from_list(bundle_number);
}
/** @} */

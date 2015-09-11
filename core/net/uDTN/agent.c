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

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "net/netstack.h"
#include "net/linkaddr.h"
#include "lib/mmem.h"
#include "lib/memb.h"
#include "lib/logging.h"

#include "api.h"
#include "dtn_process.h"
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
static QueueHandle_t event_queue = NULL;

void agent_process(void* p);

bool agent_init(void)
{
	// if the agent process is already running, to nothing
	static bool is_running = false;
	if(is_running) {
		return true;
	}

	// Otherwise start the agent process
	if ( !dtn_process_create_with_queue(agent_process, "AGENT process", 0x100, &event_queue) ) {
		return false;
	}

	is_running = true;
	return true;
}

/*  Bundle Protocol Prozess */
void agent_process(void* p)
{
	uint32_t * bundle_number_ptr = NULL;
	udtn_timeval_t tv;
	uint32_t tmp = 0;

//	PROCESS_BEGIN();

	/* We obtain our dtn_node_id from the RIME address of the node */
	dtn_node_id = convert_rime_to_eid(&linkaddr_node_addr);
	dtn_seq_nr = 0;
	dtn_seq_nr_ab = 0;
	dtn_last_time_stamp = 0;

	/* We are initialized quite early - give Contiki some time to do its stuff */
//	process_poll(&agent_process);
//	PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

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

	// We use printf here, to make this message visible in every case!
	printf("Starting DTN Bundle Protocol Agent with EID ipn:%lu\n", dtn_node_id);

	// Start all dtn apps
	dtn_apps_start();

	while(1) {
		event_container_t ev;
		const BaseType_t event_received = xQueueReceive(event_queue, &ev, portMAX_DELAY);
		if (!event_received) {
			/* timeout has expired, wait again for next event */
			continue;
		}

		if(ev.event == dtn_application_registration_event) {
			registration_new_application(ev.registration->app_id, ev.registration->event_queue, ev.registration->node_id);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_INF, "New Service registration for endpoint %lu", ev.registration->app_id);
			continue;
		}

		if(ev.event == dtn_application_status_event) {
			int status = -2;
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Service switching status to %i", ev.registration->status);
			if(ev.registration->status == APP_ACTIVE)
				status = registration_set_active(ev.registration->app_id, ev.registration->node_id);
			else if(ev.registration->status == APP_PASSIVE)
				status = registration_set_passive(ev.registration->app_id, ev.registration->node_id);

			if(status == -1) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "no registration found to switch");
			} else if(status == -2) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "Desired Regirstation Status is invalid");
			}

			continue;
		}

		if(ev.event == dtn_application_remove_event) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Unregistering service for endpoint %lu", ev.registration->app_id);
			registration_remove_application(ev.registration->app_id, ev.registration->node_id);
			continue;
		}

		if(ev.event == dtn_send_bundle_event) {
			uint8_t n = 0;
			struct bundle_t * bundle = NULL;
//			struct process * source_process = NULL;
			uint32_t bundle_flags = 0;
			uint32_t payload_length = 0;
			if( ev.bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid pointer");
				// TODO send error to all processes
//				process_post(source_process, dtn_bundle_store_failed, NULL);
				continue;
			}

			bundle = (struct bundle_t *) MMEM_PTR(ev.bundlemem);
			if( bundle == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid MMEM structure");
				// TODO send error to all processes
//				process_post(source_process, dtn_bundle_store_failed, NULL);
				continue;
			}

			// TODO check if the queue was registered for dtn
//			/* Go and find the process from which the bundle has been sent */
			const uint32_t app_id = registration_get_application_id(bundle->source_event_queue);
//			if( app_id == REGISTRATION_EID_UNDEFINED  && bundle->source_event_queue != event_queue) {
//				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "Unregistered process %s tries to send a bundle", PROCESS_NAME_STRING(bundle->source_event_queue));
//				// TODO send error to all processes
//				process_post(source_process, dtn_bundle_store_failed, NULL);
//				bundle_decrement(ev.bundlemem);
//				continue;
//			}

			/* Find out, if the source process has set an app id */
			uint32_t service_app_id;
			bundle_get_attr(ev.bundlemem, SRC_SERV, &service_app_id);

			/* If the service did not set an app id, do it now */
			if( service_app_id == 0 ) {
				bundle_set_attr(ev.bundlemem, SRC_SERV, &app_id);
			}

			/* Set the source node */
			bundle_set_attr(ev.bundlemem, SRC_NODE, &dtn_node_id);

			/* Check for report-to and set node and service accordingly */
			bundle_get_attr(ev.bundlemem, FLAGS, &bundle_flags);
			if( bundle_flags & BUNDLE_FLAG_REPORT ) {
				uint32_t report_to_node = 0;
				bundle_get_attr(ev.bundlemem, REP_NODE, &report_to_node);

				if( report_to_node == 0 ) {
					bundle_set_attr(ev.bundlemem, REP_NODE, &dtn_node_id);
				}

				uint32_t report_to_service = 0;
				bundle_get_attr(ev.bundlemem, REP_SERV, &report_to_service);

				if( report_to_service ) {
					bundle_set_attr(ev.bundlemem, REP_SERV, &app_id);
				}
			}

			// Set time-stamp if clock state is at least "good"
			if (udtn_getclockstate() >= UDTN_CLOCK_STATE_GOOD) {
				// Get the global time-stamp
				udtn_gettimeofday(&tv);

				// Set the time-stamp in the bundle
				// FIXME: uint32_t is too small after year 2030 ;)
				tmp = tv.tv_sec - UDTN_CLOCK_DTN_EPOCH_OFFSET;
				bundle_set_attr(ev.bundlemem, TIME_STAMP, &tmp);

				// Reset sequence number if time-stamp has changed since the last call
				if (dtn_last_time_stamp != tv.tv_sec) {
					dtn_seq_nr = 0;
					dtn_last_time_stamp = tv.tv_sec;
				}

				LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_send_bundle_event(%p) with seqNo %lu with timestamp %lu", ev.bundlemem, dtn_seq_nr, tv.tv_sec);

				// Set the outgoing sequence number
				bundle_set_attr(ev.bundlemem, TIME_STAMP_SEQ_NR, &dtn_seq_nr);
				dtn_seq_nr++;
			} else {
				// clock state is not sufficient
				// use age block approach and leave time-stamp set to 0
				LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_send_bundle_event(%p) with seqNo %lu without timestamp", ev.bundlemem, dtn_seq_nr_ab);

				// Set the outgoing sequence number
				bundle_set_attr(ev.bundlemem, TIME_STAMP_SEQ_NR, &dtn_seq_nr_ab);
				dtn_seq_nr_ab++;
			}

			// Copy the sending process, because 'bundle' will not be accessible anymore afterwards
//			source_process = bundle->source_event_queue;
			const QueueHandle_t source_queue = bundle->source_event_queue;

			// To uniquely identify fragments, we need the length of the payload block
			if( bundle->flags & BUNDLE_FLAG_FRAGMENT ) {
				struct bundle_block_t * payload_block = bundle_get_payload_block(ev.bundlemem);
				payload_length = payload_block->block_size;
			}

			// Calculate the bundle number
			bundle->bundle_num = HASH.hash_convenience(bundle->tstamp_seq, bundle->tstamp, bundle->src_node, bundle->src_srv, bundle->frag_offs, payload_length);

			// Save the bundle in storage
			n = BUNDLE_STORAGE.save_bundle(ev.bundlemem, &bundle_number_ptr);

			// Reset our pointers to avoid using invalid ones
			bundle = NULL;
			ev.bundlemem = NULL;

			// Notify the sender process
			if( n ) {
				/* Bundle has been successfully saved, send event to service */
//				process_post(source_process, dtn_bundle_stored, NULL);
				const event_container_t event = {
					.event = dtn_bundle_stored,
				};
				if ( !xQueueSend(source_queue, &event, 0) ) {
					LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Could not add event to queue of the source process! (app_id %d, queue %p)",
						app_id, source_queue);
				}
			} else {
				/* Bundle could not be saved, notify service */
//				process_post(source_process, dtn_bundle_store_failed, NULL);
				const event_container_t event = {
					.event = dtn_bundle_store_failed,
				};
				if ( !xQueueSend(source_queue, &event, 0) ) {
					LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Could not add event to queue of the source process!");
				}
			}

			// Now emulate the event to our agent
			if( n ) {
				// TODO
				ev.bundle_number_ptr = bundle_number_ptr;
				ev.event = dtn_bundle_in_storage_event;
			} else {
				continue;
			}
		}

		if(ev.event == dtn_send_admin_record_event) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "Send admin record currently not implemented");
			continue;
		}

		if(ev.event == dtn_beacon_event) {
			ROUTING.new_neighbor(ev.linkaddr);
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "dtn_beacon_event for %u.%u", ev.linkaddr->u8[0], ev.linkaddr->u8[1]);
			continue;
		}

		if(ev.event == dtn_bundle_in_storage_event) {
			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "bundle %lu in storage", *ev.bundle_number_ptr);

			if(ROUTING.new_bundle(ev.bundle_number_ptr) < 0){
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "routing reports error when announcing new bundle %lu", *ev.bundle_number_ptr);
				continue;
			}

			continue;
		}

		if(ev.event == dtn_processing_finished) {
			// data should contain the bundlemem ptr
			struct bundle_t * bundle = NULL;

			if( ev.bundlemem == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_processing_finished with invalid pointer");
				continue;
			}

			bundle = (struct bundle_t *) MMEM_PTR(ev.bundlemem);
			if( bundle == NULL ) {
				LOG(LOGD_DTN, LOG_AGENT, LOGL_ERR, "dtn_send_bundle_event with invalid MMEM structure");
				continue;
			}

			LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "service has processed bundle %lu", bundle->bundle_num);

			// Notify routing, that service has finished processing a bundle
			ROUTING.locally_delivered(ev.bundlemem);

			continue;
		}
	}
}

uint32_t agent_get_sequence_number()
{
	return dtn_seq_nr_ab;
}

void agent_set_sequence_number(uint32_t seqno)
{
	dtn_seq_nr_ab = seqno;
}

void agent_set_bundle_source(struct bundle_t* const bundle)
{
	bundle->source_event_queue = event_queue;
}

void agent_delete_bundle(uint32_t bundle_number){
	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Agent deleting bundle no %lu", bundle_number);

	convergence_layer_delete_bundle(bundle_number);
	ROUTING.del_bundle(bundle_number);
	CUSTODY.del_from_list(bundle_number);
}

void agent_send_event(const event_container_t* const event)
{
	if ( !xQueueSend(event_queue, event, 0) ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Could not add event to queue of the dtn agent!");
	}
}

/** @} */

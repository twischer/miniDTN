/** 
 * \addtogroup bundle_status
 * @{
 */

/**
 * \defgroup bundle_status_basic Basic status report module
 * @{
 */

/**
 * \file
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Pšttner <poettner@ibr.cs.tu-bs.de>
 */

#include <string.h>

#include "mmem.h"
#include "logging.h"

#include "bundle.h"
#include "agent.h"
#include "storage.h"
#include "sdnv.h"
#include "api.h"
#include "administrative_record.h"
#include "eid.h"

#include "statusreport.h"

#define BUFFER_LENGTH 100

/**
 * \brief Encode a bundle status report from the struct to a flat buffer
 */
int statusreport_encode(status_report_t * report, uint8_t * buffer, uint8_t length)
{
	uint8_t offset = 0;
	int ret = 0;

	// Encode that we have an status record
	buffer[offset++] = TYPE_CODE_BUNDLE_STATUS_REPORT;

	// Store the 8 bit fields for status flags and reason code
	buffer[offset++] = report->status_flags;
	buffer[offset++] = report->reason_code;

	/* For responses to fragments */
	if( report->fragment ) {
		// Set the header flag
		buffer[0] |= ADMIN_FLAGS_IS_FOR_FRAGMENT;

		/* Fragment offset */
		ret = sdnv_encode(report->dtn_time_seconds, &buffer[offset], length - offset);
		if (ret < 0)
			return -1;
		offset += ret;

		/* Fragment length */
		ret = sdnv_encode(report->dtn_time_seconds, &buffer[offset], length - offset);
		if (ret < 0)
			return -1;
		offset += ret;
	}

	/**
	 * TODO: We assume here, that all status reports have a timestamp. This is true in
	 * the latest version of RFC5050 but is not necessarily true in the future
	 */

	/* DTN Time Seconds */
	ret = sdnv_encode(report->dtn_time_seconds, &buffer[offset], length - offset);
	if (ret < 0)
		return -1;
	offset += ret;

	/* DTN Time Nanoseconds */
	ret = sdnv_encode(report->dtn_time_nanoseconds, &buffer[offset], length - offset);
	if (ret < 0)
		return -1;
	offset += ret;

	/* Now we need the creation timestamp of the original bundle */
	ret = sdnv_encode(report->bundle_creation_timestamp, &buffer[offset], length - offset);
	if (ret < 0)
		return -1;
	offset += ret;

	/* And the sequence number of the original bundle */
	ret = sdnv_encode(report->bundle_sequence_number, &buffer[offset], length - offset);
	if (ret < 0)
		return -1;
	offset += ret;

	/* Now encode the original source EID in string format */
	offset += eid_create_full_length(report->source_eid_node, report->source_eid_service, &buffer[offset], length - offset);

	return offset;
}

int statusreport_decode(status_report_t * report, uint8_t * buffer, uint8_t length)
{
	int offset = 0;
	int ret;

	// Check for the proper type
	if( !(buffer[offset] & TYPE_CODE_BUNDLE_STATUS_REPORT) ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Status Report Format mismatch");
		return -1;
	}

	if( buffer[offset++] & ADMIN_FLAGS_IS_FOR_FRAGMENT ) {
		report->fragment = 1;
	}

	// Set report struct to 0
	memset(report, 0, sizeof(status_report_t));

	// Recover status flags and reason code
	report->status_flags = buffer[offset++];
	report->reason_code = buffer[offset++];

	// Recover fragment offset and length (if present)
	if( report->fragment ) {
		/* Fragment offset */
		ret = sdnv_decode(&buffer[offset], length - offset, &report->dtn_time_seconds);
		if( ret < 0 )
			return -1;
		offset += ret;

		/* Fragment length */
		ret = sdnv_decode(&buffer[offset], length - offset, &report->dtn_time_seconds);
		if( ret < 0 )
			return -1;
		offset += ret;
	}

	// FIXME: seconds and nanoseconds could be present 0 - n times. We assume here 1
	/* Recover dtn_time seconds */
	ret = sdnv_decode(&buffer[offset], length - offset, &report->dtn_time_seconds);
	if( ret < 0 )
		return -1;
	offset += ret;

	/* Recover dtn_time nanoseconds */
	ret = sdnv_decode(&buffer[offset], length - offset, &report->dtn_time_nanoseconds);
	if( ret < 0 )
		return -1;
	offset += ret;

	/* Recover timestamp */
	ret = sdnv_decode(&buffer[offset], length - offset, &report->bundle_creation_timestamp);
	if( ret < 0 )
		return -1;
	offset += ret;

	/* Recover timestamp sequence number */
	ret = sdnv_decode(&buffer[offset], length - offset, &report->bundle_sequence_number);
	if( ret < 0 )
		return -1;
	offset += ret;

	/* Recover the EID */
	ret = eid_parse_full_length(&buffer[offset], length - offset, &report->source_eid_node, &report->source_eid_service);
	if( ret < 0 )
		return -1;

	return 1;
}

/**
 * \brief sends a status report for a bundle to the "report-to"-node
 * \param bundle pointer to bundle
 * \param status status code for the bundle
 * \param reason reason code for the status
 */
uint8_t statusreport_basic_send(struct mmem * bundlemem, uint8_t status, uint8_t reason)
{
	uint32_t bundle_flags;
	uint32_t report_node_id;
	uint32_t report_service_id;
	uint32_t flags;
	uint32_t lifetime;
	struct mmem * report_bundle = NULL;
	struct bundle_t * bundle = NULL;
	status_report_t report;
	uint8_t buffer[BUFFER_LENGTH];
	int ret;

	// Check if we really should send a report
	bundle_get_attr(bundlemem, FLAGS, &bundle_flags);
	if( !(bundle_flags & BUNDLE_FLAG_REPORT) ) {
		return 0;
	}

	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Sending out status report for status %u and reason %u", status, reason);

	// Fill out the statusreport struct
	memset(&report, 0, sizeof(status_report_t));
	report.status_flags = status;
	report.reason_code = reason;

	// FIXME: the time stored here is badly broken but we do not have any better solution ATM
	report.dtn_time_seconds = clock_seconds();
	report.dtn_time_nanoseconds = 0;

	// Collect all necessary information to fill out the report
	bundle_get_attr(bundlemem, SRC_NODE, &report.source_eid_node);
	bundle_get_attr(bundlemem, SRC_SERV, &report.source_eid_service);
	bundle_get_attr(bundlemem, TIME_STAMP, &report.bundle_creation_timestamp);
	bundle_get_attr(bundlemem, TIME_STAMP_SEQ_NR, &report.bundle_sequence_number);

	// Collect all necessary information to create report bundle
	bundle_get_attr(bundlemem, REP_NODE, &report_node_id);
	bundle_get_attr(bundlemem, REP_SERV, &report_service_id);

	// Check for a proper destination node
	if( report_node_id == 0 ) {
		LOG(LOGD_DTN, LOG_AGENT, LOGL_WRN, "Cannot send status report, destination node is %lu", report_node_id);
		return 0;
	}

	// Allocate memory for our bundle
	report_bundle = bundle_create_bundle();

	// "Fake" the source of the bundle to be the agent
	// Otherwise we cannot send bundles, because we do not have an endpoint registration
	bundle = (struct bundle_t *) MMEM_PTR(report_bundle);
	bundle->source_process = &agent_process;

	// Set destination addresses for the outgoing bundle
	bundle_set_attr(report_bundle, DEST_NODE, &report_node_id);
	bundle_set_attr(report_bundle, DEST_SERV, &report_service_id);

	// Copy timestamp from incoming bundle
	bundle_set_attr(report_bundle, TIME_STAMP, &report.bundle_creation_timestamp);

	LOG(LOGD_DTN, LOG_AGENT, LOGL_DBG, "Report goes to %lu.%lu", report_node_id, report_service_id);

	// Set lifetime
	lifetime = 3600;
	bundle_set_attr(report_bundle, LIFE_TIME, &lifetime);

	// Make the outgoing bundle an admin record
	flags = BUNDLE_FLAG_ADM_REC;
	bundle_set_attr(report_bundle, FLAGS, &flags);

	// Encode status report
	ret = statusreport_encode(&report, buffer, BUFFER_LENGTH);
	if( ret < 0 )
		return -1;

	// Add status report to bundle
	ret = bundle_add_block(report_bundle, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, buffer, ret);
	if( ret < 0 )
		return -1;

	// Send out the report
	process_post(&agent_process, dtn_send_bundle_event, report_bundle);

	return 1;
}

const struct status_report_driver statusreport_basic = {
		"STATUSREPORT_BASIC",
		statusreport_basic_send,
};
/** @} */
/** @} */

/**
 * \addtogroup discovery 
 * @{
 */

/**
 * \defgroup discovery_ipnd IP-ND discovery module
 *
 * @{
 */

/**
 * \file
 * \brief IP-ND compliant discovery module
 * IP-ND = http://tools.ietf.org/html/draft-irtf-dtnrg-ipnd-01
 *
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include <stdbool.h>
#include <string.h> // for memset

#include "FreeRTOS.h"
#include "timers.h"

#include "net/netstack.h"
#include "net/packetbuf.h" 
#include "net/linkaddr.h"
#include "net/mac/frame802154.h" // for IEEE802154_PANID
#include "lib/logging.h"

#include "dtn_apps.h"
#include "dtn_network.h"
#include "agent.h"
#include "sdnv.h"
#include "statistics.h"
#include "convergence_layers.h"
#include "eid.h"
#include "discovery_scheduler.h"

#include "discovery.h"


#define DISCOVERY_OVER_ETHERNET	1


typedef struct {
	uint16_t sequence_nr;
	uint32_t node_id;
	uint16_t port;
} ipnd_msg_attrs_t;


static int discovery_ipnd_refresh_neighbour(const cl_addr_t* const neighbour);
static void discovery_ipnd_parse_msg(const uint8_t* const payload, const uint8_t length, ipnd_msg_attrs_t* const attrs);
static int discovery_ipnd_save_neighbour(const uint32_t eid, const cl_addr_t* const addr);
static void discovery_ipnd_remove_stale_neighbours(const TimerHandle_t timer);
void discovery_ipnd_print_list();

/*
 * This is a strlen implementation which only can be used
 * with static strings like
 * "Hello"
 * But in this case,
 * it can be calculated during compile time.
 */
#define STATIC_STRLEN(x)			(sizeof(x) - 1)

#define DISCOVERY_NEIGHBOUR_CACHE	3
#define DISCOVERY_NEIGHBOUR_TIMEOUT	(5 * DISCOVERY_CYCLE)
#define DISCOVERY_IPND_SERVICE		"lowpancl"
#define DISCOVERY_IPND_SERVICE_UDP	"dgram:udp"
#define DISCOVERY_IPND_SERVICE_PORT	"port="
#define DISCOVERY_IPND_BUFFER_LEN 	70
#define DISCOVERY_IPND_WHITELIST	0


#define IPND_FLAGS_SOURCE_EID		(1<<0)
#define IPND_FLAGS_SERVICE_BLOCK	(1<<1)
#define IPND_FLAGS_BLOOMFILTER		(1<<2)


/**
 * This "internal" struct extends the discovery_neighbour_list_entry struct with
 * more attributes for internal use
 */
struct discovery_ipnd_neighbour_list_entry {
	struct discovery_ipnd_neighbour_list_entry *next;
	uint8_t addr_type;
	// TODO use uint64_t, because compressed bundle header can have such big addresses
	linkaddr_t neighbour;
	ip_addr_t ip;
	uint16_t port;
	unsigned long timestamp_last_lowpan;
	unsigned long timestamp_last_ip;
	unsigned long timestamp_discovered;
};

/**
 * List and memory blocks to save information about neighbours
 */
LIST(neighbour_list);
MEMB(neighbour_mem, struct discovery_ipnd_neighbour_list_entry, DISCOVERY_NEIGHBOUR_CACHE);

static uint8_t discovery_status = 0;
static TimerHandle_t discovery_timeout_timer = NULL;
uint16_t discovery_sequencenumber = 0;

linkaddr_t discovery_whitelist[DISCOVERY_IPND_WHITELIST];

/**
 * \brief IPND Discovery init function (called by agent)
 */
static bool discovery_ipnd_init()
{
	// Initialize the neighbour list
	list_init(neighbour_list);

	// Initialize the neighbour memory block
	memb_init(&neighbour_mem);

#if DISCOVERY_IPND_WHITELIST > 0
	// Clear the discovery whitelist
	memset(&discovery_whitelist, 0, sizeof(linkaddr_t) * DISCOVERY_IPND_WHITELIST);

	// Fill the datastructure here

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Whitelist enabled");
#endif

	// Set the neighbour timeout timer
	discovery_timeout_timer = xTimerCreate("discovery timeout timer", pdMS_TO_TICKS(DISCOVERY_NEIGHBOUR_TIMEOUT * 1000),
										   pdFALSE, NULL, discovery_ipnd_remove_stale_neighbours);
	if (discovery_timeout_timer == NULL) {
		return false;
	}

	if ( !xTimerStart(discovery_timeout_timer, 0) ) {
		return false;
	}

	// Enable discovery module
	discovery_status = 1;

	return true;
}


static struct discovery_ipnd_neighbour_list_entry* discovery_ipnd_find_neighbour(const uint32_t eid)
{
	if( discovery_status == 0 ) {
		// Not initialized yet
		return NULL;
	}

	const linkaddr_t addr = convert_eid_to_rime(eid);

	for(struct discovery_ipnd_neighbour_list_entry* entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( linkaddr_cmp(&entry->neighbour, &addr) ) {
			return entry;
		}
	}

	return NULL;
}


/** 
 * \brief Checks if address is currently listed a neighbour
 * \param dest Address of potential neighbour
 * \return 1 if neighbour, 0 otherwise
 */
static bool discovery_ipnd_is_neighbour(const uint32_t eid)
{
	return (discovery_ipnd_find_neighbour(eid) != NULL);
}

/**
 * \brief Enable discovery functionality
 */
void discovery_ipnd_enable()
{
}

/**
 * \brief Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void discovery_ipnd_disable()
{
}

/**
 * \brief Parses an incoming EID in an IPND frame
 * \param eid Pointer where the EID will be stored
 * \param buffer Pointer to the incoming EID
 * \param length Length of the Pointer
 * \return Length of the parsed EID
 */
uint8_t discovery_ipnd_parse_eid(uint32_t* const eid, const uint8_t* const buffer, const uint8_t length) {
	int ret = 0;

	/* Parse EID */
	ret = eid_parse_host_length(buffer, length, eid);
	if( ret < 0 ) {
		return 0;
	}

	return ret;
}

/**
 * @brief discovery_ipnd_parse_service_param parses the port parameter of
 * a service block of a ipnd beacon message
 * @param attrs will contain the found port after parsing
 */
static void discovery_ipnd_parse_service_param(const uint8_t* const service_param, const uint32_t param_len, ipnd_msg_attrs_t* const attrs)
{
	const size_t param_min_len = STATIC_STRLEN(DISCOVERY_IPND_SERVICE_PORT);
	if (param_len < param_min_len) {
		/*
		 * Service parameter is to small for
		 * containing a port number.
		 */
		return;
	}

	// TODO the parameters are seperated by simicolons.
	// So check all other parameters if there are more than one
	if (memcmp(service_param, DISCOVERY_IPND_SERVICE_PORT, param_min_len) != 0) {
		/*
		 * Service parameter does not contain the
		 * port number of this service
		 */
		return;
	}

	const char* const service_port_val = (char*)service_param + param_min_len;
	attrs->port = atoi(service_port_val);
}

/**
 * \brief Function that could parse the IPND service block
 * \param buffer Pointer to the block
 * \param length Length of the block
 * \return the number of decoded bytes
 */
uint8_t discovery_ipnd_parse_service_block(uint32_t eid, const uint8_t* buffer, const uint8_t length, ipnd_msg_attrs_t* const attrs) {
	uint32_t offset, num_services, i, sdnv_len, tag_len, data_len;
	uint8_t *tag_buf;
	int h;

	// decode the number of services
	sdnv_len = sdnv_decode(buffer, length, &num_services);
	offset = sdnv_len;
	buffer += sdnv_len;

	// iterate through all services
	for (i = 0; num_services > i; ++i) {
		// decode service tag length
		sdnv_len = sdnv_decode(buffer, length - offset, &tag_len);
		offset += sdnv_len;
		buffer += sdnv_len;

		// decode service tag string
		tag_buf = (uint8_t*)buffer;
		offset += tag_len;
		buffer += tag_len;

		// decode service content length
		sdnv_len = sdnv_decode(buffer, length - offset, &data_len);
		offset += sdnv_len;
		buffer += sdnv_len;

		/* parse UDP-CL service data, if available */
		const size_t udpcl_len = STATIC_STRLEN(DISCOVERY_IPND_SERVICE_UDP);
		if (tag_len == udpcl_len && memcmp(tag_buf, DISCOVERY_IPND_SERVICE_UDP, udpcl_len) == 0) {
			// TODO warn if the port will be overwritten
			discovery_ipnd_parse_service_param(buffer, data_len, attrs);
		}

		// Allow all registered DTN APPs to parse the IPND service block
		for(h=0; dtn_apps[h] != NULL; h++) {
			if( dtn_apps[h]->parse_ipnd_service_block == NULL ) {
				continue;
			}

			dtn_apps[h]->parse_ipnd_service_block(eid, tag_buf, tag_len, (uint8_t*)buffer, data_len);
		}

		offset += data_len;
		buffer += data_len;
	}


	return offset;
}

/**
 * \brief Dummy function that could parse the IPND bloom filter
 * \param eid The endpoint which broadcasted this bloomfilter
 * \param buffer Pointer to the filter
 * \param length Length of the filter
 * \return dummy
 */
uint8_t discovery_ipnd_parse_bloomfilter(uint32_t eid, const uint8_t* const buffer, const uint8_t length) {
	return 0;
}

/**
 * \brief DTN Network has received an incoming discovery packet
 * \param source Source address of the packet
 * \param payload Payload pointer of the packet
 * \param length Length of the payload
 */
static int discovery_ipnd_receive(const cl_addr_t* const addr, const uint8_t* const payload, const uint8_t length)
{
	if( discovery_status == 0 ) {
		// Not initialized yet
		return -1;
	}

	ipnd_msg_attrs_t attrs;
	discovery_ipnd_parse_msg(payload, length, &attrs);

	/* overwrite the port, if it was ip discovery message */
	if (addr->clayer == &clayer_udp_dgram) {
		cl_addr_t bundle_addr;
		memcpy(&bundle_addr, addr, sizeof(bundle_addr));
		bundle_addr.port = attrs.port;

		/*
		 * save the new neighbour,
		 * if it not already exists.
		 * If it already exists refresh only the time stamp.
		 */
		return discovery_ipnd_save_neighbour(attrs.node_id, &bundle_addr);
	} else if (addr->clayer == &clayer_lowpan_dgram) {
		return discovery_ipnd_save_neighbour(attrs.node_id, addr);
	} else {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "Unknown convergence layer %p used.", addr->clayer);
		return -1;
	}
}


static void discovery_ipnd_parse_msg(const uint8_t* const payload, const uint8_t length, ipnd_msg_attrs_t* const attrs)
{
	if (attrs == NULL) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "Attribute structure is a NULL pointer.");
		return;
	}

	/* initialize the return variables */
	memset(attrs, 0, sizeof(ipnd_msg_attrs_t));

	/* set default dgram:udp cl port */
	attrs->port = CL_UDP_BUNDLE_PORT;


	if( length < 3 ) {
		// IPND must have at least 3 bytes
		return;
	}

	// Version
	int offset = 0;
	if( payload[offset++] != 0x02 ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "IPND version mismatch");
		return;
	}

	// Flags
	const uint8_t flags = payload[offset++];

	// Sequence Number
	attrs->sequence_nr = ((payload[offset] & 0xFF) << 8) + (payload[offset+1] & 0xFF);
	offset += 2;

	if( flags & IPND_FLAGS_SOURCE_EID ) {
		offset += discovery_ipnd_parse_eid(&(attrs->node_id), &payload[offset], length - offset);
	}

	if( flags & IPND_FLAGS_SERVICE_BLOCK ) {
		offset += discovery_ipnd_parse_service_block(attrs->node_id, &payload[offset], length - offset, attrs);
	}

	if( flags & IPND_FLAGS_BLOOMFILTER ) {
		offset += discovery_ipnd_parse_bloomfilter(attrs->node_id, &payload[offset], length - offset);
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Discovery from ipn:%lu with flags %02X and seqNo %u", attrs->node_id, flags, attrs->sequence_nr);
}


#ifdef DISCOVERY_OVER_ETHERNET
/**
 * @brief discovery_ipnd_add_service_udp_cl
 * @param buffer the beacon message buffer
 * @param buf_len the max length of beacon message buffer
 * @param poffset pointer to the offset inside the message buffer
 * @return The count of added service blocks
 */
static int discovery_ipnd_add_service_udp_cl(uint8_t* const buffer, const size_t buf_len, int* const poffset)
{
	/*
	 * Overwrite offset at the end of the function,
	 * if the processing not failed
	 */
	uint8_t offset = *poffset;

	/* a service block for IPND
	 * consist of the following fields
	 * (see IPND darft 01 for more details)
	 *
	 * 1 byte	Service name length
	 * x byte	Service name contains "udpcl"
	 * 1 byte	Service parameters length
	 * y byte	Service parameters contains "port=65535;"
	 */
	const uint8_t service_name_len = STATIC_STRLEN(DISCOVERY_IPND_SERVICE_UDP);
	const uint8_t service_param_type_len = STATIC_STRLEN(DISCOVERY_IPND_SERVICE_PORT);
	const uint8_t UINT16_AS_STRING_LEN = 5;
	const uint8_t MAX_SERVICE_PARAM_LEN = service_param_type_len + UINT16_AS_STRING_LEN + 1;

	if ( buf_len < (offset + 1 + service_name_len + 1 + MAX_SERVICE_PARAM_LEN) ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "Discovery message buffer is too small for UDP-CL service parameters.");
		return 0;
	}

	/* add the service name length */
	buffer[offset++] = service_name_len;

	/* add the service name */
	uint8_t* const service_name_buf = &buffer[offset];
	if (memcpy(service_name_buf, DISCOVERY_IPND_SERVICE_UDP, service_name_len) != service_name_buf) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "memcpy failed.");
		return 0;
	}
	offset += service_name_len;

	/* remember the position for the service parameter length byte */
	uint8_t* const service_param_len = &buffer[offset++];

	/* build and add the service parameter value */
	const int len = snprintf((char*)&buffer[offset], MAX_SERVICE_PARAM_LEN, DISCOVERY_IPND_SERVICE_PORT"%u;", CL_UDP_BUNDLE_PORT);
	if (len < 0) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "snprintf failed.");
		return 0;
	}
	(*service_param_len) = len;
	offset += len;

	/*
	 * Everything is done successfully.
	 * So return the new offset and
	 * the count of added service blocks.
	 */
	(*poffset) = offset;
	/* One service block was added */
	return 1;
}
#endif /* SEND_DGRAM_UDPCL_PORT */


/**
 * \brief Send out IPND beacon
 */
static void discovery_ipnd_send()
{
	uint8_t ipnd_buffer[DISCOVERY_IPND_BUFFER_LEN];
	int offset = 0;
	int h = 0;
	uint8_t * services = NULL;

	// Clear the ipnd_buffer
	memset(ipnd_buffer, 0, DISCOVERY_IPND_BUFFER_LEN);

	// Version
	ipnd_buffer[offset++] = 0x02;

	// Flags
	ipnd_buffer[offset++] = IPND_FLAGS_SOURCE_EID | IPND_FLAGS_SERVICE_BLOCK;

	// Beacon Sequence Number
	ipnd_buffer[offset++] = (discovery_sequencenumber & 0xFF00) >> 8;
	ipnd_buffer[offset++] = (discovery_sequencenumber & 0x00FF) >> 0;
	discovery_sequencenumber ++;

	/**
	 * Add node's EID
	 */
	offset += eid_create_host_length(dtn_node_id, &ipnd_buffer[offset], DISCOVERY_IPND_BUFFER_LEN - offset);

	/**
	 * Add static Service block
	 */
	services = &ipnd_buffer[offset++]; // This is a pointer onto the location of the service counter in the outgoing buffer
	/* first service is the DGRAM:LOWPAN convergence layer */
	*services = 1;

	// Allow all registered DTN APPs to add an IPND service block
	for(h=0; dtn_apps[h] != NULL; h++) {
		if( dtn_apps[h]->add_ipnd_service_block == NULL ) {
			continue;
		}

		*services += dtn_apps[h]->add_ipnd_service_block(ipnd_buffer, DISCOVERY_IPND_BUFFER_LEN, &offset);
	}

	// Now: Send it
	/* retry sending, if the tranceiver is busy */
	for (int i=0; i<10; i++) {
		if (clayer_lowpan_dgram.send_discovery(ipnd_buffer, offset) >= 1) {
			LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Discovery beacon message sent over lowpan.");
			break;
		}

		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Discovery beacon message sent over lowpan failed.");
		vTaskDelay(pdMS_TO_TICKS(1));
	}


#ifdef DISCOVERY_OVER_ETHERNET
	*services += discovery_ipnd_add_service_udp_cl(ipnd_buffer, sizeof(ipnd_buffer), &offset);

	const int ret = convergence_layers_send_discovery_ethernet(ipnd_buffer, offset);
	if (ret < 0) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Discovery beacon message sent over udp failed. (ret %d)", ret);
	} else {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Discovery beacon message sent over udp.");
	}
#endif /* DISCOVERY_OVER_ETHERNET */
}

/**
 * \brief Checks if ''neighbours'' is already known
 * Yes: refresh timestamp
 * No:  Create entry
 *
 * \param neighbour Address of the neighbour that should be refreshed
 * \return <0 error
 *          0 neighbour not existing
 *          1 neighbour was updated
 */
static int discovery_ipnd_refresh_neighbour(const cl_addr_t* const neighbour)
{
	if (neighbour == NULL) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "called with NULL pointer address.");
		return -1;
	}

#if DISCOVERY_IPND_WHITELIST > 0
	int i;
	int found = 0;
	for(i=0; i<DISCOVERY_IPND_WHITELIST; i++) {
		if( linkaddr_cmp(&discovery_whitelist[i], neighbour) ) {
			found = 1;
			break;
		}
	}

	if( !found ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "Ignoring peer %u.%u, not on whitelist", neighbour->u8[0], neighbour->u8[1]);
		return;
	}
#endif

	if( discovery_status == 0 ) {
		// Not initialized yet
		return - 2;
	}

	for(struct discovery_ipnd_neighbour_list_entry* entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if(  discovery_neighbour_cmp( (struct discovery_neighbour_list_entry*)entry, neighbour )  ) {
			if (neighbour->isIP) {
				entry->timestamp_last_ip = clock_seconds();
			} else {
				entry->timestamp_last_lowpan = clock_seconds();
			}
			return 1;
		}
	}

	return 0;
}


static void discovery_ipnd_destroy_neighbour(struct discovery_ipnd_neighbour_list_entry* entry)
{
	list_remove(neighbour_list, entry);
	memb_free(&neighbour_mem, entry);
}


/**
 * \brief Marks a neighbour as 'dead' after multiple transmission attempts have failed
 * \param neighbour Address of the neighbour
 */
static void discovery_ipnd_delete_neighbour(const cl_addr_t* const neighbour)
{
	struct discovery_ipnd_neighbour_list_entry * entry;

	if( discovery_status == 0 ) {
		// Not initialized yet
		return;
	}

	char addr_str[CL_ADDR_STRING_LENGTH];
	cl_addr_string(neighbour, addr_str, sizeof(addr_str));
	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Neighbour %s disappeared", addr_str);

	// Tell the CL that this neighbour has disappeared
	convergence_layer_dgram_neighbour_down(neighbour);

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		if( discovery_neighbour_cmp((struct discovery_neighbour_list_entry*)entry, neighbour) ) {
			/* firstly remove the corresponding address type
			 * and check, if there are other adresses
			 */
			const uint8_t addr_type = neighbour->isIP ? CL_TYPE_FLAG_DGRAM_UDP : CL_TYPE_FLAG_DGRAM_LOWPAN;
			entry->addr_type &= ~addr_type;

			if (entry->addr_type != 0) {
				/* there are other addresses for this neighbour available.
				 * So do not delete this discovery entry
				 */
				return;
			}

			// Notify the statistics module
			statistics_contacts_down(&entry->neighbour, entry->timestamp_last_lowpan - entry->timestamp_discovered);
			// TODO statistics_contacts_down(&entry->neighbour, entry->timestamp_last_ip - entry->timestamp_discovered);

			discovery_ipnd_destroy_neighbour(entry);
			return;
		}
	}
}


static void discovery_ipnd_neighbour_update_ip(const cl_addr_t* const addr, struct discovery_ipnd_neighbour_list_entry* const entry)
{
	entry->addr_type |= CL_TYPE_FLAG_DGRAM_UDP;
	ip_addr_copy(entry->ip, addr->ip);
	entry->port = addr->port;
	// TODO update ip timestamp
	entry->timestamp_last_ip = clock_seconds();
}


/**
 * \brief Save neighbour to local cache
 * \param neighbour Address of the neighbour
 */
static int discovery_ipnd_save_neighbour(const uint32_t eid, const cl_addr_t* const addr)
{
	if( discovery_status == 0 ) {
		// Not initialized yet
		return -1;
	}

	// If we know that neighbour already, no need to re-add it
	if(discovery_ipnd_refresh_neighbour(addr) >= 1) {
		return 0;
	}


	/* add informations to an existing LOWPAN entry,
	 * if the eid matches
	 */
	if (addr->isIP) {
		struct discovery_ipnd_neighbour_list_entry* const entry = discovery_ipnd_find_neighbour(eid);
		if (entry != NULL) {
			/* entry matching the EID already existing
			 * add the missing parameters
			 */
			discovery_ipnd_neighbour_update_ip(addr, entry);
			return 1;
		}
	}

	struct discovery_ipnd_neighbour_list_entry* const entry = memb_alloc(&neighbour_mem);

	if( entry == NULL ) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_WRN, "no more space for neighbours");
		return -2;
	}

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Found new neighbour ipn:%lu", eid);

	// Clean the entry struct
	memset(entry, 0, sizeof(struct discovery_ipnd_neighbour_list_entry));

	entry->neighbour = convert_eid_to_rime(eid);
	if (addr->isIP) {
		discovery_ipnd_neighbour_update_ip(addr, entry);
	} else {
		entry->addr_type = CL_TYPE_FLAG_DGRAM_LOWPAN;
		ip_addr_copy(entry->ip, *IP_ADDR_ANY);
		entry->port = 0;
		entry->timestamp_last_lowpan = clock_seconds();

		/* rime and eid should be the same
		 * otherwise the wrong destination will be used
		 * in further processing
		 */
		// TODO remove const cast
		if (convert_rime_to_eid((linkaddr_t*)&addr->lowpan) != eid) {
			LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "The EID %u and the PAN addresse %u.%u are different. This is not supported.",
				eid, addr->lowpan.u8[0], addr->lowpan.u8[1]);
		}
	}
	entry->timestamp_discovered = clock_seconds();

	// Notify the statistics module
	statistics_contacts_up(eid);

	list_add(neighbour_list, entry);

	// We have found a new neighbour, now go and notify the agent
	const event_container_t event = {
		.event = dtn_beacon_event,
		.linkaddr = &entry->neighbour
	};
	agent_send_event(&event);

	return 2;
}

/**
 * \brief Returns the list of currently known neighbours
 * \return Pointer to list with neighbours
 */
struct discovery_neighbour_list_entry * discovery_ipnd_list_neighbours()
{
	return list_head(neighbour_list);
}

/**
 * \brief Stops pending discoveries
 */
void discovery_ipnd_stop_pending()
{

}

static void discovery_ipnd_start(clock_time_t duration, uint8_t index)
{
	// Send at the beginning of a cycle
	discovery_ipnd_send();
}

static void discovery_ipnd_stop()
{
	// Send at the end of a cycle
	discovery_ipnd_send();
}

void discovery_ipnd_clear()
{
	struct discovery_ipnd_neighbour_list_entry * entry;

	LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_INF, "Clearing neighbour list");

	while(list_head(neighbour_list) != NULL) {
		entry = list_head(neighbour_list);

		// Notify the statistics module
		statistics_contacts_down(&entry->neighbour, entry->timestamp_last_lowpan - entry->timestamp_discovered);
		// TODO statistics_contacts_down(&entry->neighbour, entry->timestamp_last_ip - entry->timestamp_discovered);

		/* call convergence_layer_neighbour_down for all discovered address types */
		cl_addr_t addr;
		if (discovery_neighbour_to_addr((struct discovery_neighbour_list_entry*)entry, CL_TYPE_FLAG_DGRAM_LOWPAN, &addr) >= 0) {
			convergence_layer_dgram_neighbour_down(&addr);
		}
		if (discovery_neighbour_to_addr((struct discovery_neighbour_list_entry*)entry, CL_TYPE_FLAG_DGRAM_UDP, &addr) >= 0) {
			convergence_layer_dgram_neighbour_down(&addr);
		}

		list_remove(neighbour_list, entry);
		memb_free(&neighbour_mem, entry);
	}
}


static int discovery_ipnd_check_neighbour_timeout(struct discovery_ipnd_neighbour_list_entry* const entry, const unsigned long timestamp_last,
												  const uint8_t addr_type)
{
	const unsigned long diff = clock_seconds() - timestamp_last;
	if (diff > DISCOVERY_NEIGHBOUR_TIMEOUT) {
		LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_DBG, "Neighbour ipn:%u timed out: %lu vs. %lu = %lu", convert_rime_to_eid(&entry->neighbour),
				clock_seconds(), timestamp_last, diff);

		cl_addr_t addr;
		if (discovery_neighbour_to_addr((struct discovery_neighbour_list_entry*)entry, addr_type, &addr) < 0) {
			// TODO possibly remove the entry
			LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "Could not convert address of discovery entry");
			return -1;
		} else {
			discovery_ipnd_delete_neighbour(&addr);
			return 1;
		}
	}

	return 0;
}


static void discovery_ipnd_remove_stale_neighbours(const TimerHandle_t timer)
{
	bool changed = true;
	while (changed) {
		changed = false;

		for(struct discovery_ipnd_neighbour_list_entry* entry = list_head(neighbour_list);
				entry != NULL;
				entry = list_item_next(entry)) {
			const bool lowpan_exists = (entry->addr_type & CL_TYPE_FLAG_DGRAM_LOWPAN);
			const bool ip_exists = (entry->addr_type & CL_TYPE_FLAG_DGRAM_UDP);
			if (!lowpan_exists && !ip_exists) {
				LOG(LOGD_DTN, LOG_DISCOVERY, LOGL_ERR, "Entry without an vaild address. Deleting");
				discovery_ipnd_destroy_neighbour(entry);
				changed = true;
				break;
			}


			if (lowpan_exists && discovery_ipnd_check_neighbour_timeout(entry, entry->timestamp_last_lowpan, CL_TYPE_FLAG_DGRAM_LOWPAN) > 0) {
				changed = true;
				/* not break here,
				 * because possibly the next address type is
				 * timed out too.
				 * So the full entry has to be deleted
				 */
			}

			if (ip_exists && discovery_ipnd_check_neighbour_timeout(entry, entry->timestamp_last_ip, CL_TYPE_FLAG_DGRAM_UDP) > 0) {
				changed = true;
				/* not break here,
				 * because possibly the next address type is
				 * timed out too.
				 * So the full entry has to be deleted
				 */
			}

			/* list has changed
			 * so reload list entries
			 */
			if (changed) {
				break;
			}

		}
	}

	xTimerReset(discovery_timeout_timer, 0);
}

void discovery_ipnd_print_list()
{
	struct discovery_ipnd_neighbour_list_entry * entry;

	for(entry = list_head(neighbour_list);
			entry != NULL;
			entry = list_item_next(entry)) {
		printf("%p: %u.%u -> %p\n", entry, entry->neighbour.u8[0], entry->neighbour.u8[1], entry->next);
	}
}

const struct discovery_driver discovery_ipnd = {
		.name			= "IPND_DISCOVERY",
		.init			= discovery_ipnd_init,
		.is_neighbour	= discovery_ipnd_is_neighbour,
		.enable			= discovery_ipnd_enable,
		.disable		= discovery_ipnd_disable,
		.receive		= discovery_ipnd_receive,
		.alive			= discovery_ipnd_refresh_neighbour,
		.dead			= discovery_ipnd_delete_neighbour,
		.discover		= discovery_ipnd_is_neighbour,
		.neighbours		= discovery_ipnd_list_neighbours,
		.stop_pending	= discovery_ipnd_stop_pending,
		.start			= discovery_ipnd_start,
		.stop			= discovery_ipnd_stop,
		.clear			= discovery_ipnd_clear
};
/** @} */
/** @} */


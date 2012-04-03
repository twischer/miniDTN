/**
 * \addtogroup discovery 
 * @{
 */

 /**
 * \defgroup ipnd_biscover  IP-ND discovery module
 *
 * @{
 */

/**
* \file
* IP-ND compliant discovery module
* IP-ND = http://tools.ietf.org/html/draft-irtf-dtnrg-ipnd-01
*
* \author Wolf-Bastian Pšttner (poettner@ibr.cs.tu-bs.de)
*/

#include "custody-signal.h"
#include "dtn_config.h"
#include "dtn-network.h"
#include "clock.h"
#include "net/netstack.h"
#include "net/packetbuf.h" 
#include "net/rime/rimeaddr.h"

#include "agent.h"
#include "status-report.h"
#include "dtn-network.h" 
#include "discovery.h"
#include <string.h> // for memset

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void b_dis_neighbour_found(rimeaddr_t * neighbour);
void b_dis_refresh_neighbour(rimeaddr_t * neighbour);
void b_dis_save_neighbour(rimeaddr_t * neighbour);

#define DISCOVERY_NEIGHBOUR_CACHE	10
#define DISCOVERY_NEIGHBOUR_TIMEOUT	5

PROCESS(discovery_process, "DISCOVERY process");

struct discovery_neighbour_entry {
	uint8_t active;
	rimeaddr_t neighbour;
	clock_time_t timestamp;
};

struct discovery_neighbour_entry discovery_neighbours[DISCOVERY_NEIGHBOUR_CACHE];
uint8_t discovery_status;
struct etimer discovery_timeout_timer;

void b_dis_init()
{
	// Clean neighbour database
	memset(discovery_neighbours, 0, DISCOVERY_NEIGHBOUR_CACHE * sizeof(struct discovery_neighbour_entry));

	// Enable discovery module
	discovery_status = 1;

	// Start discovery process
	process_start(&discovery_process, NULL);
}

/** 
* \brief sends a discovery message 
* \param bundle pointer to a bundle (not used here)
*/
uint8_t b_dis_neighbour(rimeaddr_t * dest)
{
	int g;
	for(g=0; g<DISCOVERY_NEIGHBOUR_CACHE; g++) {
		if( discovery_neighbours[g].active && rimeaddr_cmp(&(discovery_neighbours[g].neighbour), dest) ) {
			return 1;
		}
	}

	return 0;
}

void b_dis_send(rimeaddr_t * destination) {
	if( discovery_status == 0 ) {
		return;
	}

	rimeaddr_t dest={{0,0}};
	PRINTF("dtn_send_discover\n");
	dtn_send_discover((uint8_t *) "DTN_DISCOVERY", 13, &dest);
}

/**
* \brief checks if msg is an answer to a discovery message
* \param msg pointer to the received message
* \return 1 if msg is an answer or 0 if not
*/
uint8_t b_dis_is_beacon(uint8_t * msg)
{
	uint8_t test[8]="DTN_HERE";
	uint8_t i;
	for (i=sizeof(test); i>0 ; i--){
		if(test[i-1]!=msg[i-1]){
			return 0;
		}
	}
	return 1;
}

/**
* \brief hecks if msg is a discovery message
* \param msg pointer to the received message
* \return 1 if msg is a discovery message or 0 if not
*/
uint8_t b_dis_is_discover(uint8_t * msg, rimeaddr_t * dest)
{
	uint8_t test[13]="DTN_DISCOVERY";
	uint8_t i;
	for (i=sizeof(test); i>0; i--) {
		if(test[i-1]!=msg[i-1]) {
			return 0;
		}
	}

	if( discovery_status == 0 ) {
		return 1;
	}

	PRINTF("DTN DISCOVERY\n");
	dtn_send_discover((uint8_t *) "DTN_HERE", 8, dest);

	return 1;
}

/**
 * Enable discovery functionality
 */
void b_dis_enable()
{
	discovery_status = 1;
}

/**
 * Disable discovery functionality
 * Prevents outgoing packets from being sent
 */
void b_dis_disable()
{
	discovery_status = 0;
}

void b_dis_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length)
{
	PRINTF("DISCOVERY: received from %u:%u\n", source->u8[0], source->u8[1]);

	// Save all peer from which we receive packets to the active neighbours list
	b_dis_refresh_neighbour(source);

	// Either somebody wants to discover ourselves
	if( b_dis_is_discover(payload, source) ) {
		PRINTF("is_discover\n");
		return;
	}

	// Or we have received a reply to our query
	if( b_dis_is_beacon(payload) ) {
		PRINTF("is_beacon\n");
		b_dis_neighbour_found(source);
	}
}

void b_dis_neighbour_found(rimeaddr_t * neighbour)
{
	PRINTF("DISCOVERY: sending DTN BEACON Event for %u.%u\n", neighbour->u8[0], neighbour->u8[1]);
	process_post(&agent_process, dtn_beacon_event, neighbour);
}

/**
 * Checks if ''neighbours'' is already known
 * Yes: refresh timestamp
 * No:  Create entry
 */
void b_dis_refresh_neighbour(rimeaddr_t * neighbour) {
	int g;

	for(g=0; g<DISCOVERY_NEIGHBOUR_CACHE; g++) {
		if( discovery_neighbours[g].active && rimeaddr_cmp(&(discovery_neighbours[g].neighbour), neighbour) ) {
			discovery_neighbours[g].timestamp = clock_time();
			return;
		}
	}

	b_dis_save_neighbour(neighbour);
}

void b_dis_peer_alive(rimeaddr_t * neighbour)
{
	b_dis_refresh_neighbour(neighbour);
}

void b_dis_save_neighbour_at(rimeaddr_t * neighbour, uint8_t where)
{
	memset(&discovery_neighbours[where], 0, sizeof(struct discovery_neighbour_entry));

	discovery_neighbours[where].active = 1;
	rimeaddr_copy(&discovery_neighbours[where].neighbour, neighbour);
	discovery_neighbours[where].timestamp = clock_time();
}

/**
 * Save neighbour to local cache
 */
void b_dis_save_neighbour(rimeaddr_t * neighbour)
{
	// If we know that neighbour already, no need to re-add it
	if( b_dis_neighbour(neighbour) ) {
		return;
	}

	int g;
	for(g=0; g<DISCOVERY_NEIGHBOUR_CACHE; g++) {
		if( !discovery_neighbours[g].active ) {
			PRINTF("DISCOVERY: Saving neighbour %u:%u to %u\n", neighbour->u8[0], neighbour->u8[1], g);
			b_dis_save_neighbour_at(neighbour, g);
			return;
		}
	}

	clock_time_t age = 0;
	uint8_t where = 0;
	for(g=0; g<DISCOVERY_NEIGHBOUR_CACHE; g++) {
		if( (clock_time() - discovery_neighbours[g].timestamp) > age ) {
			age = clock_time() - discovery_neighbours[g].timestamp;
			where = g;
		}
	}

	PRINTF("DISCOVERY: Overwriting neighbour %u:%u to %u\n", neighbour->u8[0], neighbour->u8[1], where);
	b_dis_save_neighbour_at(neighbour, where);
}


/**
 * Start to discover a neighbour
 * Returns 1 if neighbour is already known in (likely) in range
 * Returns 0 otherwise
 */
uint8_t b_dis_discover(rimeaddr_t * dest)
{
	if (dest==0){
		rimeaddr_t tmp={{0,0}};
		dest=&tmp;
		PRINTF("DISCOVERY: agent asks to discover broadcast\n");
		b_dis_send(dest);
		return 0;
	}else{
		PRINTF("DISCOVERY: agent asks to discover %u:%u\n", dest->u8[0], dest->u8[1]);

		// Check, if we already know this neighbour
		if(b_dis_neighbour(dest)) {
			return 1;
		}

		// Otherwise, send out a discovery beacon
		b_dis_send(dest);

		return 0;
	}
}

/**
 * Basic Discovery Persistent Process
 */
PROCESS_THREAD(discovery_process, ev, data)
{
	PROCESS_BEGIN();

	etimer_set(&discovery_timeout_timer, DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND);
	PRINTF("DISCOVERY: process running\n");

	while(1) {
		PROCESS_WAIT_EVENT_UNTIL(ev);

		if( etimer_expired(&discovery_timeout_timer) ) {
			int g;

			for(g=0; g<DISCOVERY_NEIGHBOUR_CACHE; g++) {
				if( discovery_neighbours[g].active &&
						(clock_time() - discovery_neighbours[g].timestamp) > (DISCOVERY_NEIGHBOUR_TIMEOUT * CLOCK_SECOND) ) {
					PRINTF("DISCOVERY: Neighbour %u:%u at %u timed out: %lu vs. %lu = %lu\n", discovery_neighbours[g].neighbour.u8[0], discovery_neighbours[g].neighbour.u8[1], g, clock_time(), discovery_neighbours[g].timestamp, clock_time() - discovery_neighbours[g].timestamp);
					memset(&discovery_neighbours[g], 0, sizeof(struct discovery_neighbour_entry));
				}
			}

			etimer_reset(&discovery_timeout_timer);
		}
	}

	PROCESS_END();
}

const struct discovery_driver b_discovery ={
	"B_DISCOVERY",
	b_dis_init,
	b_dis_neighbour,
	b_dis_enable,
	b_dis_disable,
	b_dis_receive,
	b_dis_peer_alive,
	b_dis_discover,
};
/** @} */
/** @} */

/**
 * \addtogroup interface
 * @{
 */

/**
 * \file
 *         Anbindung an untere Schichten um Daten zu Senden und Empfangen
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clock.h"
#include "net/dtn/dtn_config.h"
#include "dtn-network.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime/rimeaddr.h"
#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "dispatching.h"
#include "routing.h"
#include "mmem.h"
#include "discovery.h"
#include "stimer.h"
#include "leds.h"
#if CONTIKI_TARGET_AVR_RAVEN
	#include <stings.h>
#endif

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

const struct mac_driver *dtn_network_mac;

uint16_t *output_offset_ptr;

//static uint8_t bundle_seqno = 0;


static void packet_sent(void *ptr, int status, int num_tx);

static struct bundle_t bundle;	
static rimeaddr_t beacon_src;
//static struct stimer wait_timer;
static uint16_t last_send;

static void dtn_network_init(void) 
{
	last_send= 0;
	packetbuf_clear();
//	input_buffer_clear();
	dtn_network_mac = &NETSTACK_MAC;
//	stimer_set(&wait_timer, 1);
	PRINTF("DTN init\n");
}


/**
*called for incomming packages
*/
static void dtn_network_input(void) 
{
	printf("DTN-NETWORK: got packet\n");
	uint8_t input_packet[114];
	int size=packetbuf_copyto(input_packet);
	rimeaddr_t dest = *packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
	rimeaddr_t bsrc = *packetbuf_addr(PACKETBUF_ADDR_SENDER);
	int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	printf("NET: rssi = %d\n", rssi-45);
	PRINTF("%x%x: dtn_network_input\n",dest.u8[0],dest.u8[1]);
	if((dest.u8[0]==0) & (dest.u8[1]==0)) { //broadcast message
		PRINTF("Broadcast\n");
			
		DISCOVERY.is_discover(input_packet);
		packetbuf_clear();
			
        } else {
		if (!DISCOVERY.is_beacon(input_packet)){ //packet is a bundle
			packetbuf_clear();
			PRINTF("%p  %p\n",&bundle,&input_packet);	
			printf(".\n");
			struct mmem mem;
			mmem_alloc(&mem,114);
			if (!MMEM_PTR(&mem)){
				PRINTF("DTN: MMEM ERROR\n");
				return;
			}

			memcpy(MMEM_PTR(&mem),&input_packet,114);
			memset(&bundle, 0, sizeof(struct bundle_t));
			if ( !recover_bundel(&bundle,&mem, (uint8_t)size)){
				PRINTF("DTN: recover ERROR\n");	
				return;
			}
			if (bundle.flags&2){
				//printf("NET: %u\n",*((uint8_t *)bundle.mem.ptr + bundle.offset_tab[DATA][OFFSET]));
			}
			bundle.rec_time=(uint32_t) clock_seconds();
#if DEBUG_H
			bundle.debug_time=clock_time();
#endif
			bundle.size= (uint8_t) size;
#if DEBUG
			uint8_t i;
			printf("NETWORK: input ");
			for (i=0; i<bundle.size; i++){
				printf("%x:",*((uint8_t *)bundle.mem.ptr + i));
			}
			printf("\n");
#endif
			bundle.msrc.u8[0]=bsrc.u8[0];
			bundle.msrc.u8[1]=bsrc.u8[1];
			//printf("NETWORK: %u:%u\n", bundle.msrc.u8[0],bundle.msrc.u8[1]);
			PRINTF("NETWORK: size of received bundle: %u block pointer %p\n",bundle.size, bundle.mem.ptr);
			dispatch_bundle(&bundle);			
//			process_post(&agent_process, dtn_receive_bundle_event, &bundle);
		}else{
			
			memcpy(&beacon_src,&bsrc,sizeof(beacon_src));
			packetbuf_clear();
			PRINTF("NETWORK: got beacon from %u,%u\n",beacon_src.u8[0],beacon_src.u8[1]);
			process_post(&agent_process, dtn_beacon_event, &beacon_src);
//			process_post(&agent_process, dtn_send_admin_record_event, NULL);
		}		
	}
		
}


static void packet_sent(void *ptr, int status, int num_tx) 
{
	switch(status) {
	  case MAC_TX_COLLISION:
	    PRINTF("DTN: collision after %d tx\n", num_tx);
	    break;
	  case MAC_TX_NOACK:
	    PRINTF("DTN: noack after %d tx\n", num_tx);
	    break;
	  case MAC_TX_OK:
	    PRINTF("DTN: sent after %d tx\n", num_tx);
	    break;
	  default:
	    PRINTF("DTN: error %d after %d tx\n", status, num_tx);
	  }
	last_send--;
	if (!last_send){
		leds_on(1);
	}
	struct route_t *route= (struct route_t *)ptr;
	PRINTF("DTN: bundle_num : %u    %p\n",route->bundle_num,ptr);
	//printf("sent to %u:%u\n",route->dest.u8[0],route->dest.u8[1]);
	ROUTING.sent((struct route_t *)ptr,status,num_tx);
	#if 0
	uint16_t bundlebuf_length;
	bundlebuf_length =  bundlebuf_get_length();
	PRINTF("DTN-NETWORK: Buflen: %i, Offset: %i \n", bundlebuf_length, *output_offset_ptr);
	
	/* überprüfe ob alle teile eines Bündels gesendet worden sind */	
	if(bundlebuf_length > *output_offset_ptr) {
		
		dtn_network_send();
	}
	/* ist alles gesendet, leere den bundlebuffer, erhöhe die sequenznummer und
	resette den offset */
	else {
		
		bundlebuf_clear();
		*output_offset_ptr = 0;
		bundle_seqno = (bundle_seqno+1) % 16;
	}
	#endif
		
}

int dtn_network_send(struct bundle_t *bundle, struct route_t *route) 
{
	uint8_t *payload = bundle->mem.ptr;
	uint8_t len = bundle->size;
	uint32_t i, time;
	last_send++;
	sdnv_decode(bundle->mem.ptr+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET],bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE],&i);
	sdnv_decode(bundle->mem.ptr+bundle->offset_tab[LIFE_TIME][OFFSET],bundle->offset_tab[LIFE_TIME][STATE],&time);

	PRINTF("seq_num %lu lifetime %lu bundle pointer %p bundel->block %p \n ",i,time,bundle,bundle->mem.ptr);
#if DEBUG
	printf("NETWORK: send ");
	for (i=0; i<bundle->mem.size; i++){
		printf("%x:",*((uint8_t*)bundle->mem.ptr + i));
	}
	printf("\n");
#endif
	/* kopiere die Daten in den packetbuf(fer) */
	packetbuf_copyfrom(payload, len);
	/*setze Zieladresse und übergebe das Paket an die MAC schicht */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &route->dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	
	NETSTACK_MAC.send(&packet_sent, route); 
//	while( clock_time()- last_trans < 80){
//		watchdog_periodic();
//	}
//	last_trans=clock_time();
	
	
	return 1;
}



const struct network_driver dtn_network_driver = 
{
  "DTN",
  dtn_network_init,
  dtn_network_input
};

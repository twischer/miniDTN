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

#include "clock.h"

#include "net/dtn/dtn_config.h"
#include "dtn-network.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime/rimeaddr.h"
#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"
#include "routing.h"
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


static void dtn_network_init(void) 
{
	
	packetbuf_clear();
//	input_buffer_clear();
	dtn_network_mac = &NETSTACK_MAC;
	PRINTF("DTN init\n");
}


/**
*called for incomming packages
*/
static void dtn_network_input(void) 
{
	PRINTF("DTN-NETWORK: got packet\n");
	uint8_t input_packet[114];
	int size=packetbuf_copyto(input_packet);
	rimeaddr_t dest = *packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
	PRINTF("%x%x: dtn_network_input\n",dest.u8[0],dest.u8[1]);
	if((dest.u8[0]==0) & (dest.u8[1]==0)) { //broadcast message
		PRINTF("Broadcast\n");
		uint8_t test[13]="DTN_DISCOVERY";
		uint8_t discover=1;
		uint8_t i;
		for (i=sizeof(test); i>0; i--){
			if(test[i-1]!=input_packet[i-1]){
				discover=0;
				break;
			}
		}
		if (discover){
			PRINTF("DTN DISCOVERY\n");
			rimeaddr_t dest = *packetbuf_addr(PACKETBUF_ADDR_SENDER);
//			rimeaddr_t dest={{0,0}};
			packetbuf_clear();
			packetbuf_copyfrom("DTN_HERE", 8);
			packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);
			packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &dest);
			packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
			NETSTACK_MAC.send(NULL, NULL);
		}else{
			PRINTF("some broadcast message\n");
		}
			
        } else {
		uint8_t test[8]="DTN_HERE";
		uint8_t beacon=1;
		uint8_t i;
		for (i=sizeof(test); i>0; i--){
			if(test[i-1]!=input_packet[i-1]){
				beacon=0;
				break;
			}
		}
		if (!beacon){ //packet is a bundle
			PRINTF("%p  %p\n",&bundle,&input_packet);	
			if ( !recover_bundel(&bundle,&input_packet, (uint8_t)size)){
				return;
			}

			printf("NETWORK: ");
			for (i=0; i<bundle.size; i++){
				printf("%x:",*(bundle.block + i));
			}
			printf("\n");
			bundle.rec_time=(uint32_t) clock_seconds();
#if DEBUG_H
			bundle.debug_time=clock_time();
#endif
			bundle.size= (uint8_t) size;
			PRINTF("NETWORK: size of received bundle: %u block pointer %p\n",bundle.size, bundle.block);
			
			process_post(&agent_process, dtn_receive_bundle_event, &bundle);	
		}else{
			
			rimeaddr_t bsrc = *packetbuf_addr(PACKETBUF_ADDR_SENDER);
			memcpy(&beacon_src,&bsrc,sizeof(beacon_src));
			PRINTF("NETWORK: got beacon from %u,%u\n",beacon_src.u8[0],beacon_src.u8[1]);
			process_post(&agent_process, dtn_beacon_event, &beacon_src);
//			process_post(&agent_process, dtn_send_admin_record_event, NULL);
		}		
	}
	packetbuf_clear();
		
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
	
	struct route_t *route= (struct route_t *)ptr;
	PRINTF("DTN: bundle_num : %u    %p\n",route->bundle_num,ptr);
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
	
	uint8_t *payload = bundle->block;
	uint8_t len = bundle->size;
	uint32_t i, time;
	sdnv_decode(bundle->block+bundle->offset_tab[TIME_STAMP_SEQ_NR][OFFSET],bundle->offset_tab[TIME_STAMP_SEQ_NR][STATE],&i);
	sdnv_decode(bundle->block+bundle->offset_tab[LIFE_TIME][OFFSET],bundle->offset_tab[LIFE_TIME][STATE],&time);

	printf("seq_num %lu lifetime %lu bundle pointer %p bundel->block %p %x\n ",i,time,bundle,bundle->block,(*(bundle->block-2)<<8)+*(bundle->block-1));
	printf("NETWORK: ");
	for (i=0; i<bundle->size; i++){
		printf("%x:",*(bundle->block + i));
	}
	printf("\n");
	/* kopiere die Daten in den packetbuf(fer) */
	packetbuf_copyfrom(payload, len);
	delete_bundle(bundle);
	/*setze Zieladresse und übergebe das Paket an die MAC schicht */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &route->dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	
	NETSTACK_MAC.send(&packet_sent, route); 
	
	
	return 1;
}

int dtn_discover(void)
{	
	uint8_t foo=23;
	rimeaddr_t dest={{0,0}};
	packetbuf_copyfrom("DTN_DISCOVERY", 13);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);
	packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	NETSTACK_MAC.send(NULL, NULL);
	return 1;
}	


const struct network_driver dtn_network_driver = 
{
  "DTN",
  dtn_network_init,
  dtn_network_input
};

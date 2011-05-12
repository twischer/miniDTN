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

#include "dtn-network.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/rime/rimeaddr.h"
#include "net/dtn/bundle.h"
#include "net/dtn/agent.h"


#define DEBUG 1
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
			NETSTACK_MAC.send(&packet_sent, NULL);
		}else{
			PRINTF("some broadcast message\n");
		}
			
        } else {
		uint8_t i;
		PRINTF("%p  %p\n",&bundle,&input_packet);	
		recover_bundel(&bundle,&input_packet, (uint8_t)size);
		bundle.rec_time=(uint32_t) clock_seconds();
		bundle.size= (uint8_t) size;
		PRINTF("NETWORK: size of received bundle: %u\n",bundle.size);
		
		
		
		#if DEBUG
		for(i=0;i<17;i++){
			uint8_t *tmp=bundle.block+bundle.offset_tab[i][0];
			PRINTF("offset %u size %u val %x\n",bundle.offset_tab[i][0], bundle.offset_tab[i][1],*tmp);
		}
		#endif
		process_post(&agent_process, dtn_receive_bundle_event, &bundle);	
			
	}
		
#if 0
	static uint16_t input_offset = 0;
	static uint8_t received_packets = 0;
	static uint8_t last_bundle_seqno = 0;
	static struct timer input_timeout;
	uint8_t input_length;
	uint8_t input_packet[114];
	
	/* pointer zum input buffer */
	uint8_t *bufptr = input_buffer_getptr();
	
	/* sollte der Puffer in Benutzung sein, kann er nicht beschrieben werden */
	if(input_buffer_in_use()) {
		
		PRINTF("DTN-NETWORK Input Buffer in use \n");
		return;
	}
	
	/* braucht ein Paket zu Lange, wird der Puffer geleert */	
	if(received_packets != 0 && timer_expired(&input_timeout)) {
		
		input_offset = 0;
		received_packets = 0;
		input_buffer_clear();
		PRINTF("DTN-NETWORK: transmission timeout \n");
	}
	
	timer_set(&input_timeout, 2*CLOCK_SECOND);
	
	/* kopiere die Daten aus dem Packetbuffer */
	input_length = packetbuf_copyto(input_packet);
	
	/* überprüfe ob Paket zum selben Bündel gehört, wie das vorherige Paket, wenn nicht fängt ein neues bundle an
	also wird der Puffer geleert */
	if(last_bundle_seqno != (input_packet[0] >> 4)) {
		
		input_offset = 0;
		received_packets = 0;
		input_buffer_clear();
	}	
	
	/* schreibe das empfange paket in den empfanspuffer */
	last_bundle_seqno = (input_packet[0] >> 4);
	memcpy(&bufptr[input_offset], &input_packet[1], input_length-1);
	input_offset += input_length-1;
	received_packets++;
	
		
	PRINTF("DTN-NETWORK RECV: input offset: %i, received_packets %i, input[0] %x \n", input_offset, received_packets, input_packet[0]);
	
	/* wurden alle pakete eines bündels empfangen, kann es an den Bundle Agent übergeben werden. */
	if(received_packets == (input_packet[0] & 0x0f)) {
		
		input_offset = 0;
		received_packets = 0;
		input_buffer_set_in_use();
		process_post(&bundle_protocol_process, dtn_receive_bundle_event, bufptr);
	}
#endif
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

int dtn_network_send(uint8_t *payload_ptr, uint8_t payload_len,rimeaddr_t dest) 
{
	
//	uint8_t *bufptr;
//	uint16_t bundlebuf_length;
//	uint8_t packet_len; 
//	static uint16_t output_offset = 0;
//	uint8_t radio_packet[114];
	#if 0	
	/* der offset sagt aus, wieviele bytes eines bundles bereits gesendet worden sind */
	output_offset_ptr = &output_offset;
	bufptr = bundlebuf_getptr();
	bundlebuf_length =  bundlebuf_get_length();
	
	/* handelt es sich um ein Custody Signal, sende es an den vorherigen Knoten */
	if((bufptr[0] & 0x02) != 0) {
		rimeaddr_t custody = PREVIOUS_CUSTODIAN;
		dest = custody;
	}
	/* sonste sende das Bündel an den nächsten Knoten */
	else {
		rimeaddr_t default_route = DEFAULT_ROUTE;
		dest = default_route;
	}
	
	/* berechne die Anzahl an benötigten Radio Packets für das Bündel 
	und schreibe dies in das erste byte */
	radio_packet[0] = bundlebuf_length / 113;
	if((bundlebuf_length % 113) != 0)
		radio_packet[0]  += 1;
	
	/* die sequenznummer wird in die oberen vier bits des ersten bytes geschrieben */
	radio_packet[0] = (bundle_seqno<<4) | radio_packet[0];
	
	PRINTF("DTN-NETWORK SEND: Buf Length: %i, Output Offset: %i, Packets: %x \n",bundlebuf_length, output_offset, radio_packet[0]);
	
	/* sind weniger als 113 bytes vom bündel übrig, passe die Paketlänge an */
	if((bundlebuf_length - output_offset) < 113) {
		
		packet_len = bundlebuf_length - output_offset;
		
		if(packet_len < 31) {
			
			memset(&radio_packet[1], 0, 31);
			memcpy(&radio_packet[1], &bufptr[output_offset], packet_len);
			packet_len = 31;
		}
		
		else {
			memcpy(&radio_packet[1], &bufptr[output_offset], packet_len);
		}
	}
	else {
		
		packet_len = 113;
		memcpy(&radio_packet[1], &bufptr[output_offset], packet_len);
	}
	#endif

	/* kopiere die Daten in den packetbuf(fer) */
	packetbuf_copyfrom(payload_ptr, payload_len);
	
	/*setze Zieladresse und übergebe das Paket an die MAC schicht */
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	
	NETSTACK_MAC.send(&packet_sent, NULL);
	
	
	return 1;
}

int dtn_discover(void)
{
	rimeaddr_t dest={{0,0}};
	packetbuf_copyfrom("DTN_DISCOVERY", 13);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);
	packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	NETSTACK_MAC.send(&packet_sent, NULL);
	return 1;
}	

const struct network_driver dtn_network_driver = 
{
  "DTN",
  dtn_network_init,
  dtn_network_input
};

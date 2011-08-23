/**
 * \addtogroup discovery 
 * @{
 */

 /**
 * \defgroup b_biscover  basic discovery module
 *
 * @{
 */

/**
* \file
* implementation of a basic discovery module
* \author Georg von Zengen (vonzeng@ibr.cs.tu-bs.de) 
*/

#include "net/dtn/custody-signal.h"
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


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


/** 
* \brief sends a discovery message 
* \param bundle pointer to a bundle (not used here)
*/
void b_dis_send(struct bundle_t *bundle)
{
	uint8_t foo=23;  
	rimeaddr_t dest={{0,0}};
	packetbuf_copyfrom("DTN_DISCOVERY", 13);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest);
	packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &dest);
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2);
	NETSTACK_MAC.send(NULL, NULL); 
}
/**
* \brief checks if msg is an answer to a discovery message
* \param msg pointer to the received message
* \return 1 if msg is an answer or 0 if not
*/
uint8_t b_dis_is_beacon(uint8_t *msg)
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
uint8_t b_dis_is_discover(uint8_t *msg)
{
	uint8_t test[13]="DTN_DISCOVERY";
	uint8_t i;
	for (i=sizeof(test); i>0; i--){
		if(test[i-1]!=msg[i-1]){
			return 0;
		}
	}
	PRINTF("DTN DISCOVERY\n");
	rimeaddr_t dest = *packetbuf_addr(PACKETBUF_ADDR_SENDER);
	//if (dest.u8[0]==0xc && dest.u8[1]==0){
	packetbuf_clear();
	packetbuf_copyfrom("DTN_HERE", 8);
	packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &dest); 
	packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &dest); 
	packetbuf_set_attr(PACKETBUF_ADDRSIZE, 2); 
	NETSTACK_MAC.send(NULL, NULL);
	//}
	return 1;
}
const struct discovery_driver b_discovery ={
	"B_DISCOVERY",
	b_dis_send,
	b_dis_is_beacon,
	b_dis_is_discover,
};
/** @} */
/** @} */

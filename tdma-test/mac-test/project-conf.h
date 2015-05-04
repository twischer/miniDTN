#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     tdma_mac_driver
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     tdma_rdc_driver
#undef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   rf230_driver
#undef NETSTACK_CONF_NETWORK 
#define NETSTACK_CONF_NETWORK tdma_network_driver 

#define RF230_CONF_CHECKSUM 1
#undef RADIO_CONF_CHANNEL 
#define RADIO_CONF_CHANNEL 23
#undef RF230_CONF_AUTOACK
#define RF230_CONF_AUTOACK 0
#undef RF230_CONF_FRAME_RETRIES
#define RF230_CONF_FRAME_RETRIES 0
#undef NODE_ID
 #define NODE_ID 5
// #undef RADIO_TX_POWER
// #define RADIO_TX_POWER 30
#endif

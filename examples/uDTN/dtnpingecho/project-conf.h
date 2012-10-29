#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

// Set our MMEM size
#undef MMEM_CONF_SIZE
#define MMEM_CONF_SIZE 2000

// Set the PAN ID that IBR-DTN uses
#define IEEE802154_CONF_PANID 0x780

// Disable profiling to save RAM
#undef PROFILES_CONF_MAX
#define PROFILES_CONF_MAX 0
#undef PROFILES_CONF_STACKSIZE
#define PROFILES_CONF_STACKSIZE 0

// Disable the CSMA retransmission buffers
#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 0

// Disable uIP6's buffer
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 0
 
#endif /* __PROJECT_CONF_H__ */

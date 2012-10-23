#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

// Set our MMEM size
#define MMEM_CONF_SIZE 2000

// Disable the CSMA retransmission buffers
#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 0

// Disable uIP6's buffer
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 0
 
#endif /* __PROJECT_CONF_H__ */

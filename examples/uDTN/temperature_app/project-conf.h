// Disable RIMEs queuebuf
#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 0

// Disable uIP6's buffer
#undef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE 0
 
// Set our MMEM size
#undef MMEM_CONF_SIZE
#define MMEM_CONF_SIZE 4000

// Set our storage elements
#define BUNDLE_CONF_STORAGE_SIZE 100

// Enable statistics
#define STATISTICS_CONF_ELEMENTS 4
#define STATISTICS_CONF_PERIOD 3600

// Set the link layer compatibility to IBR-DTN
#define IBR_COMP 1

// Disable profiling to save memory
#undef PROFILES_CONF_MAX
#define PROFILES_CONF_MAX 1 

#undef PROFILES_CONF_STACKSIZE
#define PROFILES_CONF_STACKSIZE 10

// Set our channel
#undef CHANNEL_802_15_4
#define CHANNEL_802_15_4 22

// Set our PAN ID
#define IEEE802154_CONF_PANID 0x780

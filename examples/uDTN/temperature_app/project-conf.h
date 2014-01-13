#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

// Set our MMEM size
#undef MMEM_CONF_SIZE
#define MMEM_CONF_SIZE 4150

// Set our storage elements
#undef BUNDLE_CONF_STORAGE_SIZE
#define BUNDLE_CONF_STORAGE_SIZE 115

// Enable statistics
#define STATISTICS_CONF_ELEMENTS 4
#define STATISTICS_CONF_PERIOD 3600
#define STATISTICS_CONF_CONTACTS 15

// Set our channel
#undef CHANNEL_802_15_4
#define CHANNEL_802_15_4 22

// Set our PAN ID
#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0x780

#endif /* __PROJECT_CONF_H__ */

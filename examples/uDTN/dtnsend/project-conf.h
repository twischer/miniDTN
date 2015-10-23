#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

// Set the PAN ID that IBR-DTN uses
#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0x780

// Avoid deleting bundles in storage
// 3 = BUNDLE_STORAGE_BEHAVIOUR_DO_NOT_DELETE
#define BUNDLE_CONF_STORAGE_BEHAVIOUR 3

#define CONF_SEND_TO_NODE 2

#define CONF_BUNDLE_SIZE 64

#endif /* __PROJECT_CONF_H__ */

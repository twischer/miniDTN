/**
* \addtogroup agent
* @{
*/
/** 
* \file
* this file chooses which are modules and also confugures some modules
*/
#define BUNDLE_STORAGE r_storage
// #define MMEM_CONF_SIZE 2048 
#define REDUNDANCE b_redundance
#define CUSTODY b_custody
#define ROUTING flood_route
#define STATUS_REPORT b_status
#define WATCHDOG_CONF_TIMEOUT 4
#define DISCOVERY ipnd_discovery
#define HASH hash_xxfast

#define LOG_NET    0
#define LOG_BUNDLE 1
#define LOG_ROUTE  2
#define LOG_STORE  3
#define LOG_SDNV   4
#define LOG_SLOTS  5
#define LOG_AGENT  6

/** @} */



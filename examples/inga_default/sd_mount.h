/* 
 * File:   sd_mount.h
 * Author: enrico
 *
 * Created on 6. Februar 2013, 01:23
 */

#ifndef SD_MOUNT_H
#define	SD_MOUNT_H

/**
 * @addtogroup default_app
 * @{
 */

/**
 * \defgroup sd_mount SD card Mounting
 * @{
 */


#include "contiki.h"

/**
 * Mount event.
 * 
 * @param data Data is address of a boolean that is true if sd card was found
 * and mounted and false if sd card was not found
 */
extern process_event_t event_mount;

/**
 * Mount process.
 * 
 * Tries to find and mount a probably inserted microSD card.
 * Broadcasts event \link event_mount if mounting is done.
 */
PROCESS_NAME(mount_process);

/** @} */
/** @} */

#endif	/* SD_MOUNT_H */


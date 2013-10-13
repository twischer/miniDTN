/* 
 * File:   default_app.h
 * Author: enrico
 *
 * Created on 5. April 2013, 13:39
 */

#ifndef DEFAULT_APP_H
#define	DEFAULT_APP_H

#include <stdint.h>

#define SYS_SD_MOUNTED        0

#define SYS_OUT_USB_EN        1
#define SYS_OUT_SD_EN         2
#define SYS_OUT_RADIO_EN      3

#define SYS_SENSOR_BATT_EN    8
#define SYS_SENSOR_ACC_EN     9 
#define SYS_SENSOR_GYRO_EN    10
#define SYS_SENSOR_TEMP_EN    11
#define SYS_SENSOR_PRESS_EN   12


/*
 * While the configuration flags only express naive desires,
 * the system flags represent the real world state.
 */

/* Sets flag enabled */
inline void sysflag_set_en(uint8_t flag);
/* Sets flag disabled */
inline void sysflag_set_dis(uint8_t flag);
/* Get flag value */
inline uint8_t sysflag_get(uint8_t flag);

#endif	/* DEFAULT_APP_H */


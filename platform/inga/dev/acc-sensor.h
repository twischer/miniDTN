/**
 * \addtogroup inga_driver
 * @{
 */

/** 
 * \defgroup inga_acc_driver ADXL345 Accelerometer Sensor Driver
 * @{
 */

/**
 * \file 
 *		Accelerometer driver
 * \author
 *		
 */

#ifndef __ADXL345_SENSOR_H__
#define __ADXL345_SENSOR_H__

#include "lib/sensors.h"

extern const struct sensors_sensor acc_sensor;

/**
 * \name Channels
 * @{ */
#define ADXL345_X   0
#define ADXL345_Y   1
#define ADXL345_Z   2
/** @} */

/**
 * \name Sensitivity Values
 * \see ADXL345_SENSOR_SENSITIVITY
 * @{ */
/// 2g acceleration
#define ADXL345_2G  0x0
/// 4g acceleration
#define ADXL345_4G  0x1
/// 8g acceleration
#define ADXL345_8G  0x2
/// 16g acceleration
#define ADXL345_16G 0x2
/** @} */

/**
 * \name FIFO mode values
 * \see ADXL345_SENSOR_FIFOMODE
 * @{ */
/// Bypass mode
#define ADXL345_BYPASS  0x0
/// FIFO mode
#define ADXL345_FIFO    0x1
/// Stream mode
#define ADXL345_STREAM  0x2
/// Trigger mode
#define ADXL345_TRIGGER 0x3
/** @} */

/**
 * \name Power mode values
 * \see ADXL345_SENSOR_POWERMODE
 * @{ */
/// No sleep
#define ADXL345_NOSLEEP   0
/// Auto sleep
#define ADXL345_AUTOSLEEP 1

/** @} */

/**
 * \name Setting types
 * @{ */
/**
  Sensitivity configuration
 
 - \ref ADXL345_2G   2g @256LSB/g
 - \ref ADXL345_4G   4g @128LSB/g
 - \ref ADXL345_8G   8g @64LSB/g
 - \ref ADXL345_16G 16g @32LSB/g
 */
#define ADXL345_SENSOR_SENSITIVITY  10
/**
 * FIFO mode configuration 
 * 
 * - \ref ADXL345_BYPASS  - Bypass mode
 * - \ref ADXL345_FIFO		- FIFO mode
 * - \ref ADXL345_STREAM  - Stream mode
 * - \ref ADXL345_TRIGGER - Trigger mode
 */
#define ADXL345_SENSOR_FIFOMODE     20
/**
 * Controls power / sleep mode
 * Value	Mode
 * - \ref ADXL345_NOSLEEP			No sleep
 * - \ref ADXL345_AUTOSLEEP			Autosleep
 */
#define ADXL345_SENSOR_POWERMODE    30
/** @} */

#endif /* __ACC-SENSOR_H__ */

/** @} */
/** @} */

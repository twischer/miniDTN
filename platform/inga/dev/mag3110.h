#ifndef MAG3110_H_
#define MAG3110_H_

#include "../dev/i2c.h"

/* Magnetometer device address*/
#define MAG3110_ADDRESS		0x0E
#define MAG3110_DEV_ADDR_W	MAG3110_ADDRESS << 1
#define MAG3110_DEV_ADDR_R	(MAG3110_ADDRESS << 1 | 0x01)

/** \name Register addresses.
 *
 * Naming convention of the datasheet with prefix MAG3110_ is used.
 *
 * @{ */
/// Data ready status per axis
#define MAG3110_DR_STATUS	0x00
/// Bits [15:8] of X measurement
#define MAG3110_OUT_X_MSB	0x01
/// Bits [7:0] of X measurement
#define MAG3110_OUT_X_LSB	0x02
/// Bits [15:8] of Y measurement
#define MAG3110_OUT_Y_MSB	0x03
/// Bits [7:0] of Y measurement
#define MAG3110_OUT_Y_LSB	0x04
/// Bits [15:8] of Z measurement
#define MAG3110_OUT_Z_MSB	0x05
/// Bits [7:0] of Z measurement
#define MAG3110_OUT_Z_LSB	0x06
/// Device ID Number
#define MAG3110_WHO_AM_I	0x07
/// Current System Mode
#define MAG3110_SYSMOD		0x08
/// Bits [14:7] of user X offset
#define MAG3110_OFF_X_MSB	0x09
/// Bits [6:0] of user X offset
#define MAG3110_OFF_X_LSB	0x0A
/// Bits [14:7] of user Y offset
#define MAG3110_OFF_Y_MSB	0x0B
/// Bits [6:0] of user Y offset
#define MAG3110_OFF_Y_LSB	0x0C
/// Bits [14:7] of user Z offset
#define MAG3110_OFF_Z_MSB	0x0D
/// Bits [6:0] of user Z offset
#define MAG3110_OFF_Z_LSB	0x0E
/// Temperature, signed 8 bits in °C
#define MAG3110_DIE_TEMP	0x0F
/// Operation Modes
#define MAG3110_CTRL_REG1	0x10
/// Operation Modes
#define MAG3110_CTRL_REG2	0x11
/** @} */

/** \name Bit addresses for CTRL_REG1
 * @{ */
/// ADC Sample Rate 1280 Hz
#define MAG3110_DR_1280		0
/// ADC Sample Rate 640 Hz
#define MAG3110_DR_640		1
/// ADC Sample Rate 320 Hz
#define MAG3110_DR_320		2
/// ADC Sample Rate 160 Hz
#define MAG3110_DR_160		3
/// ADC Sample Rate 80 Hz
#define MAG3110_DR_80		4
/// ADC Sample Rate 40 Hz
#define MAG3110_DR_40		5
/// ADC Sample Rate 20 Hz
#define MAG3110_DR_20		6
/// ADC Sample Rate 10 Hz
#define MAG3110_DR_10		7

/// bitshift offset for ADC Sample Rate
#define MAG3110_DR_OFFSET	5
/// bitmask for ADC Sample Rate
#define MAG3110_DR_MASK		0xE0

/// 16x Oversampling
#define MAG3110_OS_16		0
/// 32x Oversampling
#define MAG3110_OS_32		1
/// 64x Oversampling
#define MAG3110_OS_64		2
/// 128x Oversampling
#define MAG3110_OS_128		3

/// bitshift offset for Oversampling
#define MAG3110_OS_OFFSET	3
/// bitmask for Oversampling
#define MAG3110_OS_MASK		0x18

/// Fast Read disabled
#define MAG3110_FR_OFF		0
/// Fast Read enabled
#define MAG3110_FR_ON		1
/// Fast Read selection
#define MAG3110_FR_BIT		2

/// Normal operation
#define MAG3110_TM_OFF		0
/// Trigger measurement
#define MAG3110_TM_ON		1
/// Trigger immediate measurement
#define MAG3110_TM_BIT		1

/// Device in standby mode
#define MAG3110_AC_STANDBY	0
/// Device is active (make periodic measurements)
#define MAG3110_AC_ACTIVE	1
/// Operating mode selection
#define MAG3110_AC_BIT		0
/** @} */

/** \name Bit addresses for CTRL_REG2.
 * @{ */
/// Automatic magnetic sensor resets disabled
#define MAG3110_AUTO_MRST_DISABLED	0
/// Automatic magnetic sensor resets enabled
#define MAG3110_AUTO_MRST_ENABLED	1
/// Automatic Magnetic Sensor Reset
#define MAG3110_AUTO_MRST_EN_BIT	7

/// Normal mode
#define MAG3110_RAW_NORMAL	0
/// Raw mode
#define MAG3110_RAW_RAW 	1
/// Data output correction
#define MAG3110_RAW_BIT		5

/// Reset cycle not active
#define MAG3110_MAG_RST_DONE	0
/// Reset cycle initiate or Reset cycle busy/active
#define MAG3110_MAG_RST_BUSY	1
/// Magnetic Sensor Reset (One-Shot)
#define MAG3110_MAG_RST_BIT		4
/** @} */

//--- types

/** Magnetometer data type. */
typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} mag_data_t;

//--- functions
/** Checks if mag3110 is avialable.
 * \retval 1 is available
 * \retval 0 not available
 */
uint8_t mag3110_available(void); // true if magnetometer is connected

/** Inits the magnetometer.
 * \retval 0 if init succeeded
 * \retval 1 if init failed
 */
int8_t mag3110_init(void);

/** Deinit the magnetometer. */
void mag3110_deinit(void);

/** Activate the magnetometer from standby. */
void mag3110_activate(void);

/** Deactivate the magnetometer to standby. */
void mag3110_deactivate(void);

/** Reads temperature value
 * @return 1 degree/digit
 */
int8_t mag3110_get_temp(void);

/** Reads data for x,y and z axis.
 *
 * @note If all channels are required, this function is faster than reading values with mag3110_get_x/y/z, because only a single readout is performed!
 */
mag_data_t mag3110_get(void);

/** Reads x axis value
 * @return x axis value
 */
int16_t mag3110_get_x(void);

/** Reads y axis value
 * @return y axis value
 */
int16_t mag3110_get_y(void);

/** Reads z axis value
 * @return z axis value
 */
int16_t mag3110_get_z(void);

/** Reads 2x8 bit register from magnetometers
 * @param addr address to read from (higher byte)
 * @return register content
 */
int16_t mag3110_read16bit(uint8_t addr);


/** Reads 8 bit register from magnetometers
 * @param addr address to read from
 * @return register content
 */
uint8_t mag3110_read8bit(uint8_t addr);

/** Writes 2x8 bit register to magnetometers
 * @param addr address to write to (higher byte)
 * @param data data to write
 */
void mag3110_write16bit(uint8_t addr, int16_t data);

/** Writes 8 bit to magnetometers register
 * @param addr address to write to
 * @param data data to write
 */
void mag3110_write8bit(uint8_t addr, uint8_t data);

#endif //MAG3110_H_

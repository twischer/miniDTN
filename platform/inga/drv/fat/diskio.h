#ifndef _DISKIO_H_
#define _DISKIO_H_
/** Allows raw access to disks, used my MBR-Subsystem*/

#define DISKIO_DEVICE_TYPE_NOT_RECOGNIZED 0
#define DISKIO_DEVICE_TYPE_SD_CARD 1
#define DISKIO_DEVICE_TYPE_GENERIC_FLASH 2
#define DISKIO_DEVICE_TYPE_FILE 4
#define DISKIO_DEVICE_TYPE_PARTITION 128

/** Bigger sectors then this are not supported. May be reduced down to 512 to use less memory. */
#define DISKIO_MAX_SECTOR_SIZE 512 

//Mask used to ignore modifiers like the PARTITION flag
#define DISKIO_DEVICE_TYPE_MASK 0x7f

#define DISKIO_DEBUG_FILE_NUM_SECTORS 2048
#define DISKIO_DEBUG_FILE_SECTOR_SIZE 512
#define DISKIO_DEBUG_FILE_NAME "diskio.img"

#define DISKIO_SUCCESS 0
#define DISKIO_ERROR_NO_DEVICE_SELECTED 1
#define DISKIO_ERROR_INTERNAL_ERROR 2
#define DISKIO_ERROR_DEVICE_TYPE_NOT_RECOGNIZED 3
#define DISKIO_ERROR_OPERATION_NOT_SUPPORTED 4
#define DISKIO_ERROR_TO_BE_IMPLEMENTED 5
#define DISKIO_FAILURE 6

#define DISKIO_MAX_DEVICES 8

#include <stdint.h>

/**
 * Stores the necessary information to identify a device using the diskio-Library.
 */
struct diskio_device_info {
	/** Specifies the recognized type of the memory */
	uint8_t type;
	/** Number to identify device */
	uint8_t number;
	/** Will be ignored if PARTITION flag is not set in type,
	 * otherwise tells which partition of this device is meant */
	uint8_t partition;
	/** Number of sectors on this device */
	uint32_t num_sectors;
	/** How big is one sector in bytes */
	uint16_t sector_size;
	/** If this is a Partition, this indicates which is the
	 * first_sector belonging to this partition on this device */
	uint32_t first_sector;
};

void print_device_info( struct diskio_device_info *dev );
/**
 * Reads one block from the specified device and stores it in buffer.
 *
 * \param *dev the pointer to the device info struct
 * \param block_address Which block should be read
 * \param *buffer buffer in which the data is written
 * \return DISKIO_SUCCESS on success, otherwise not 0 for an error
 */
int diskio_read_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer );

/**
 * Reads multiple blocks from the specified device.
 *
 * This may or may not be supported for every device. DISKIO_ERROR_OPERATION_NOT_SUPPORTED will be
 * returned in that case.
 * \param *dev the pointer to the device info
 * \param block_start_address the address of the first block to be read
 * \param num_blocks the number of blocks to be read
 * \param *buffer buffer in which the data is written
 * \return DISKIO_SUCCESS on success, otherwise not 0 for an error
 */
int diskio_read_blocks( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer );

/**
 * Writes a single block to the specified device
 *
 * \param *dev the pointer to the device info
 * \param block_address address where the block should be written
 * \param *buffer buffer in which the data is stored
 * \return DISKIO_SUCCESS on success, !0 on error
 */
int diskio_write_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer );

/**
 * Writes multiple blocks to the specified device
 *
 * \param *dev the pointer to the device info
 * \param block_start_address the address of the first block to be written
 * \param num_blocks number of blocks to be written
 * \param *buffer buffer where the data is stored
 * \return DISKIO_SUCCESS on success, !0 on error
 */
int diskio_write_blocks( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer );

/**
 * Returns the device-Database.
 *
 * Number of devices in the database is limited by the DISKIO_MAX_DEVICES define.
 * \return pointer to the Array of device infos. No available device entries have the type set to DISKIO_DEVICE_TYPE_NOT_RECOGNIZED.
 */
struct diskio_device_info * diskio_devices();

/**
 * Creates the internal database of available devices.
 *
 * Adds virtual devices for multiple Partitions on devices using the MBR-Library.
 * Number of devices in the database is limited by the DISKIO_MAX_DEVICES define.
 * Warning the device numbers may change when calling this function. It should be called
 * once on start but may also be called, if the microSD-Card was ejected to update the device
 * database.
 */
int diskio_detect_devices();

/**
 * Sets the default operation device.
 *
 * This allows to call the diskio_* functions with the dev parameter set to NULL.
 * The functions will then use the default device to operate.
 * \param *dev the pointer to the device info, which will be the new default device
 */
void diskio_set_default_device( struct diskio_device_info *dev );

#endif

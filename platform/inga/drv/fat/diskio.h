#ifndef _DISKIO_H_
#define _DISKIO_H_
/** Allows raw access to disks, used my MBR-Subsystem*/

#define diskio_device_type_NOT_RECOGNIZED 0
#define diskio_device_type_SD_CARD 1
#define diskio_device_type_GENERIC_FLASH 2
#define diskio_device_type_PARTITION 4

#define DISKIO_SUCCESS 0
#define DISKIO_ERROR_NO_DEVICE_SELECTED 1
#define DISKIO_ERROR_INTERNAL_ERROR 2
#define DISKIO_ERROR_DEVICE_TYPE_NOT_RECOGNIZED 3
#define DISKIO_ERROR_OPERATION_NOT_SUPPORTED 4
#define DISKIO_ERROR_TO_BE_IMPLEMENTED 5

#define DISKIO_MAX_DEVICES 8

struct diskio_device_info{
	/** Specifiey the recognized type of the memory*/
	uint8_t type;
	/** Number to identify device*/
	uint8_t number;
	/** Will be ignored if PARTITION flag is not set in type, otherwise tells which partition of this device is meant*/
	uint8_t partition;
};

int diskio_read_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer );
int diskio_read_blocks( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer );
int diskio_write_block( struct diskio_device_info *dev, uint32_t block_address, uint8_t *buffer );
int diskio_write_blocks( struct diskio_device_info *dev, uint32_t block_start_address, uint8_t num_blocks, uint8_t *buffer );

/** Implementation should use mbr_read to automatically add new devices */
struct diskio_device_info * diskio_devices();

/** sets to which device normally reads and writes go */
int diskio_set_default_device( struct diskio_device_info *dev );

#endif
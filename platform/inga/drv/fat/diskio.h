#ifndef _DISKIO_H_
#define _DISKIO_H_
/** Allows raw access to disks, used my MBR-Subsystem*/

#define diskio_device_type_NOT_RECOGNIZED 0
#define diskio_device_type_SD_CARD 1
#define diskio_device_type_GENERIC_FLASH 2
#define diskio_device_type_PARTITION 4

struct diskio_device_info{
	/** Specifiey the recognized type of the memory*/
	uint8_t type;
	/** Number to identify device*/
	uint8_t number;
	/** Will be ignored if PARTITION flag is not set in type, otherwise tells which partition of this device is meant*/
	uint8_t partition;
};

int diskio_read_block();
int diskio_read_blocks();
int diskio_write_block();
int diskio_write_blocks();

/** Implementation should use mbr_read to automatically add new devices */
struct diskio_device_info * diskio_devices();

/** sets to which device normally reads and writes go */
int diskio_set_current_device();

#endif
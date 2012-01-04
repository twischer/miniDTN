#include "../diskio.h"
#include <string.h>

int main() {
	uint8_t buffer[DISKIO_DEBUG_FILE_SECTOR_SIZE];
	struct diskio_device_info *info = 0;
	uint16_t i = 0;
	memset(buffer, 0, DISKIO_DEBUG_FILE_SECTOR_SIZE);
	diskio_detect_devices();
	info = diskio_devices();
	for(i = 0; i < DISKIO_MAX_DEVICES; i++) {
		if( (info + i)->type == DISKIO_DEVICE_TYPE_FILE ) {
			info += i;
			break;
		}
	}
	if( i == DISKIO_MAX_DEVICES )
		return 1;
	diskio_write_block( info, 0, buffer );
	memset(buffer, 1, DISKIO_DEBUG_FILE_SECTOR_SIZE);
	diskio_read_block( info, 0, buffer );
	for( i = 0; i < DISKIO_DEBUG_FILE_SECTOR_SIZE; i++ ) {
		if( buffer[i] != 0 )
			return 2;
	}
	return 0;
}

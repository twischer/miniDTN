/**
 * \file
 *	   FAT FS test case
 * \author
 *	   Timo Wischer
 */

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "ff.h"

#include "contiki.h"
#include "net/netstack.h"
#include "net/packetbuf.h"

#include "dtn_process.h"

const char filename[] = "test.bin";


static void udtn_sink_process(void* p)
{
	static uint8_t data[178];
	for (int i=0; i<sizeof(data); i++) {
		data[i] = (i & 0xFF);
	}

	while (true) {
		{
			FIL fd_write;
			const FRESULT res = f_open(&fd_write, filename, FA_CREATE_ALWAYS | FA_WRITE);
			if (res != FR_OK) {
				printf("Unable to open file, abort here\n");
				return;
			}

			// Write our complete bundle
			UINT bytes_written = 0;
			const FRESULT ret = f_write(&fd_write, data, sizeof(data), &bytes_written);
			if(ret != FR_OK || bytes_written != sizeof(data)) {
				printf("unable to write %u bytes to file %s, aborting (err %u, written %u)\n",
					sizeof(data), filename, res, bytes_written);
				f_unlink(filename);
				return;
			}

			const FRESULT close_res = f_close(&fd_write);
			if(close_res != FR_OK) {
				printf("unable to close file %s (err %u)\n", filename, close_res);
				return;
			}

//			printf("File %s wirtten\n", filename);
		}
//		vTaskDelay(pdMS_TO_TICKS(1));


		{
			FIL fd_read;
			const FRESULT res = f_open(&fd_read, filename, FA_OPEN_EXISTING | FA_READ);
			if (res != FR_OK) {
				printf("Unable to open file, abort here\n");
				return;
			}

			uint8_t read_data[512];
			UINT bytes_read = 0;
			const FRESULT ret = f_read(&fd_read, read_data, sizeof(read_data), &bytes_read);
			if(ret != FR_OK || bytes_read != sizeof(data)) {
				printf("unable to read %u bytes from file %s, aborting\n", sizeof(data), filename);
				return;
			}

			const FRESULT close_res = f_close(&fd_read);
			if(close_res != FR_OK) {
				printf("unable to close file %s (err %u)\n", filename, close_res);
				return;
			}

			/* compare file contant */
			for (int i=0; i<sizeof(data); i++) {
				if (data[i] != read_data[i]) {
					printf("File contant differs at %u (desired %x, actual %x)\n", i, data[i], read_data[i]);
				}
			}

//			printf("File %s read\n", filename);
		}
//		vTaskDelay(pdMS_TO_TICKS(1));

		static uint32_t counter = 0;
		counter++;

		if (counter % 10 == 0) {
			printf("%lu repeated\n", counter);
		}
	}
}
/*---------------------------------------------------------------------------*/

bool init()
{
	if ( !dtn_process_create_other_stack(udtn_sink_process, "FAT FS test", configFATFS_USING_TASK_STACK_DEPTH) ) {
		return false;
	}

  return true;
}

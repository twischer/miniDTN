/*
 * \brief Example application which sends an incremented counter with each bundle
 * To test it, run
 * $ dtnrecv --name 26 --count 1000
 * on destination DTN node.
 *
 * \author Timo Wischer <t.wischer@tu-bs.de>
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "net/uDTN/dtn_process.h"
#include "net/uDTN/agent.h"


#define DEST_NODE_EID    10
#define DEST_ENDPOINT    26

#define DTN_SERVICE      (DEST_ENDPOINT + 1)


void counter_process(void* p)
{
	/* Give agent time to initialize */
	vTaskDelay( pdMS_TO_TICKS(1000) );

	/* Register counting endpoint */
	static struct registration_api reg;
	reg.status = APP_ACTIVE;
	reg.event_queue = dtn_process_get_event_queue();
	reg.app_id = DTN_SERVICE;
	const event_container_t reg_event = {
		.event = dtn_application_registration_event,
		.registration = &reg
	};
	agent_send_event(&reg_event);

	printf("main: queue=%p app=%lu\n",reg.event_queue, reg.app_id);
	printf("Counter sending to ipn:%u.%u\n", DEST_NODE_EID, DEST_ENDPOINT);

	while (1) {
		vTaskDelay( pdMS_TO_TICKS(1000) );

		struct mmem* const bundlemem = bundle_create_bundle();
		if (bundlemem == NULL) {
			printf("create_bundle failed\n");
			continue;
		}

		const uint32_t dest_node = DEST_NODE_EID;
		bundle_set_attr(bundlemem, DEST_NODE, &dest_node);

		const uint32_t dest_service = DEST_ENDPOINT;
		bundle_set_attr(bundlemem, DEST_SERV, &dest_service);

		static uint32_t counter = 0;
		/* More than 3 bits are needed to represent one digit.
		 * One character is needed for the line break.
		 * A second character is needed for the terminating zero.
		 */
		static char payload_buffer[(sizeof(counter) * 8 / 3) + 2];
		const int length = snprintf(payload_buffer, sizeof(payload_buffer), "%lu\n", counter);
		if (length <= 0) {
			printf("Counter conversion failed\n");
			continue;
		} else if (length >= sizeof(payload_buffer)) {
			printf("Payload buffer to small\n");
			continue;
		}
		counter++;

		/* add the counter to the payload of the bundle */
		bundle_add_block(bundlemem, BUNDLE_BLOCK_TYPE_PAYLOAD, BUNDLE_BLOCK_FLAG_NULL, (uint8_t*)payload_buffer, length);


		const event_container_t send_event = {
			.event = dtn_send_bundle_event,
			.bundlemem = bundlemem
		};
		agent_send_event(&send_event);
	}
}


bool init()
{
	if ( !dtn_process_create(counter_process, "DTN Counter") ) {
		return false;
	}

  return true;
}

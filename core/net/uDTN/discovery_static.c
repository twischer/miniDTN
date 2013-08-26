/**
 * \addtogroup discovery
 * @{
 */

/**
 * \defgroup discovery_static Static discovery module
 *
 * @{
 */

/**
 * \file
 * \brief Static discovery Module
 *
 * \author Georg von Zengen <vonzeng@ibr.cs.tu-bs.de>
 * \author Wolf-Bastian Poettner <poettner@ibr.cs.tu-bs.de>
 */

#include "net/rime/rimeaddr.h"

#include "discovery.h"

void discovery_static_init()
{

}

uint8_t discovery_static_is_neighbour(rimeaddr_t * dest)
{
	return 1;
}

void discovery_static_enable()
{

}

void discovery_static_disable()
{

}

void discovery_static_receive(rimeaddr_t * source, uint8_t * payload, uint8_t length)
{

}

void discovery_static_alive(rimeaddr_t * source)
{

}

uint8_t discovery_static_discover(rimeaddr_t * dest)
{
	return 1;
}

struct discovery_neighbour_list_entry * discovery_static_list_neighbours()
{
	return NULL;
}

void discovery_static_stop_pending()
{

}

void discovery_static_start(clock_time_t duration, uint8_t index)
{
}

void discovery_static_stop()
{
}

void discovery_static_clear()
{
}

const struct discovery_driver discovery_static ={
		.name = "S_DISCOVERY",
		.init = discovery_static_init,
		.is_neighbour = discovery_static_is_neighbour,
		.enable = discovery_static_enable,
		.disable = discovery_static_disable,
		.receive = discovery_static_receive,
		.alive = discovery_static_alive,
		.dead = NULL,
		.discover = discovery_static_discover,
		.neighbours = discovery_static_list_neighbours,
		.stop_pending = discovery_static_stop_pending,
		.start        = discovery_static_start,
		.stop         = discovery_static_stop,
		.clear        = discovery_static_clear,
};


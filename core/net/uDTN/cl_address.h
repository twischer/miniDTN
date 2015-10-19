#ifndef CL_ADDRESS_H
#define CL_ADDRESS_H

#include <stdbool.h>
#include <string.h>
#include "lwip/ip_addr.h"
#include "net/linkaddr.h"

/**
 * Defines maximal length of an ip string
 * e.g 255.255.255.255:65535
 */
#define CL_ADDR_STRING_LENGTH	22


/* convergence_layers.h could not included here,
 * becasue it would result in an infinite recursion
 */
struct convergence_layer;


/**
 * Unified destination addresses
 * of the DTN neighbours
 */
typedef struct {
	const struct convergence_layer* clayer;
	bool isIP;
	union {
		linkaddr_t lowpan;

		struct {
			ip_addr_t ip;
			uint16_t port;
		};
	};
} cl_addr_t;



bool cl_addr_cmp(const cl_addr_t* const src1, const cl_addr_t* const src2);
int cl_addr_string(const cl_addr_t* const addr, char* const buf, const size_t buflen);


static inline void cl_addr_copy(cl_addr_t* const dest, const cl_addr_t* const src)
{
	memcpy(dest, src, sizeof(cl_addr_t));
}

#endif // CL_ADDRESS_H

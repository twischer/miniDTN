#include "cl_address.h"
#include "convergence_layer_lowpan_dgram.h"
#include "convergence_layer_udp_dgram.h"


int cl_addr_build_lowpan_dgram(const linkaddr_t* const addr, cl_addr_t* const cl_addr)
{
	cl_addr->clayer = &clayer_lowpan_dgram;
	cl_addr->isIP = false;
	linkaddr_copy(&cl_addr->lowpan, addr);

	return 1;
}


int cl_addr_build_udp_dgram(const ip_addr_t* const addr, const uint16_t port, cl_addr_t* const cl_addr)
{
	cl_addr->clayer = &clayer_udp_dgram;
	cl_addr->isIP = true;
	ip_addr_copy(cl_addr->ip, *addr);
	cl_addr->port = port;

	return 1;
}


bool cl_addr_cmp(const cl_addr_t* const src1, const cl_addr_t* const src2)
{
	if (src1->isIP != src2->isIP) {
		return false;
	}

	if (src1->isIP) {
		if ( !ip_addr_cmp(&src1->ip, &src2->ip) ) {
			return false;
		}

		return (src1->port == src2->port);
	} else {
		return linkaddr_cmp(&src1->lowpan, &src2->lowpan);
	}
}


int cl_addr_string(const cl_addr_t* const addr, char* const buf, const size_t buflen)
{
	if (addr->isIP) {
		// TODO nach IP2STR und IPSTR suchen
		if (ipaddr_ntoa_r(&addr->ip, buf, buflen) == NULL) {
			return -1;
		}

		const size_t ip_str_len = strlen(buf);
		const size_t port_buf_len = buflen - ip_str_len;
		char* const port_buf = &buf[ip_str_len];
		const int ret = snprintf(port_buf, port_buf_len, ":%u", addr->port);
		if (ret < 0) {
			/* there was an encoding error */
			return -2;
		} else if (ret >=port_buf_len) {
			/* the buffer was too small */
			return -3;
		}
	} else {
		const int ret = snprintf(buf, buflen, "pan:%u", ntohs(addr->lowpan.u16));
		if (ret < 0) {
			/* there was an encoding error */
			return -2;
		} else if (ret >=buflen) {
			/* the buffer was too small */
			return -3;
		}
	}

	return 0;
}


#ifndef __SPROFILING_H__
#define __SPROFILING_H__

#include <stdint.h>

/* The structure that holds the callsites */
struct sprofile_site_t {
	void *addr;
	uint16_t calls;
};

struct sprofile_t {
	uint16_t max_sites;
	uint16_t num_sites;
	uint32_t num_samples;
	struct sprofile_site_t *sites;
};



void sprofiling_init(void);
void sprofiling_start(void);
void sprofiling_stop(void);
void sprofiling_report(const char* name, uint8_t pretty);
struct sprofile_t *sprofiling_get(void);
inline void sprofiling_add_sample(void *pc);

/* Arch functions */
void sprofiling_arch_init(void);
inline void sprofiling_arch_start(void);
inline void sprofiling_arch_stop(void);

#endif /* __SPROFILING_H__ */

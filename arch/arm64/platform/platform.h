#ifndef NXU_PLATFORM_H
#define NXU_PLATFORM_H

#include <nxu/types.h>

/*
 * Architecture map
 *
 *   platform description
 *         |
 *         +-- GIC distributor base
 *         +-- GIC redistributor base
 *         +-- redistributor stride
 *
 * Platform only describes where hardware lives.
 * It does not configure hardware.
 */

struct nxu_platform {
    nxu_uptr gic_distributor_base;
    nxu_uptr gic_redistributor_base;
    nxu_uptr gic_redistributor_stride; /* 0 = use hardware default */
};

int nxu_platform_init(void);
int nxu_platform_memory_init(void);
const struct nxu_platform *nxu_platform_get(void);

#endif
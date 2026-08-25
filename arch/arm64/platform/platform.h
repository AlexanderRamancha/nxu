#ifndef NXU_PLATFORM_H
#define NXU_PLATFORM_H

#include <nxu/types.h>

/*
 * Architecture map
 *
 * Platform layer
 *
 *     platform description
 *            |
 *            +-- GIC Distributor
 *            |
 *            +-- GIC Redistributors
 *            |
 *            +-- CPU topology
 *
 * The platform layer describes where hardware exists.
 *
 * It does not configure the hardware.
 */

struct nxu_platform {

    nxu_uptr gic_distributor_base;
    nxu_uptr gic_redistributor_base;
};

int
nxu_platform_init(void);

const struct nxu_platform *
nxu_platform_get(void);

#endif
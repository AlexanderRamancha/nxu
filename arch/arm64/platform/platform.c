#include "platform.h"

/*
 * Architecture map
 *
 *     ARM64 platform
 *           |
 *           v
 *     platform description
 *           |
 *           v
 *       GIC addresses
 *
 * Current provider:
 *
 *     QEMU virt
 *
 * These values will later be supplied by
 * firmware / DTB instead of being compiled here.
 */

#define NXU_PLATFORM_GICD_BASE \
    ((nxu_uptr)0x08000000UL)

#define NXU_PLATFORM_GICR_BASE \
    ((nxu_uptr)0x080A0000UL)


static struct nxu_platform platform;


int
nxu_platform_init(void)
{
    platform.gic_distributor_base =
        NXU_PLATFORM_GICD_BASE;

    platform.gic_redistributor_base =
        NXU_PLATFORM_GICR_BASE;

    return 0;
}


const struct nxu_platform *
nxu_platform_get(void)
{
    return &platform;
}
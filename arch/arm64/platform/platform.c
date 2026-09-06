#include "platform.h"

#include <nxu/memory.h>


/*
 * ============================================================================
 * QEMU virt platform memory
 * ============================================================================
 *
 * This is PLATFORM knowledge.
 *
 * It must NOT be used directly by generic NXU memory or DTB code.
 *
 * QEMU virt currently provides:
 *
 *     RAM base = 0x40000000
 *     RAM size = 0x20000000
 *
 *     RAM end  = 0x60000000
 */

#define NXU_PLATFORM_RAM_BASE \
    ((nxu_uptr)0x40000000UL)

#define NXU_PLATFORM_RAM_SIZE \
    ((nxu_u64)0x20000000ULL)


/*
 * Existing GIC platform constants.
 */

#define NXU_PLATFORM_GICD_BASE \
    ((nxu_uptr)0x08000000UL)

#define NXU_PLATFORM_GICR_BASE \
    ((nxu_uptr)0x080A0000UL)

#define NXU_PLATFORM_GICR_STRIDE \
    ((nxu_uptr)0x20000UL)


static struct nxu_platform platform;


int
nxu_platform_memory_init(void)
{
    if (nxu_memory_init() != 0)
        return -1;

    if (
        nxu_memory_add_region(
            NXU_PLATFORM_RAM_BASE,
            NXU_PLATFORM_RAM_SIZE
        ) != 0
    )
        return -1;

    return 0;
}


int
nxu_platform_init(void)
{
    platform.gic_distributor_base =
        NXU_PLATFORM_GICD_BASE;

    platform.gic_redistributor_base =
        NXU_PLATFORM_GICR_BASE;

    platform.gic_redistributor_stride =
        NXU_PLATFORM_GICR_STRIDE;

    return 0;
}


const struct nxu_platform *
nxu_platform_get(void)
{
    return &platform;
}
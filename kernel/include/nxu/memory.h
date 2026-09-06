#ifndef NXU_MEMORY_H
#define NXU_MEMORY_H

#include <nxu/types.h>

/*
 * ============================================================================
 * NXU PHYSICAL MEMORY MAP
 * ============================================================================
 *
 * A memory region describes physical memory that NXU is currently allowed
 * to access.
 *
 * "base" = first physical address in the region.
 * "size" = number of bytes in the region.
 *
 * The valid range is:
 *
 *     [base, base + size)
 *
 * The end address itself is NOT part of the region.
 */

struct nxu_memory_region {
    nxu_uptr base;
    nxu_u64 size;
};


/*
 * Maximum number of physical memory regions known during early boot.
 *
 * This is intentionally small for the prototype.
 */
#define NXU_MAX_MEMORY_REGIONS 8U


/*
 * Initialize the early NXU physical memory map.
 *
 * Platform-specific code calls this to publish the memory ranges that
 * are safe to access during early boot.
 */
int nxu_memory_init(void);


/*
 * Add one physical memory region to the NXU memory map.
 */
int nxu_memory_add_region(
    nxu_uptr base,
    nxu_u64 size
);


/*
 * Check whether the COMPLETE physical range:
 *
 *     [address, address + size)
 *
 * lies inside one known NXU memory region.
 *
 * Returns:
 *
 *     0  = valid
 *    -1  = invalid
 */
int nxu_memory_is_valid(
    nxu_uptr address,
    nxu_u64 size
);

#endif
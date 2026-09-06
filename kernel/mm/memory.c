#include <nxu/memory.h>


/*
 * ============================================================================
 * EARLY NXU PHYSICAL MEMORY MAP
 * ============================================================================
 *
 * This table contains only memory that NXU has explicitly established as
 * accessible during the current boot stage.
 */

static struct nxu_memory_region memory_regions[
    NXU_MAX_MEMORY_REGIONS
];

static nxu_u32 memory_region_count;


/*
 * ============================================================================
 * nxu_memory_init
 * ============================================================================
 *
 * Reset the early memory map.
 *
 * Platform-specific code will populate it afterwards.
 */

int
nxu_memory_init(void)
{
    nxu_u32 i;

    memory_region_count = 0U;

    for (i = 0U; i < NXU_MAX_MEMORY_REGIONS; i++) {
        memory_regions[i].base = 0U;
        memory_regions[i].size = 0U;
    }

    return 0;
}


/*
 * ============================================================================
 * nxu_memory_add_region
 * ============================================================================
 */

int
nxu_memory_add_region(
    nxu_uptr base,
    nxu_u64 size
)
{
    if (size == 0U)
        return -1;

    if (memory_region_count >= NXU_MAX_MEMORY_REGIONS)
        return -1;

    /*
     * Physical address arithmetic must not wrap.
     *
     * The region end must fit inside the address space represented
     * by nxu_uptr.
     *
     * Since nxu_uptr is unsigned, this comparison catches:
     *
     *     base + size > UINTPTR_MAX
     */
    if (size > ((nxu_u64)(~(nxu_uptr)0) - (nxu_u64)base))
        return -1;

    memory_regions[memory_region_count].base = base;
    memory_regions[memory_region_count].size = size;

    memory_region_count++;

    return 0;
}


/*
 * ============================================================================
 * nxu_memory_is_valid
 * ============================================================================
 *
 * Check whether the COMPLETE requested range belongs to one known
 * memory region.
 *
 * We deliberately avoid:
 *
 *     address + size
 *
 * when possible because unsigned address arithmetic can wrap around.
 */

int
nxu_memory_is_valid(
    nxu_uptr address,
    nxu_u64 size
)
{
    nxu_u32 i;

    if (size == 0U)
        return -1;

    for (i = 0U; i < memory_region_count; i++) {

        nxu_uptr base;
        nxu_u64 region_size;
        nxu_u64 offset;

        base = memory_regions[i].base;
        region_size = memory_regions[i].size;

        /*
         * Requested range must start inside this region.
         */
        if (address < base)
            continue;

        /*
         * offset = distance from region start to requested start.
         *
         * Because address >= base, this subtraction cannot underflow.
         */
        offset =
            (nxu_u64)address -
            (nxu_u64)base;

        /*
         * Entire requested range must fit.
         *
         * Instead of:
         *
         *     address + size <= region_end
         *
         * we use:
         *
         *     offset <= region_size
         *     size   <= region_size - offset
         *
         * This avoids address overflow.
         */
        if (offset > region_size)
            continue;

        if (size > (region_size - offset))
            continue;

        return 0;
    }

    return -1;
}
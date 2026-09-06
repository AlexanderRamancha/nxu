#ifndef NXU_DTB_H
#define NXU_DTB_H

#include <nxu/types.h>


/*
 * ============================================================================
 * Flattened Device Tree (FDT) header
 * ============================================================================
 *
 * DTB = Device Tree Blob.
 *
 * FDT = Flattened Device Tree.
 *
 * The DTB is a binary representation of the hardware description.
 *
 * IMPORTANT:
 *
 * Multi-byte DTB fields are stored in BIG-ENDIAN byte order.
 */


/*
 * FDT magic value.
 *
 * On the wire/in memory:
 *
 *     D0 0D FE ED
 *
 * Numerically:
 *
 *     0xD00DFEED
 */
#define NXU_DTB_MAGIC 0xD00DFEEDU


/*
 * Size of the FDT header.
 */
#define NXU_DTB_HEADER_SIZE \
    40U


/*
 * Minimum supported FDT version for this validator.
 */
#define NXU_DTB_MIN_VERSION \
    17U


/*
 * Maximum FDT version understood by this implementation.
 *
 * Version 17 is the currently understood structure format for this
 * prototype validator.
 */
#define NXU_DTB_MAX_VERSION \
    17U


/*
 * FDT header.
 *
 * IMPORTANT:
 *
 * These values are stored in big-endian form in the DTB.
 *
 * We therefore do NOT directly trust the C integer values after loading
 * them from memory. They must pass through nxu_dtb_be32().
 */
struct nxu_dtb_header {
    nxu_u32 magic;
    nxu_u32 totalsize;
    nxu_u32 off_dt_struct;
    nxu_u32 off_dt_strings;
    nxu_u32 off_mem_rsvmap;
    nxu_u32 version;
    nxu_u32 last_comp_version;
    nxu_u32 boot_cpuid_phys;
    nxu_u32 size_dt_strings;
    nxu_u32 size_dt_struct;
};


/*
 * Validate a DTB supplied at a physical address.
 *
 * Returns:
 *
 *     0  = valid
 *    -1  = invalid
 */
int nxu_dtb_validate(
    nxu_uptr dtb_address
);

#endif
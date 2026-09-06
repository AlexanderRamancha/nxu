#include <nxu/dtb.h>
#include <nxu/memory.h>


/*
 * ============================================================================
 * BIG-ENDIAN CONVERSION
 * ============================================================================
 *
 * DTB fields are stored in big-endian order.
 *
 * NXU currently runs on little-endian ARM64, so a value read directly
 * from memory needs its byte order reversed.
 *
 * Example:
 *
 *     DTB bytes:
 *
 *         D0 0D FE ED
 *
 *     raw little-endian CPU load:
 *
 *         0xEDFE0DD0
 *
 *     after conversion:
 *
 *         0xD00DFEED
 */

static nxu_u32
nxu_dtb_be32(nxu_u32 value)
{
    return
        ((value & 0x000000FFU) << 24) |
        ((value & 0x0000FF00U) << 8)  |
        ((value & 0x00FF0000U) >> 8)  |
        ((value & 0xFF000000U) >> 24);
}


/*
 * ============================================================================
 * SAFE RANGE CHECK INSIDE THE DTB
 * ============================================================================
 *
 * Checks:
 *
 *     [offset, offset + size)
 *
 * against:
 *
 *     [0, totalsize)
 *
 * without performing unsafe addition.
 */

static int
nxu_dtb_range_valid(
    nxu_u32 offset,
    nxu_u32 size,
    nxu_u32 totalsize
)
{
    if (offset > totalsize)
        return -1;

    if (size > (totalsize - offset))
        return -1;

    return 0;
}


/*
 * ============================================================================
 * nxu_dtb_validate
 * ============================================================================
 */

int
nxu_dtb_validate(
    nxu_uptr dtb_address
)
{
    struct nxu_dtb_header *header;

    nxu_u32 magic;
    nxu_u32 totalsize;
    nxu_u32 off_dt_struct;
    nxu_u32 off_dt_strings;
    nxu_u32 off_mem_rsvmap;
    nxu_u32 version;
    nxu_u32 last_comp_version;
    nxu_u32 size_dt_strings;
    nxu_u32 size_dt_struct;


    /*
     * ------------------------------------------------------------------------
     * STEP 1
     * ------------------------------------------------------------------------
     *
     * A zero address can never be a valid DTB address.
     */

    if (dtb_address == 0U)
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 2
     * ------------------------------------------------------------------------
     *
     * Before reading even ONE header byte, prove that the header itself
     * is inside memory NXU is allowed to access.
     *
     * This is the first important security boundary.
     */

    if (
        nxu_memory_is_valid(
            dtb_address,
            NXU_DTB_HEADER_SIZE
        ) != 0
    )
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 3
     * ------------------------------------------------------------------------
     *
     * Now it is safe for this early-boot implementation to access the
     * header.
     *
     * MMU/page-table setup is not yet part of this stage, so the prototype
     * uses the physical address directly.
     */

    header =
        (struct nxu_dtb_header *)(nxu_uptr)dtb_address;


    /*
     * ------------------------------------------------------------------------
     * STEP 4
     * ------------------------------------------------------------------------
     *
     * Convert fields from DTB big-endian format.
     */

    magic =
        nxu_dtb_be32(header->magic);

    totalsize =
        nxu_dtb_be32(header->totalsize);

    off_dt_struct =
        nxu_dtb_be32(header->off_dt_struct);

    off_dt_strings =
        nxu_dtb_be32(header->off_dt_strings);

    off_mem_rsvmap =
        nxu_dtb_be32(header->off_mem_rsvmap);

    version =
        nxu_dtb_be32(header->version);

    last_comp_version =
        nxu_dtb_be32(header->last_comp_version);

    size_dt_strings =
        nxu_dtb_be32(header->size_dt_strings);

    size_dt_struct =
        nxu_dtb_be32(header->size_dt_struct);


    /*
     * ------------------------------------------------------------------------
     * STEP 5 — MAGIC
     * ------------------------------------------------------------------------
     */

    if (magic != NXU_DTB_MAGIC)
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 6 — TOTAL SIZE
     * ------------------------------------------------------------------------
     *
     * totalsize tells us how many bytes belong to the complete DTB.
     *
     * It must at least contain the FDT header.
     */

    if (totalsize < NXU_DTB_HEADER_SIZE)
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 7 — DTB MUST FIT IN NXU MEMORY
     * ------------------------------------------------------------------------
     *
     * We already know the start address is valid.
     *
     * Now prove the COMPLETE DTB fits.
     */

    if (
        nxu_memory_is_valid(
            dtb_address,
            (nxu_u64)totalsize
        ) != 0
    )
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 8 — VERSION
     * ------------------------------------------------------------------------
     */

    if (version < NXU_DTB_MIN_VERSION)
        return -1;

    if (version > NXU_DTB_MAX_VERSION)
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 9 — COMPATIBILITY VERSION
     * ------------------------------------------------------------------------
     *
     * last_comp_version says which older FDT version is the oldest version
     * required for compatibility.
     *
     * It must not claim compatibility with a newer format than the DTB
     * itself uses.
     */

    if (last_comp_version > version)
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 10 — STRUCTURE BLOCK
     * ------------------------------------------------------------------------
     *
     * The structure block must fit completely inside totalsize.
     */

    if (
        nxu_dtb_range_valid(
            off_dt_struct,
            size_dt_struct,
            totalsize
        ) != 0
    )
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 11 — STRINGS BLOCK
     * ------------------------------------------------------------------------
     */

    if (
        nxu_dtb_range_valid(
            off_dt_strings,
            size_dt_strings,
            totalsize
        ) != 0
    )
        return -1;


    /*
     * ------------------------------------------------------------------------
     * STEP 12 — RESERVED MEMORY MAP LOCATION
     * ------------------------------------------------------------------------
     *
     * FDT reserved-memory map entries are 64-bit address + 64-bit size,
     * so the map must begin on an 8-byte boundary.
     */

    if ((off_mem_rsvmap & 7U) != 0U)
        return -1;

    if (off_mem_rsvmap >= totalsize)
        return -1;


    /*
     * ------------------------------------------------------------------------
     * DTB v1 VALID
     * ------------------------------------------------------------------------
     */

    return 0;
}
#ifndef NXU_BOOT_H
#define NXU_BOOT_H

#include <nxu/types.h>

/*
 * NXU boot contract
 *
 * This structure represents information that the boot
 * environment provides to NXU.
 *
 * Future NXU Bootloader:
 *
 *     Bootloader
 *          |
 *          v
 *     nxu_boot_info
 *          |
 *          v
 *       NXU kernel
 */

#define NXU_BOOT_FLAG_DTB_VALID   (1U << 0)

struct nxu_boot_info {
    nxu_u64 boot_cpu_affinity;
    nxu_uptr dtb_address;
    nxu_u32 flags;
};

int nxu_boot_validate(const struct nxu_boot_info *boot_info);

#endif
/*
 * NXU MAP
 * Layer: silicon translate (ops API)
 * Owns: map/enable prototypes and perm codes
 * Consumes: PA + perm from foundation
 * Produces: op prototypes
 * Hardware: stage-1
 * Forbidden: ownership policy
 * Invariant: operations only
 */
#ifndef NXU_MMU_H
#define NXU_MMU_H

#include <nxu/types.h>

#define NXU_MAP_RX      1U
#define NXU_MAP_R       2U
#define NXU_MAP_RW      3U
#define NXU_MAP_DEV     4U
#define NXU_MAP_USER_RX 5U
#define NXU_MAP_USER_RW 6U

void nxu_mmu_init_pool(void);
void nxu_mmu_map(nxu_u64 pa, nxu_u64 size, nxu_u32 perm);
void nxu_mmu_tlbi(void);
nxu_u64 nxu_mmu_leaf(nxu_u64 pa);
void nxu_mmu_enable(nxu_u32 el);
nxu_u64 nxu_mmu_probe(nxu_u64 va);
nxu_u64 nxu_mmu_ttbr(void);
nxu_u64 nxu_mmu_tcr(void);
nxu_u64 nxu_mmu_mair(void);

#endif

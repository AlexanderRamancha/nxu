/*
 * NXU MAP
 * Layer: foundation space (type)
 * Owns: ownership table layout
 * Forbidden: mapping unknown
 * Invariant: owner classes as inv. 25
 */
#ifndef NXU_SPACE_H
#define NXU_SPACE_H

#include <nxu/types.h>
#include <nxu/config.h>
#include <nxu/range.h>

struct nxu_owner_region {
	struct nxu_range range;
	nxu_u32 owner;
};

struct nxu_space {
	nxu_u32 count;
	struct nxu_owner_region region[NXU_OWNER_MAX];
	nxu_u32 armed;
	nxu_u32 frozen;
};

extern struct nxu_space nxu_space;

void nxu_s5_ownership(void);
void nxu_s6_mmu(void);
nxu_u32 nxu_space_owner(nxu_u64 pa, nxu_u64 size);

#endif

/*
 * NXU MAP
 * Layer: foundation
 * Owns: wrap-safe physical membership
 * Forbidden: overflowing address+size
 * Invariant: A6
 */
#ifndef NXU_RANGE_H
#define NXU_RANGE_H

#include <nxu/types.h>

struct nxu_range {
	nxu_u64 base;
	nxu_u64 size;
};

static inline int
nxu_range_ok(nxu_u64 base, nxu_u64 size)
{
	if (size == 0U)
		return 0;
	if (base + size < base)
		return 0;
	return 1;
}

static inline int
nxu_range_contains(nxu_u64 outer_base, nxu_u64 outer_size,
    nxu_u64 inner_base, nxu_u64 inner_size)
{
	if (!nxu_range_ok(outer_base, outer_size))
		return 0;
	if (!nxu_range_ok(inner_base, inner_size))
		return 0;
	if (inner_base < outer_base)
		return 0;
	if ((inner_base + inner_size) > (outer_base + outer_size))
		return 0;
	return 1;
}

static inline int
nxu_range_overlap(nxu_u64 a, nxu_u64 asz, nxu_u64 b, nxu_u64 bsz)
{
	if (!nxu_range_ok(a, asz) || !nxu_range_ok(b, bsz))
		return 0;
	if (a + asz <= b)
		return 0;
	if (b + bsz <= a)
		return 0;
	return 1;
}

#endif

/*
 * NXU MAP
 * Layer: foundation observe
 * Owns: nxu_fail prototype
 * Invariant: fail is halt
 */
#ifndef NXU_FAIL_H
#define NXU_FAIL_H

#include <nxu/types.h>

void nxu_fail_at(nxu_u32 code, nxu_u64 a, nxu_u64 b,
    const char *func, const char *file, nxu_u32 line);

#define nxu_fail(code, a, b) \
	nxu_fail_at((code), (a), (b), __func__, __FILE__, __LINE__)

#endif

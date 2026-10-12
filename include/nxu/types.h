/*
 * NXU MAP
 * Layer: foundation types
 * Owns: nxu_u8..nxu_u64, nxu_uptr
 * Forbidden: libc types as TCB dependency
 * Invariant: freestanding
 */
#ifndef NXU_TYPES_H
#define NXU_TYPES_H

typedef unsigned char      nxu_u8;
typedef unsigned short     nxu_u16;
typedef unsigned int       nxu_u32;
typedef unsigned long long nxu_u64;
typedef unsigned long      nxu_uptr;

#define NXU_NULL ((void *)0)

#endif

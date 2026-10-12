/*
 * NXU MAP
 * Layer: silicon
 * Owns: MRS/MSR helpers, DAIF, barriers
 * Consumes: none
 * Produces: register access
 * Hardware: AArch64 sysregs
 * Forbidden: board addresses; policy
 * Invariant: operations only
 */
#ifndef NXU_SYSREG_H
#define NXU_SYSREG_H

#include <nxu/types.h>

#define nxu_mrs(name) \
	({ nxu_u64 _v; asm volatile("mrs %0, " #name : "=r"(_v)); _v; })

#define nxu_msr(name, val) \
	do { \
		nxu_u64 _v = (val); \
		asm volatile("msr " #name ", %0" :: "r"(_v) : "memory"); \
	} while (0)

static inline void
nxu_isb(void)
{
	asm volatile("isb" ::: "memory");
}

static inline void
nxu_dsb(void)
{
	asm volatile("dsb sy" ::: "memory");
}

static inline void
nxu_daif_mask(void)
{
	asm volatile("msr daifset, #0xf" ::: "memory");
}

static inline void
nxu_daif_unmask_irq(void)
{
	asm volatile("msr daifclr, #2" ::: "memory");
}

static inline nxu_u32
nxu_current_el(void)
{
	return (nxu_u32)((nxu_mrs(CurrentEL) >> 2) & 3ULL);
}

static inline nxu_u64 nxu_mrs_s3_4_c2_c0_0(void) { nxu_u64 v; asm volatile("mrs %0, S3_4_C2_C0_0" : "=r"(v)); return v; }
static inline void nxu_msr_s3_4_c2_c0_0(nxu_u64 v) { asm volatile("msr S3_4_C2_C0_0, %0" :: "r"(v) : "memory"); }
static inline void nxu_msr_s3_4_c2_c0_2(nxu_u64 v) { asm volatile("msr S3_4_C2_C0_2, %0" :: "r"(v) : "memory"); }
static inline void nxu_msr_s3_4_c10_c2_0(nxu_u64 v) { asm volatile("msr S3_4_C10_C2_0, %0" :: "r"(v) : "memory"); }
static inline nxu_u64 nxu_mrs_s3_4_c1_c0_0(void) { nxu_u64 v; asm volatile("mrs %0, S3_4_C1_C0_0" : "=r"(v)); return v; }
static inline void nxu_msr_s3_4_c1_c0_0(nxu_u64 v) { asm volatile("msr S3_4_C1_C0_0, %0" :: "r"(v) : "memory"); }
static inline void nxu_msr_s3_4_c12_c0_0(nxu_u64 v) { asm volatile("msr S3_4_C12_C0_0, %0" :: "r"(v) : "memory"); }
static inline nxu_u64 nxu_mrs_s3_4_c13_c0_2(void) { nxu_u64 v; asm volatile("mrs %0, S3_4_C13_C0_2" : "=r"(v)); return v; }
static inline void nxu_msr_s3_4_c13_c0_2(nxu_u64 v) { asm volatile("msr S3_4_C13_C0_2, %0" :: "r"(v) : "memory"); }
static inline nxu_u64 nxu_mrs_s3_4_c5_c2_0(void) { nxu_u64 v; asm volatile("mrs %0, S3_4_C5_C2_0" : "=r"(v)); return v; }
static inline nxu_u64 nxu_mrs_s3_4_c6_c0_0(void) { nxu_u64 v; asm volatile("mrs %0, S3_4_C6_C0_0" : "=r"(v)); return v; }
static inline void nxu_msr_s3_4_c14_c2_0(nxu_u64 v) { asm volatile("msr S3_4_C14_C2_0, %0" :: "r"(v) : "memory"); }
static inline void nxu_msr_s3_4_c14_c2_1(nxu_u64 v) { asm volatile("msr S3_4_C14_C2_1, %0" :: "r"(v) : "memory"); }
static inline nxu_u64 nxu_mrs_s3_4_c14_c2_1(void) { nxu_u64 v; asm volatile("mrs %0, S3_4_C14_C2_1" : "=r"(v)); return v; }

#endif

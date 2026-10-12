/*
 * NXU MAP
 * Layer: silicon GICv3 (ops API)
 * Owns: GIC operation declarations; redistributor stride constant
 * Consumes: physical addresses as parameters
 * Produces: op prototypes
 * Hardware: GICv3
 * Forbidden: board addresses
 * Invariant: stride is an adapter fact, not a CPU count
 */
#ifndef NXU_GICV3_H
#define NXU_GICV3_H

#include <nxu/types.h>

#define NXU_GICR_STRIDE 0x20000ULL

nxu_u32 nxu_gic_r32(nxu_u64 base, nxu_u32 off);
void nxu_gic_w32(nxu_u64 base, nxu_u32 off, nxu_u32 v);
nxu_u64 nxu_gic_r64(nxu_u64 base, nxu_u32 off);

void nxu_gic_icc_enable(nxu_u32 el);
void nxu_gic_icc_sre_el2(void);
void nxu_gic_icc_sre_el1(void);
void nxu_gic_icc_pmr(nxu_u32 v);
void nxu_gic_icc_bpr(nxu_u32 v);
void nxu_gic_icc_igrpen(void);
nxu_u32 nxu_gic_iar(void);
void nxu_gic_eoi(nxu_u32 intid);
void nxu_gic_sgi(nxu_u32 intid, nxu_u64 mpidr);

void nxu_gicd_enable(nxu_u64 gicd);
void nxu_gicr_wake(nxu_u64 gicr);
void nxu_gicr_enable_ppi(nxu_u64 gicr, nxu_u32 intid);
void nxu_gicr_enable_sgi(nxu_u64 gicr, nxu_u32 intid);
void nxu_gicr_clear_pending(nxu_u64 gicr, nxu_u32 intid);

nxu_u64 nxu_gicr_typer(nxu_u64 gicr);
nxu_u32 nxu_gicd_iidr(nxu_u64 gicd);
nxu_u32 nxu_gicd_typer(nxu_u64 gicd);

#endif

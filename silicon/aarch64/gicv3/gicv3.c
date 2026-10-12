/*
 * NXU MAP
 * Layer: silicon GICv3
 * Owns: distributor/redistributor/ICC register operations
 * Consumes: physical addresses as parameters
 * Produces: IIDR/TYPER, IAR, EOI, SGI, wake, enable
 * Hardware: GICv3
 * Forbidden: board addresses; CPU count policy; handler policy
 * Invariant: silicon has no policy
 */
#include <silicon/aarch64/gicv3/gicv3.h>
#include <silicon/aarch64/sysreg.h>

nxu_u32
nxu_gic_r32(nxu_u64 base, nxu_u32 off)
{
	return *(volatile nxu_u32 *)(nxu_uptr)(base + (nxu_u64)off);
}

void
nxu_gic_w32(nxu_u64 base, nxu_u32 off, nxu_u32 v)
{
	*(volatile nxu_u32 *)(nxu_uptr)(base + (nxu_u64)off) = v;
}

nxu_u64
nxu_gic_r64(nxu_u64 base, nxu_u32 off)
{
	return *(volatile nxu_u64 *)(nxu_uptr)(base + (nxu_u64)off);
}

void
nxu_gic_icc_sre_el2(void)
{
	nxu_msr(S3_4_C12_C9_5, 7ULL); /* ICC_SRE_EL2 */
	nxu_isb();
}

void
nxu_gic_icc_sre_el1(void)
{
	nxu_msr(S3_0_C12_C12_5, 7ULL); /* ICC_SRE_EL1 */
	nxu_isb();
}

void
nxu_gic_icc_pmr(nxu_u32 v)
{
	nxu_msr(S3_0_C4_C6_0, (nxu_u64)v); /* ICC_PMR_EL1 */
}

void
nxu_gic_icc_bpr(nxu_u32 v)
{
	nxu_msr(S3_0_C12_C12_3, (nxu_u64)v); /* ICC_BPR1_EL1 */
}

void
nxu_gic_icc_igrpen(void)
{
	nxu_msr(S3_0_C12_C12_7, 1ULL); /* ICC_IGRPEN1_EL1 */
}

void
nxu_gic_icc_igrpen_el2en(void)
{
	nxu_msr(S3_6_C12_C12_7, 5ULL); /* ICC_IGRPEN1_EL3: EL1EN|EL2EN */
	nxu_isb();
	nxu_msr(S3_0_C12_C12_7, 5ULL); /* ICC_IGRPEN1_EL1 (EL2 view) */
	nxu_isb();
}

void
nxu_gic_icc_enable(nxu_u32 el)
{
	if (el == 2U)
		nxu_gic_icc_sre_el2();
	nxu_gic_icc_sre_el1();
	nxu_gic_icc_pmr(0xFFU);
	nxu_gic_icc_bpr(0U);
	nxu_gic_icc_igrpen();
	nxu_isb();
}

nxu_u32
nxu_gic_iar(void)
{
	return (nxu_u32)nxu_mrs(S3_0_C12_C12_0); /* ICC_IAR1_EL1 */
}

void
nxu_gic_eoi(nxu_u32 intid)
{
	nxu_msr(S3_0_C12_C12_1, (nxu_u64)intid); /* ICC_EOIR1_EL1 */
	nxu_isb();
}

void
nxu_gic_sgi(nxu_u32 intid, nxu_u64 mpidr)
{
	nxu_u64 aff0;
	nxu_u64 aff1;
	nxu_u64 aff2;
	nxu_u64 aff3;
	nxu_u64 v;

	aff0 = mpidr & 0xFFULL;
	aff1 = (mpidr >> 8) & 0xFFULL;
	aff2 = (mpidr >> 16) & 0xFFULL;
	aff3 = (mpidr >> 32) & 0xFFULL;
	v = (aff3 << 48) | (aff2 << 32) | (aff1 << 16) |
	    ((nxu_u64)intid << 24) | (1ULL << aff0);
	nxu_dsb();
	nxu_msr(S3_0_C12_C11_5, v); /* ICC_SGI1R_EL1 */
	nxu_isb();
}

void
nxu_gicd_enable(nxu_u64 gicd)
{
	nxu_gic_w32(gicd, 0U, (1U << 4));
	nxu_dsb();
	nxu_gic_w32(gicd, 0U, (1U << 1) | (1U << 4));
	nxu_dsb();
}

static void
gicr_frame_sync(nxu_u64 gicr)
{
	nxu_u32 spins = 0U;

	/* GICR accesses may be delayed on some implementations; GICR_SYNCR
	 * reads 0 while outstanding operations are pending, 1 when complete. */
	while ((nxu_gic_r32(gicr, 0xC0U) & 1U) == 0U) {
		spins++;
		if (spins > 100000U)
			return;
	}
}

static void
gicr_frame_enable(nxu_u64 gicr, nxu_u32 intid)
{
	nxu_u64 frame;
	nxu_u32 pri_off;
	nxu_u32 pri;

	frame = gicr + 0x10000ULL;
	nxu_gic_w32(frame, 0x80U, 0xFFFFFFFFU);
	pri_off = 0x400U + (intid & ~3U);
	pri = nxu_gic_r32(frame, pri_off);
	pri &= ~(0xFFU << ((intid & 3U) * 8U));
	pri |= (0x80U << ((intid & 3U) * 8U));
	nxu_gic_w32(frame, pri_off, pri);
	nxu_gic_w32(frame, 0x100U, (1U << intid));
	nxu_dsb();
	gicr_frame_sync(gicr);
}

void
nxu_gicr_enable_ppi(nxu_u64 gicr, nxu_u32 intid)
{
	gicr_frame_enable(gicr, intid);
}

void
nxu_gicr_enable_sgi(nxu_u64 gicr, nxu_u32 intid)
{
	gicr_frame_enable(gicr, intid);
}

void
nxu_gicr_clear_pending(nxu_u64 gicr, nxu_u32 intid)
{
	nxu_gic_w32(gicr + 0x10000ULL, 0x280U, 1U << intid);
	nxu_dsb();
}

/*
 * GICR_WAKER is at RD_base+0x14: bit1 ProcessorSleep, bit2 ChildrenAsleep,
 * bit0 RES0. A sleeping redistributor silently drops enable writes, so this
 * must succeed before any ISENABLER programming is trusted.
 */
void
nxu_gicr_wake(nxu_u64 gicr)
{
	nxu_u32 w;
	nxu_u32 spins = 0U;

	w = nxu_gic_r32(gicr, 0x14U);
	w &= ~(1U << 1);
	nxu_gic_w32(gicr, 0x14U, w);
	/* The FVP reset value can read as zero; still issue the architectural
	 * ProcessorSleep clear explicitly rather than treating that read as state. */
	nxu_gic_w32(gicr, 0x14U, 0U);
	nxu_dsb();
	gicr_frame_sync(gicr);
	do {
		w = nxu_gic_r32(gicr, 0x14U);
		spins++;
		if (spins > 10000U)
			return;
	} while ((w & (1U << 2)) != 0U);
}

nxu_u64
nxu_gicr_typer(nxu_u64 gicr)
{
	return nxu_gic_r64(gicr, 0x08U);
}

nxu_u32
nxu_gicd_iidr(nxu_u64 gicd)
{
	return nxu_gic_r32(gicd, 0x08U);
}

nxu_u32
nxu_gicd_typer(nxu_u64 gicd)
{
	return nxu_gic_r32(gicd, 0x04U);
}

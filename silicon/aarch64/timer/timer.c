/*
 * NXU MAP
 * Layer: silicon timer
 * Owns: CNTPCT/CNTFRQ and EL1/EL2 physical timer ops
 * Consumes: tick count as parameter
 * Produces: counter, ISTATUS, mask/enable
 * Hardware: ARM generic timer
 * Forbidden: PPI policy; UART tick
 * Invariant: operations only
 */
#include <silicon/aarch64/timer/timer.h>
#include <silicon/aarch64/sysreg.h>

nxu_u64
nxu_timer_cntpct(void)
{
	nxu_isb();
	return nxu_mrs(cntpct_el0);
}

nxu_u64
nxu_timer_cntfrq(void)
{
	return nxu_mrs(cntfrq_el0);
}

void
nxu_timer_enable_el1(nxu_u32 ticks)
{
	nxu_msr(cntp_tval_el0, (nxu_u64)ticks);
	nxu_msr(cntp_ctl_el0, 1ULL);
	nxu_isb();
}

void
nxu_timer_enable_el2(nxu_u32 ticks)
{
	nxu_msr_s3_4_c14_c2_0((nxu_u64)ticks);
	nxu_msr_s3_4_c14_c2_1(1ULL);
	nxu_isb();
}

nxu_u64
nxu_timer_ctl_el1(void)
{
	return nxu_mrs(cntp_ctl_el0);
}

nxu_u64
nxu_timer_cval_el1(void)
{
	return nxu_mrs(cntp_cval_el0);
}

nxu_u64
nxu_timer_ctl_el2(void)
{
	return nxu_mrs_s3_4_c14_c2_1();
}

nxu_u64
nxu_timer_tval_el1(void)
{
	return nxu_mrs(cntp_tval_el0);
}

nxu_u64
nxu_timer_tval_el2(void)
{
	return nxu_mrs(S3_4_C14_C2_0);
}

nxu_u32
nxu_timer_istatus_el1(void)
{
	return (nxu_u32)((nxu_mrs(cntp_ctl_el0) >> 2) & 1ULL);
}

nxu_u32
nxu_timer_istatus_el2(void)
{
	return (nxu_u32)((nxu_mrs_s3_4_c14_c2_1() >> 2) & 1ULL);
}

void
nxu_timer_mask_el1(void)
{
	nxu_msr(cntp_ctl_el0, 3ULL);
	nxu_isb();
}

void
nxu_timer_mask_el2(void)
{
	nxu_msr_s3_4_c14_c2_1(3ULL);
	nxu_isb();
}

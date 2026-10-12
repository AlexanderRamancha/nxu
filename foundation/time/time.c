/*
 * NXU MAP
 *
 *                    NXU FOUNDATION
 *                           │
 *                           ▼
 *                     TIMER POLICY
 *                           │
 *                           ▼
 *                  SILICON TIMER ACCESS
 *                           │
 *                           ▼
 *                      TIMER IRQ
 *                           │
 *                           ▼
 *                    NXU OBSERVATION
 *                           │
 *                 ┌─────────┴─────────┐
 *                 ▼                   ▼
 *             CHRONICLE            BLACK BOX
 *                 │
 *                 ▼
 *              CONSOLE
 *
 * Layer: foundation time (S11)
 * Owns: per-CPU one-shot deadline as a TCB signal
 * Consumes: observed CNTFRQ, ISA PPI from control_el, GIC PPI bind
 * Produces: deadline fire via timer PPI; timer_fired visible to S11
 * Hardware: ARM generic timer CNTP/CNTHP
 * Forbidden: HZ tick; UART tick; jiffies; board addresses
 * Invariant: kernel timer is CNTP; wait observes the IRQ store
 */

#include <nxu/time.h>
#include <nxu/facts.h>
#include <nxu/signal.h>
#include <nxu/cpu.h>
#include <nxu/intake.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <nxu/space.h>
#include <silicon/aarch64/timer/timer.h>
#include <silicon/aarch64/gicv3/gicv3.h>
#include <silicon/aarch64/translate/mmu.h>
#include <silicon/aarch64/sysreg.h>

nxu_u64
nxu_cntpct(void)
{
	return nxu_timer_cntpct();
}

nxu_u64
nxu_cntfrq(void)
{
	return nxu_timer_cntfrq();
}

void
nxu_timer_program(nxu_u64 ticks)
{
	nxu_timer_enable_el1((nxu_u32)ticks);
}

nxu_u32
nxu_timer_istatus(void)
{
	return nxu_timer_istatus_el1();
}

void
nxu_timer_ack(void)
{
	nxu_timer_mask_el1();
}

nxu_u64
nxu_timer_tval(void)
{
	return nxu_timer_tval_el1();
}

/*
 * CNTHP/CNTP ISTATUS is the timer condition. It is not evidence that the
 * PE took an IRQ. Group, pending, and ISR say which signal the GIC used.
 */
static void
nxu_timer_observe_delivery(void)
{
	nxu_u64 rd;
	nxu_u64 sgi;

	rd = nxu_cpu[0].gicr;
	sgi = rd + 0x10000ULL;
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_WAKER, nxu_gic_r32(rd, 0x14U));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_GICD_CTLR,
	    nxu_gic_r32(nxu_facts.gicd.range.base, 0U));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_IGROUPR0, nxu_gic_r32(sgi, 0x80U));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_ISENABLER0, nxu_gic_r32(sgi, 0x100U));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_ISPENDR0, nxu_gic_r32(sgi, 0x200U));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_IGRPMODR0, nxu_gic_r32(sgi, 0xD00U));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_DAIF, nxu_mrs(daif));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_ISR, nxu_mrs(isr_el1));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_CNTP_CTL, nxu_timer_ctl_el1());
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_CNTP_CVAL, nxu_timer_cval_el1());
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_IGRPEN1, nxu_mrs(icc_igrpen1_el1));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_HPPIR1, nxu_mrs(icc_hppir1_el1));
	NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_HPPIR0, nxu_mrs(icc_hppir0_el1));
}

void
nxu_s11_time(void)
{
	volatile nxu_u32 *fired;
	nxu_u64 t0;
	nxu_u32 spins;

	nxu_sig_timer.intid = nxu_facts.timer_ppi;
	nxu_sig_timer.owner = NXU_SIG_OWNER_TCB;
	nxu_sig_timer.route_cpu = 0U;
	nxu_sig_timer.enabled = 1U;
	nxu_cpu[0].timer_fired = 0U;
	fired = &nxu_cpu[0].timer_fired;
	nxu_dsb();
	nxu_chronicle_put(NXU_REC_S11_FREQ, nxu_facts.timer_freq, 0U);
	nxu_chronicle_put(NXU_REC_S11_PPI, nxu_facts.timer_ppi, 0U);
	NXU_LOG(NXU_EV_TIMER, NXU_SUB_TIME, nxu_facts.timer_ppi, nxu_facts.timer_freq);
	nxu_debug("TIME", "one-shot deadline");
	nxu_debug_u64("freq", nxu_facts.timer_freq);
	nxu_debug_u64("ppi", nxu_facts.timer_ppi);

	nxu_gicr_enable_ppi(nxu_cpu[0].gicr, nxu_facts.timer_ppi);
	t0 = nxu_cntpct();
	nxu_timer_program(nxu_facts.timer_freq / 100ULL);
	nxu_daif_unmask_irq();
	spins = 0U;
	while (*fired == 0U) {
		spins++;
		asm volatile("" ::: "memory");
		if (spins > 10000000U) {
			NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_T0, t0);
			NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_TNOW, nxu_cntpct());
			NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_TVAL, nxu_timer_tval());
			NXU_LOG(NXU_EV_MACHINE, NXU_SUB_TIME, NXU_MACH_ISTATUS, nxu_timer_istatus());
			if (nxu_timer_istatus() != 0U) {
				nxu_timer_observe_delivery();
				nxu_fail(NXU_FAIL_TIMER, nxu_facts.timer_ppi, 2U);
			}
			nxu_fail(NXU_FAIL_TIMER, nxu_facts.timer_ppi, 1U);
		}
	}
	nxu_daif_mask();
	nxu_chronicle_put(NXU_REC_S11_FIRE, spins, nxu_cntpct());
	NXU_LOG(NXU_EV_TIMER_FIRE, NXU_SUB_TIME, nxu_cntpct(), spins);
	nxu_debug("TIME", "deadline observed");
	nxu_debug_u64("spins", spins);
}

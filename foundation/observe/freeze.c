/*
 * NXU MAP
 *
 *                         S14
 *                           │
 *                           ▼
 *                   INVARIANT CHECK
 *                           │
 *                           ▼
 *                   S14 FREEZE RECORD
 *                           │
 *                           ▼
 *                    [NXU][S14] LINE
 *
 * Layer: foundation observe (S14)
 * Owns: freeze-time invariant check
 * Consumes: space, topology, signals, time, contexts
 * Produces: S14 freeze record and console line; boot CPU waiting
 * Hardware: none
 * Forbidden: remaining available/starting CPUs
 * Invariant: all present online; one bound context each
 */

#include <nxu/s0.h>
#include <nxu/cpu.h>
#include <nxu/space.h>
#include <nxu/facts.h>
#include <nxu/intake.h>
#include <nxu/signal.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <silicon/aarch64/translate/mmu.h>

void
nxu_s14_freeze(void)
{
	nxu_u32 i;

	if (!nxu_space.armed)
		nxu_fail(NXU_FAIL_INVARIANT, 7U, 0U);
	if (!nxu_cpu_present_frozen)
		nxu_fail(NXU_FAIL_INVARIANT, 9U, 0U);
	if (nxu_cpu_present_count == 0U)
		nxu_fail(NXU_FAIL_INVARIANT, 8U, 0U);
	if (nxu_facts.timer_freq == 0U)
		nxu_fail(NXU_FAIL_INVARIANT, 19U, 0U);
	if (nxu_sig_timer.enabled && nxu_sig_timer.owner != NXU_SIG_OWNER_TCB)
		nxu_fail(NXU_FAIL_INVARIANT, 14U, nxu_sig_timer.intid);
	if (nxu_sig_ipi.enabled && nxu_sig_ipi.owner != NXU_SIG_OWNER_TCB)
		nxu_fail(NXU_FAIL_INVARIANT, 14U, nxu_sig_ipi.intid);

	for (i = 0U; i < nxu_cpu_present_count; i++) {
		if (nxu_cpu[i].state == NXU_CPU_AVAILABLE ||
		    nxu_cpu[i].state == NXU_CPU_STARTING)
			nxu_fail(NXU_FAIL_INVARIANT, 28U, i);
		if (nxu_cpu[i].state != NXU_CPU_ONLINE)
			nxu_fail(NXU_FAIL_INVARIANT, 12U, i);
		if (nxu_cpu[i].ctx.life != NXU_CTX_RUNNING &&
		    nxu_cpu[i].ctx.life != NXU_CTX_WAITING)
			nxu_fail(NXU_FAIL_INVARIANT, 12U, nxu_cpu[i].ctx.life);
		if (nxu_cpu[i].ctx.el != nxu_control_el())
			nxu_fail(NXU_FAIL_INVARIANT, 11U, nxu_cpu[i].ctx.el);
		if (nxu_cpu[i].ctx.ttbr != nxu_mmu_ttbr())
			nxu_fail(NXU_FAIL_INVARIANT, 26U, i);
	}
	nxu_chronicle_put(NXU_REC_S14_FREEZE, nxu_cpu_present_count,
	    nxu_facts.mem[0].range.base);
	NXU_LOG(NXU_EV_KERNEL, NXU_SUB_BOOT, nxu_facts.mem[0].range.base,
	    nxu_cpu_present_count);
	nxu_debug("S14", "freeze");
	nxu_debug_u64("present", nxu_cpu_present_count);
	nxu_debug_u64("ram", nxu_facts.mem[0].range.base);
	nxu_cpu[0].ctx.life = NXU_CTX_WAITING;
}

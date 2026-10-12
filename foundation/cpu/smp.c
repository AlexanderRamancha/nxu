/*
 * NXU MAP
 * Layer: foundation cpu (S12)
 * Owns: secondary occupy path; CPU_ON + timeout; freeze tables before CPU_ON
 * Consumes: available set, PSCI adapter, already-built tables
 * Produces: N present → N online or halt
 * Hardware: PSCI CPU_ON as adapter only
 * Forbidden: CPU_ON success as online; secondaries discovering
 * Invariant: A12; N from facts
 */
#include <nxu/cpu.h>
#include <nxu/facts.h>
#include <nxu/space.h>
#include <nxu/intake.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/time.h>
#include <nxu/signal.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <silicon/aarch64/psci/psci.h>
#include <silicon/aarch64/sysreg.h>
#include <silicon/aarch64/translate/mmu.h>
#include <silicon/aarch64/gicv3/gicv3.h>
#include <silicon/aarch64/timer/timer.h>

struct nxu_cpu_gate {
	volatile nxu_u64 mpidr;
	volatile nxu_u64 context;
	volatile nxu_u32 ready;
	volatile nxu_u32 pad;
} __attribute__((aligned(32)));

struct nxu_cpu_gate nxu_cpu_gate[NXU_CPU_BOUND];
volatile nxu_u32 nxu_ipi_echo;

void
nxu_secondary(nxu_u64 logical)
{
	nxu_u32 id;
	nxu_u32 el;
	extern char nxu_vec_full[];
	nxu_u64 vbar;

	id = (nxu_u32)logical;
	if (id == 0U || id >= nxu_cpu_present_count)
		nxu_fail(NXU_FAIL_CPU, id, 4U);
	NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, id, nxu_cpu[id].mpidr, NXU_CPU_ENTER);

	el = nxu_current_el();
	if (el != NXU_EL1 || nxu_control_el() != NXU_EL1)
		nxu_fail(NXU_FAIL_BAD_EL, el, 1U);
	NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, id, el, NXU_CPU_EL1);

	nxu_mmu_enable(el);
	vbar = (nxu_u64)(nxu_uptr)nxu_vec_full;
	nxu_msr(vbar_el1, vbar);
	nxu_isb();
	NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, id, vbar, NXU_CPU_VBAR);
	nxu_gic_icc_enable(el);
	nxu_gicr_wake(nxu_cpu[id].gicr);
	nxu_gicr_enable_ppi(nxu_cpu[id].gicr, nxu_facts.timer_ppi);
	nxu_gicr_enable_sgi(nxu_cpu[id].gicr, NXU_SGI_IPI);
	nxu_timer_mask_el1();
	nxu_cpu[id].ctx.life = NXU_CTX_WAITING;
	nxu_dsb();
	nxu_cpu[id].state = NXU_CPU_ONLINE;
	nxu_dsb();
	nxu_chronicle_put(NXU_REC_SEC_ENTER, id, nxu_cpu[id].mpidr);
	nxu_daif_unmask_irq();
	for (;;) {
		nxu_u32 echo;

		asm volatile("wfi" ::: "memory");
		echo = nxu_ipi_echo;
		if (echo == id && nxu_cpu[id].ipi_seen != 0U) {
			nxu_ipi_echo = 0U;
			nxu_dsb();
			nxu_gic_sgi(NXU_SGI_IPI, nxu_cpu[0].mpidr);
		}
	}
}

static nxu_u64
release(nxu_u32 id)
{
	nxu_u64 entry;

	asm volatile("adr %0, nxu_secondary_entry" : "=r"(entry));
	if (nxu_facts.pwrc.size != 0U) {
		volatile nxu_u32 *pponr;

		nxu_cpu_gate[id].context = id;
		nxu_cpu_gate[id].mpidr = nxu_cpu[id].mpidr;
		nxu_dsb();
		nxu_cpu_gate[id].ready = 1U;
		nxu_dsb();
		pponr = (volatile nxu_u32 *)(nxu_uptr)(nxu_facts.pwrc.base + 4U);
		*pponr = (nxu_u32)nxu_cpu[id].mpidr;
		return 0U;
	}
	return nxu_psci_cpu_on(nxu_facts.psci_conduit, nxu_cpu[id].mpidr,
	    entry, (nxu_u64)id);
}

void
nxu_s12_smp(void)
{
	nxu_u32 i;
	nxu_u64 deadline;
	nxu_u64 ret;

	nxu_facts.frozen = 1U;
	nxu_space.frozen = 1U;

	if (nxu_facts.psci_conduit == NXU_PSCI_NONE && nxu_facts.pwrc.size == 0U)
		nxu_fail(NXU_FAIL_PSCI, 0U, 0U);

	nxu_cpu[0].state = NXU_CPU_ONLINE;
	NXU_LOG(NXU_EV_CPU_ONLINE, NXU_SUB_CPU, nxu_cpu[0].mpidr, 0U);
	if (nxu_cpu_present_count <= 1U) {
		nxu_chronicle_put(NXU_REC_S12_ONLINE, 1U, 0U);
		return;
	}

	for (i = 1U; i < nxu_cpu_present_count; i++) {
		nxu_cpu[i].state = NXU_CPU_STARTING;
		nxu_chronicle_put(NXU_REC_S12_ON, i, nxu_cpu[i].mpidr);
		ret = release(i);
		if (ret != 0U)
			nxu_fail(NXU_FAIL_PSCI, ret, i);
		deadline = nxu_cntpct() + nxu_facts.timer_freq * 2ULL;
		while (nxu_cpu[i].state != NXU_CPU_ONLINE) {
			if (nxu_cntpct() > deadline)
				nxu_fail(NXU_FAIL_TIMEOUT, i, nxu_cpu[i].mpidr);
			asm volatile("" ::: "memory");
		}
		NXU_LOG(NXU_EV_CPU_ONLINE, NXU_SUB_CPU, nxu_cpu[i].mpidr, i);
	}
	nxu_chronicle_put(NXU_REC_S12_ONLINE, nxu_cpu_present_count, 0U);
}

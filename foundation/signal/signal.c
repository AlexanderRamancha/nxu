/*
 * NXU MAP
 *
 *                    SIGNAL POLICY
 *                           │
 *                           ▼
 *                    SILICON GIC OPS
 *                           │
 *                           ▼
 *                      IRQ / IPI
 *                           │
 *                           ▼
 *                   timer_fired / ipi_seen
 *
 * Layer: foundation signal (S10 S13)
 * Owns: exclusive single-CPU signal bind; INTID class dispatch
 * Consumes: GIC silicon ops, CPU present set, GICR bases as parameters
 * Produces: timer and IPI signals TCB-owned; rdist corroboration
 * Hardware: GICv3 distributor/redistributor/ICC (adapter)
 * Forbidden: 1-of-N; function-pointer handlers; ITS/LPI
 * Invariant: enabled implies one owner; unbound INTID → halt
 */

#include <nxu/signal.h>
#include <nxu/cpu.h>
#include <nxu/facts.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/time.h>
#include <nxu/intake.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <silicon/aarch64/gicv3/gicv3.h>
#include <silicon/aarch64/sysreg.h>

struct nxu_signal nxu_sig_timer;
struct nxu_signal nxu_sig_ipi;

extern nxu_u64 nxu_icc_sre3_obs[8];

void
nxu_irq_dispatch(nxu_u32 intid)
{
	nxu_u32 id;

	id = nxu_this_logical();
	if (id >= NXU_CPU_BOUND)
		id = 0U;
	nxu_chronicle_put(NXU_REC_IRQ, intid, id);

	if (intid == nxu_sig_timer.intid && nxu_sig_timer.enabled) {
		nxu_cpu[id].timer_fired++;
		nxu_dsb();
		nxu_timer_ack();
		return;
	}
	if (intid == nxu_sig_ipi.intid && nxu_sig_ipi.enabled) {
		nxu_cpu[id].ipi_seen++;
		nxu_dsb();
		nxu_chronicle_put(NXU_REC_S13_RECV, intid, id);
		return;
	}
	nxu_fail(NXU_FAIL_UNBOUND_IRQ, intid, id);
}

void
nxu_s10_gic(void)
{
	nxu_u64 gicd;
	nxu_u64 gicr_base;
	nxu_u64 gicr;
	nxu_u32 last;
	nxu_u32 nrd;
	nxu_u32 scanned;
	nxu_u32 i;
	nxu_u32 el;
	nxu_u32 iidr;
	nxu_u32 typer;

	gicd = nxu_facts.gicd.range.base;
	gicr_base = nxu_facts.gicr.range.base;
	iidr = nxu_gicd_iidr(gicd);
	typer = nxu_gicd_typer(gicd);
	nxu_chronicle_put(NXU_REC_S10_GIC, iidr, typer);
	NXU_LOG(NXU_EV_GIC, NXU_SUB_GIC, gicd, typer);
	nxu_debug("GIC", "distributor");
	nxu_debug_u64("gicd", gicd);
	nxu_debug_u64("gicr", gicr_base);
	nxu_debug_u64("iidr", iidr);

	/* IIDR/TYPER observation after the MMIO region is owned and mapped. */
	if (nxu_facts.gicd.grade == NXU_GRADE_SINGLE)
		nxu_facts.gicd.grade = NXU_GRADE_CORROBORATED;
	nxu_facts.gicd.source |= NXU_SRC_OBS;

	nrd = 0U;
	scanned = 0U;
	gicr = gicr_base;
	last = 0U;
	while (last == 0U) {
		nxu_u64 t = nxu_gicr_typer(gicr);
		nxu_u64 aff = (t >> 32) & 0xFFFFFFFFULL;
		nxu_u32 found = 0U;

		last = (nxu_u32)((t >> 4) & 1ULL);
		for (i = 0U; i < nxu_cpu_present_count; i++) {
			nxu_u64 cpu_aff;

			cpu_aff = (nxu_cpu[i].mpidr & 0x00FFFFFFULL) |
			    (((nxu_cpu[i].mpidr >> 32) & 0xFFULL) << 24);
			if (cpu_aff == aff) {
				nxu_cpu[i].gicr = gicr;
				found = 1U;
				NXU_LOG3(NXU_EV_CPU, NXU_SUB_GIC, i, aff, NXU_CPU_GIC);
			}
		}
		nxu_chronicle_put(NXU_REC_S10_RDIST, aff, found);
		if (found == 0U)
			NXU_LOG3(NXU_EV_CPU, NXU_SUB_GIC, aff, 0U, NXU_CPU_RDIST);
		if (found)
			nrd++;
		scanned++;
		if (scanned > nxu_facts.gicr.range.size / NXU_GICR_STRIDE)
			nxu_fail(NXU_FAIL_GIC, nrd, 0U);
		gicr += NXU_GICR_STRIDE;
	}
	if (nrd != nxu_cpu_present_count)
		nxu_fail(NXU_FAIL_GIC, nrd, nxu_cpu_present_count);
	for (i = 0U; i < nxu_cpu_present_count; i++) {
		if (nxu_cpu[i].gicr == 0U)
			nxu_fail(NXU_FAIL_GIC, nxu_cpu[i].mpidr, 2U);
	}

	el = nxu_control_el();
	if (el == NXU_EL1) {
		/* No EL3/EL2 firmware stage existed; EL1 owns its SRE enable. */
		nxu_gic_icc_sre_el1();
	}
	nxu_gic_icc_pmr(0xFFU);
	nxu_gic_icc_bpr(0U);
	nxu_gic_icc_igrpen();
	nxu_isb();
	nxu_gicd_enable(gicd);
	nxu_gicr_wake(nxu_cpu[0].gicr);
	NXU_LOG(NXU_EV_GIC_INIT, NXU_SUB_GIC, nxu_cpu[0].gicr, nxu_icc_sre3_obs[7]);
	nxu_cpu_present_frozen = 1U;
	nxu_chronicle_put(NXU_REC_S10_FREEZE, nrd, nxu_cpu[0].gicr);
}

static void
ipi_to(nxu_u32 src, nxu_u32 dst)
{
	nxu_u32 spins;
	volatile nxu_u32 *seen;
	nxu_u32 before;

	before = nxu_cpu[dst].ipi_seen;
	NXU_LOG3(NXU_EV_IPI, NXU_SUB_SMP, src, dst, NXU_IPI_SEND);
	nxu_gic_sgi(NXU_SGI_IPI, nxu_cpu[dst].mpidr);
	seen = &nxu_cpu[dst].ipi_seen;
	spins = 0U;
	while (*seen == before) {
		spins++;
		asm volatile("" ::: "memory");
		if (spins > 1000000U)
			nxu_fail(NXU_FAIL_TIMEOUT, dst, src);
	}
	NXU_LOG3(NXU_EV_IPI, NXU_SUB_SMP, src, dst, NXU_IPI_RECV);
	nxu_chronicle_put(NXU_REC_S13_IPI, src, dst);
}

static int
other_cluster(nxu_u32 id)
{
	nxu_u32 boot;
	nxu_u32 aff;

	boot = (nxu_u32)((nxu_cpu[0].mpidr >> 16) & 0xffULL);
	aff = (nxu_u32)((nxu_cpu[id].mpidr >> 16) & 0xffULL);
	return aff != boot;
}

void
nxu_s13_ipi(void)
{
	nxu_u32 i;
	nxu_u32 echo;
	nxu_u32 cross;

	nxu_sig_ipi.intid = NXU_SGI_IPI;
	nxu_sig_ipi.owner = NXU_SIG_OWNER_TCB;
	nxu_sig_ipi.route_cpu = 0U;
	nxu_sig_ipi.enabled = 1U;
	nxu_gicr_enable_sgi(nxu_cpu[0].gicr, NXU_SGI_IPI);
	nxu_daif_unmask_irq();
	nxu_cpu[0].ipi_seen = 0U;
	ipi_to(0U, 0U);

	echo = 0U;
	cross = 0U;
	for (i = 1U; i < nxu_cpu_present_count; i++) {
		if (echo == 0U)
			echo = i;
		if (cross == 0U && other_cluster(i))
			cross = i;
		ipi_to(0U, i);
	}
	if (echo != 0U) {
		nxu_u32 before;
		nxu_u32 spins;

		before = nxu_cpu[0].ipi_seen;
		nxu_ipi_echo = echo;
		nxu_dsb();
		ipi_to(0U, echo);
		spins = 0U;
		while (nxu_cpu[0].ipi_seen == before) {
			spins++;
			asm volatile("" ::: "memory");
			if (spins > 1000000U)
				nxu_fail(NXU_FAIL_TIMEOUT, echo, 0U);
		}
		NXU_LOG3(NXU_EV_IPI, NXU_SUB_SMP, echo, 0U, NXU_IPI_SEND);
		NXU_LOG3(NXU_EV_IPI, NXU_SUB_SMP, echo, 0U, NXU_IPI_RECV);
	}
	if (cross != 0U && cross != echo) {
		nxu_u32 before;
		nxu_u32 spins;

		before = nxu_cpu[0].ipi_seen;
		nxu_ipi_echo = cross;
		nxu_dsb();
		ipi_to(0U, cross);
		spins = 0U;
		while (nxu_cpu[0].ipi_seen == before) {
			spins++;
			asm volatile("" ::: "memory");
			if (spins > 1000000U)
				nxu_fail(NXU_FAIL_TIMEOUT, cross, 0U);
		}
		NXU_LOG3(NXU_EV_IPI, NXU_SUB_SMP, cross, 0U, NXU_IPI_SEND);
		NXU_LOG3(NXU_EV_IPI, NXU_SUB_SMP, cross, 0U, NXU_IPI_RECV);
	}
	NXU_LOG(NXU_EV_CPU_SET, NXU_SUB_SMP, nxu_cpu_present_count, 0U);
}

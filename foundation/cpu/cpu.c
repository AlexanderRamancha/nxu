/*
 * NXU MAP
 * Layer: foundation cpu (S8 S9)
 * Owns: present set; logical ids; per-CPU stack and context bind
 * Consumes: CPU facts, boot MPIDR
 * Produces: initialized/available slots for every present CPU
 * Hardware: TPIDR as logical id
 * Forbidden: hardcoded single-secondary start; present==online
 * Invariant: boot MPIDR is present; no CPU1-only stack
 */
#include <nxu/cpu.h>
#include <nxu/facts.h>
#include <nxu/intake.h>
#include <nxu/image.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <silicon/aarch64/sysreg.h>
#include <silicon/aarch64/translate/mmu.h>

struct nxu_cpu nxu_cpu[NXU_CPU_BOUND];
volatile nxu_u32 nxu_cpu_present_count;
nxu_u32 nxu_cpu_present_frozen;
nxu_u8 nxu_cpu_stack[NXU_CPU_BOUND][NXU_STACK_SIZE] __attribute__((aligned(16)));

nxu_u32
nxu_this_logical(void)
{
	nxu_u64 t;

	if (nxu_current_el() == 2U)
		t = nxu_mrs_s3_4_c13_c0_2();
	else
		t = nxu_mrs(tpidr_el1);
	return (nxu_u32)t;
}

void
nxu_set_this_logical(nxu_u32 id)
{
	if (nxu_current_el() == 2U)
		nxu_msr_s3_4_c13_c0_2((nxu_u64)id);
	else
		nxu_msr(tpidr_el1, (nxu_u64)id);
	nxu_isb();
}

void
nxu_s8_present(void)
{
	nxu_u32 i;
	nxu_u64 boot;
	nxu_u32 dst;

	boot = nxu_intake.mpidr & 0xFF00FFFFFFULL;
	if (nxu_facts.cpu_count == 0U || nxu_facts.cpu_count > NXU_CPU_BOUND)
		nxu_fail(NXU_FAIL_CPU, nxu_facts.cpu_count, 0U);

	nxu_cpu[0].mpidr = boot;
	nxu_cpu[0].logical = 0U;
	nxu_cpu[0].state = NXU_CPU_PRESENT;
	dst = 1U;
	for (i = 0U; i < nxu_facts.cpu_count; i++) {
		if (nxu_facts.cpu[i].mpidr == boot) {
			nxu_facts.cpu[i].logical = 0U;
			continue;
		}
		if (dst >= NXU_CPU_BOUND)
			nxu_fail(NXU_FAIL_CPU, dst, 3U);
		nxu_cpu[dst].mpidr = nxu_facts.cpu[i].mpidr;
		nxu_cpu[dst].logical = dst;
		nxu_cpu[dst].state = NXU_CPU_PRESENT;
		nxu_facts.cpu[i].logical = dst;
		dst++;
	}
	nxu_cpu_present_count = dst;
	nxu_cpu_present_frozen = 0U;

	{
		nxu_u32 found = 0U;
		for (i = 0U; i < nxu_cpu_present_count; i++) {
			if (nxu_cpu[i].mpidr == boot)
				found = 1U;
			nxu_chronicle_put(NXU_REC_S8_CPU, nxu_cpu[i].mpidr, i);
		}
		if (!found)
			nxu_fail(NXU_FAIL_CPU, boot, 4U);
	}
	nxu_chronicle_put(NXU_REC_S8_PRESENT, nxu_cpu_present_count, boot);
	NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, 0U, 0U, NXU_CPU_DETECT);
	{
		nxu_u32 aff2_id[NXU_CPU_BOUND];
		nxu_u32 aff2_n[NXU_CPU_BOUND];
		nxu_u32 nc;
		nxu_u32 k;

		nc = 0U;
		for (i = 0U; i < nxu_cpu_present_count; i++) {
			nxu_u32 aff2;
			nxu_u32 slot;

			aff2 = (nxu_u32)((nxu_cpu[i].mpidr >> 16) & 0xffULL);
			NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, i, nxu_cpu[i].mpidr,
			    NXU_CPU_PRESENT);
			slot = nc;
			for (k = 0U; k < nc; k++) {
				if (aff2_id[k] == aff2)
					slot = k;
			}
			if (slot == nc) {
				aff2_id[nc] = aff2;
				aff2_n[nc] = 1U;
				nc++;
			} else {
				aff2_n[slot]++;
			}
		}
		for (k = 0U; k < nc; k++)
			NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, aff2_id[k], aff2_n[k],
			    NXU_CPU_CLUSTER);
	}
	nxu_debug("CPU", "present set");
	nxu_debug_u64("present", nxu_cpu_present_count);
	nxu_debug_u64("boot_mpidr", boot);
}

void
nxu_s9_init(void)
{
	nxu_u32 i;
	nxu_u64 boot;

	boot = nxu_intake.mpidr & 0xFF00FFFFFFULL;
	for (i = 0U; i < nxu_cpu_present_count; i++) {
		nxu_cpu[i].stack_base = nxu_pa_of(&nxu_cpu_stack[i][0]);
		nxu_cpu[i].stack_limit = nxu_cpu[i].stack_base + NXU_STACK_SIZE;
		nxu_cpu[i].ipi_seen = 0U;
		nxu_cpu[i].timer_fired = 0U;
		nxu_cpu[i].gicr = 0U;
		if (nxu_cpu[i].mpidr == boot) {
			nxu_cpu[i].stack_base = nxu_intake.sp_nxu - NXU_STACK_SIZE;
			nxu_cpu[i].stack_limit = nxu_intake.sp_nxu;
			nxu_set_this_logical(i);
			nxu_context_bind(&nxu_cpu[i], i, NXU_CTX_RUNNING);
		} else {
			nxu_context_bind(&nxu_cpu[i], i, NXU_CTX_ARMED);
		}
		nxu_cpu[i].state = NXU_CPU_INITIALIZED;
		NXU_LOG3(NXU_EV_CPU, NXU_SUB_CPU, i, nxu_cpu[i].mpidr, NXU_CPU_INIT);
		nxu_cpu[i].state = NXU_CPU_AVAILABLE;
		nxu_chronicle_put(NXU_REC_S9_INIT, i, nxu_cpu[i].stack_base);
	}
}

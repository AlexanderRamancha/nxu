/*
 * NXU MAP
 * Layer: foundation context
 * Owns: binding nxu_cpu_exec_context to a CPU slot under the TCB regime
 * Consumes: CPU stack bounds, MMU TTBR/TCR/MAIR, control EL
 * Produces: life running/waiting/armed; translate fields = TCB tables
 * Hardware: none
 * Forbidden: scheduling; a second translation regime; renaming the type
 * Invariant: each online CPU has exactly one bound context
 */

#include <nxu/context.h>
#include <nxu/cpu.h>
#include <nxu/intake.h>
#include <nxu/config.h>
#include <silicon/aarch64/translate/mmu.h>

void
nxu_context_bind(struct nxu_cpu *cpu, nxu_u32 id, nxu_u32 life)
{
	cpu->ctx.cpu_id = id;
	cpu->ctx.context_id = id;
	cpu->ctx.generation = 1U;
	cpu->ctx.life = life;
	cpu->ctx.el = nxu_control_el();
	cpu->ctx.ttbr = nxu_mmu_ttbr();
	cpu->ctx.tcr = nxu_mmu_tcr();
	cpu->ctx.mair = nxu_mmu_mair();
	cpu->ctx.stack_base = cpu->stack_base;
	cpu->ctx.stack_limit = cpu->stack_limit;
	cpu->ctx.owner = NXU_SIG_OWNER_TCB;
	cpu->ctx.esr = 0U;
	cpu->ctx.far = 0U;
	cpu->ctx.intid = 0U;
}

void
nxu_context_bind_boot(void)
{
	struct nxu_cpu *cpu;

	cpu = &nxu_cpu[0];
	cpu->stack_base = nxu_intake.sp_nxu - NXU_STACK_SIZE;
	cpu->stack_limit = nxu_intake.sp_nxu;
	nxu_context_bind(cpu, 0U, NXU_CTX_RUNNING);
}

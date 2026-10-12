#ifndef NXU_CONTEXT_H
#define NXU_CONTEXT_H

/*
 * NXU MAP
 * Layer: foundation context (type)
 * Owns: nxu_cpu_exec_context layout and TCB bind
 * Consumes: CPU slot, translation regime, stack bounds
 * Produces: one bound context per online CPU
 * Hardware: none directly
 * Forbidden: scheduler, thread/process/task, renaming the type
 * Invariant: type name is nxu_cpu_exec_context; translate = TCB regime
 */

#include <nxu/types.h>
#include <nxu/config.h>

struct nxu_cpu_exec_context {
	nxu_u32 cpu_id;
	nxu_u32 context_id;
	nxu_u32 generation;
	nxu_u32 life;
	nxu_u64 gpr[31];
	nxu_u64 sp;
	nxu_u64 pc;
	nxu_u64 pstate;
	nxu_u32 el;
	nxu_u64 ttbr;
	nxu_u64 tcr;
	nxu_u64 mair;
	nxu_u64 esr;
	nxu_u64 far;
	nxu_u64 daif;
	nxu_u32 intid;
	nxu_u64 stack_base;
	nxu_u64 stack_limit;
	nxu_u32 owner;
};

typedef struct nxu_cpu_exec_context nxu_cpu_exec_context;

struct nxu_cpu;

void nxu_context_bind_boot(void);
void nxu_context_bind(struct nxu_cpu *cpu, nxu_u32 id, nxu_u32 life);

#endif

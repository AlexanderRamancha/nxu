#ifndef NXU_CPU_H
#define NXU_CPU_H

/*
 * NXU MAP
 * Layer: foundation cpu
 * Owns: present/init/available/starting/online/failed; per-CPU storage
 * Consumes: accepted CPU facts, boot MPIDR, PSCI ops (S12)
 * Produces: logical ids after present set; stacks for every present CPU
 * Hardware: MPIDR, TPIDR, PSCI CPU_ON (adapter)
 * Forbidden: hardcoded single-secondary start; single-secondary stack; present==online; CPU_ON==online
 * Invariant: present ≠ initialized ≠ available ≠ online; secondaries occupy
 */

#include <nxu/types.h>
#include <nxu/config.h>
#include <nxu/context.h>

struct nxu_cpu {
	nxu_u64 mpidr;
	nxu_u32 logical;
	nxu_u32 state;
	nxu_u64 gicr;
	nxu_u64 stack_base;
	nxu_u64 stack_limit;
	volatile nxu_u32 ipi_seen;
	volatile nxu_u32 timer_fired;
	nxu_cpu_exec_context ctx;
};

extern struct nxu_cpu nxu_cpu[NXU_CPU_BOUND];
extern volatile nxu_u32 nxu_cpu_present_count;
extern volatile nxu_u32 nxu_ipi_echo;
extern nxu_u32 nxu_cpu_present_frozen;
extern nxu_u8 nxu_cpu_stack[NXU_CPU_BOUND][NXU_STACK_SIZE];

nxu_u32 nxu_this_logical(void);
void nxu_set_this_logical(nxu_u32 id);
void nxu_s8_present(void);
void nxu_s9_init(void);
void nxu_s12_smp(void);
void nxu_secondary(nxu_u64 logical);

#endif

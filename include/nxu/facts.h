/*
 * NXU MAP
 * Layer: foundation facts (type)
 * Owns: accepted fact table layout
 * Forbidden: using facts before corroboration
 * Invariant: every fact has source mask and grade
 */
#ifndef NXU_FACTS_H
#define NXU_FACTS_H

#include <nxu/types.h>
#include <nxu/config.h>
#include <nxu/range.h>

struct nxu_mem_fact {
	struct nxu_range range;
	nxu_u32 source;
	nxu_u32 grade;
};

struct nxu_cpu_fact {
	nxu_u64 mpidr;
	nxu_u32 logical;
	nxu_u32 state;
	nxu_u32 source;
	nxu_u32 grade;
};

struct nxu_ctl_fact {
	struct nxu_range range;
	nxu_u32 kind;
	nxu_u32 source;
	nxu_u32 grade;
};

struct nxu_facts {
	nxu_u32 mem_count;
	struct nxu_mem_fact mem[NXU_MEM_FACT_MAX];
	nxu_u32 cpu_count;
	struct nxu_cpu_fact cpu[NXU_CPU_BOUND];
	struct nxu_ctl_fact gicd;
	struct nxu_ctl_fact gicr;
	struct nxu_ctl_fact uart;
	nxu_u32 uart_valid;
	nxu_u32 psci_conduit;
	nxu_u32 psci_source;
	nxu_u32 timer_ppi;
	nxu_u64 timer_freq;
	struct nxu_range pwrc;
	nxu_u32 control_el;
	nxu_u32 dtb_valid;
	nxu_u64 dtb_base;
	nxu_u64 dtb_size;
	nxu_u32 frozen;
};

extern struct nxu_facts nxu_facts;

void nxu_s3_dtb(void);
void nxu_s4_facts(void);

#endif

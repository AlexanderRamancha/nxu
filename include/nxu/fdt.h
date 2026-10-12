/*
 * NXU MAP
 * Layer: foundation facts decoder (type)
 * Owns: DTB candidate struct
 * Forbidden: decoder policy
 * Invariant: candidates are not accepted facts
 */
#ifndef NXU_FDT_H
#define NXU_FDT_H

#include <nxu/types.h>
#include <nxu/config.h>
#include <nxu/range.h>

/* DTB decoder output. Candidate facts only. No policy. */
struct nxu_dtb_cand {
	nxu_u32 valid;
	nxu_u32 mem_count;
	struct nxu_range mem[NXU_MEM_FACT_MAX];
	nxu_u32 cpu_count;
	nxu_u64 cpu_mpidr[NXU_CPU_BOUND];
	struct nxu_range gicd;
	struct nxu_range gicr;
	struct nxu_range uart;
	nxu_u32 uart_valid;
	nxu_u32 psci_conduit;
	nxu_u32 timer_ppi;
	nxu_u64 base;
	nxu_u64 size;
};

extern struct nxu_dtb_cand nxu_dtb_cand;

#endif

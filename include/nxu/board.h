#ifndef NXU_BOARD_H
#define NXU_BOARD_H

/*
 * NXU MAP
 * Layer: board (type)
 * Owns: candidate fact struct only
 * Consumes: none
 * Produces: nxu_board from a board facts file
 * Hardware: none
 * Forbidden: functions, silicon calls, alternate architecture
 * Invariant: board has no behavior
 */

#include <nxu/types.h>
#include <nxu/config.h>
#include <nxu/range.h>

struct nxu_board_facts {
	nxu_u32 present;
	nxu_u32 ram_count;
	struct nxu_range ram[NXU_MEM_FACT_MAX];
	struct nxu_range gicd;
	struct nxu_range gicr;
	struct nxu_range uart;
	nxu_u32 cpu_count;
	nxu_u64 cpu_mpidr[NXU_CPU_BOUND];
	nxu_u32 psci_conduit;
	nxu_u32 timer_freq;
	struct nxu_range pwrc;
};

extern const struct nxu_board_facts nxu_board;

#endif

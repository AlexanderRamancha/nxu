/*
 * NXU MAP
 * Layer: board/fvp-base
 * Owns: candidate RAM/GIC/UART/CPU/PSCI facts for FVP Base
 * Consumes: none
 * Produces: nxu_board candidates
 * Hardware: none (data)
 * Forbidden: behavior; silicon calls; skip-validation
 * Invariant: addresses live only here, not in foundation/silicon
 */
#include <nxu/board.h>

const struct nxu_board_facts nxu_board = {
	.present = 1U,
	.ram_count = 1U,
	.ram = {
		{ 0x80000000ULL, 0x80000000ULL },
	},
	.gicd = { 0x2F000000ULL, 0x10000ULL },
	.gicr = { 0x2F100000ULL, 0x400000ULL },
	.uart = { 0x1C090000ULL, 0x1000ULL },
	/* Measured on Base RevC with affinity shifted into Aff1, cluster in Aff2.
	 * Aff0 stays 0. Redistributor TYPER reported these eight affinities. */
	.cpu_count = 8U,
	.cpu_mpidr = {
		0x00000ULL, 0x00100ULL, 0x00200ULL, 0x00300ULL,
		0x10000ULL, 0x10100ULL, 0x10200ULL, 0x10300ULL,
	},
	.psci_conduit = NXU_PSCI_SMC,
	.timer_freq = 100000000U,
	.pwrc = { 0x1C100000ULL, 0x1000ULL },
};

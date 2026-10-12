/*
 * NXU MAP
 * Layer: board/qemu-virt
 * Owns: candidate RAM/GIC/UART/CPU/PSCI facts for QEMU virt
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
		{ 0x40000000ULL, 0x8000000ULL },
	},
	.gicd = { 0x08000000ULL, 0x10000ULL },
	.gicr = { 0x080A0000ULL, 0xF60000ULL },
	.uart = { 0x09000000ULL, 0x1000ULL },
	.cpu_count = 4U,
	.cpu_mpidr = { 0U, 1U, 2U, 3U },
	.psci_conduit = NXU_PSCI_HVC,
	.timer_freq = 62500000U,
	.pwrc = { 0U, 0U },
};

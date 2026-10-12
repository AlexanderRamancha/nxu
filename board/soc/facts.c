/*
 * NXU MAP
 * Layer: board/soc (candidate facts)
 * Owns: placeholder SoC fact table; present=0 until a real SoC is described
 * Consumes: none
 * Produces: empty candidates (boot with this board fails closed at facts)
 * Hardware: none until filled from a real SoC
 * Forbidden: behavior, silicon calls, skip-validation
 * Invariant: architecture is identical; only facts would change
 */

#include <nxu/board.h>

const struct nxu_board_facts nxu_board = {
	.present = 0U,
	.ram_count = 0U,
	.cpu_count = 0U,
	.psci_conduit = NXU_PSCI_NONE,
};

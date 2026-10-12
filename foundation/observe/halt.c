/*
 * NXU MAP
 *
 *                         HALT
 *                           │
 *                           ▼
 *                    DAIF MASK + WFI
 *
 * Layer: foundation observe
 * Owns: halt loop
 * Consumes: none (UART is optional and already used by fail/freeze)
 * Produces: DAIF masked, WFI
 * Hardware: DAIF, WFI
 * Forbidden: CPU_ON after halt; continue
 * Invariant: halt does not require UART
 */

#include <nxu/halt.h>

void
nxu_halt(void)
{
	for (;;) {
		asm volatile("msr daifset, #0xf" ::: "memory");
		asm volatile("wfi" ::: "memory");
	}
}

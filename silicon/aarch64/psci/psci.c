/*
 * NXU MAP
 * Layer: silicon PSCI
 * Owns: CPU_ON calling convention (HVC or SMC)
 * Consumes: conduit, mpidr, entry, context as parameters
 * Produces: firmware return code
 * Hardware: HVC/SMC
 * Forbidden: treating return as online; board CPU numbers
 * Invariant: adapter only
 */
#include <silicon/aarch64/psci/psci.h>

#define PSCI_CPU_ON_64 0xC4000003ULL

nxu_u64
nxu_psci_cpu_on(nxu_u32 conduit, nxu_u64 mpidr, nxu_u64 entry, nxu_u64 ctx)
{
	nxu_u64 fid = PSCI_CPU_ON_64;
	nxu_u64 ret;

	if (conduit == 2U) {
		asm volatile(
		    "mov x0, %1\n"
		    "mov x1, %2\n"
		    "mov x2, %3\n"
		    "mov x3, %4\n"
		    "hvc #0\n"
		    "mov %0, x0"
		    : "=r"(ret)
		    : "r"(fid), "r"(mpidr), "r"(entry), "r"(ctx)
		    : "x0", "x1", "x2", "x3", "memory"
		);
	} else {
		asm volatile(
		    "mov x0, %1\n"
		    "mov x1, %2\n"
		    "mov x2, %3\n"
		    "mov x3, %4\n"
		    ".inst 0xd4000003\n"
		    "mov %0, x0"
		    : "=r"(ret)
		    : "r"(fid), "r"(mpidr), "r"(entry), "r"(ctx)
		    : "x0", "x1", "x2", "x3", "memory"
		);
	}
	return ret;
}

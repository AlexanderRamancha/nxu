/*
 * NXU MAP
 * Layer: silicon PSCI (ops API)
 * Owns: CPU_ON prototype
 * Consumes: conduit + addresses as parameters
 * Produces: op prototype
 * Hardware: PSCI
 * Forbidden: online policy
 * Invariant: operations only
 */
#ifndef NXU_PSCI_H
#define NXU_PSCI_H

#include <nxu/types.h>

nxu_u64 nxu_psci_cpu_on(nxu_u32 conduit, nxu_u64 mpidr, nxu_u64 entry, nxu_u64 ctx);

#endif

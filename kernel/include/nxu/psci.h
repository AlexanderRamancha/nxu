#ifndef NXU_PSCI_H
#define NXU_PSCI_H

#include <nxu/types.h>

#define NXU_PSCI_CPU_ON  0xC4000003ULL

nxu_s64 nxu_psci_cpu_on(nxu_u64 target_cpu, nxu_u64 entry, nxu_u64 context);

#endif
#include <nxu/psci.h>

nxu_s64 nxu_psci_cpu_on(nxu_u64 target_cpu, nxu_u64 entry, nxu_u64 context)
{
    register nxu_u64 x0 asm("x0") = NXU_PSCI_CPU_ON;
    register nxu_u64 x1 asm("x1") = target_cpu;
    register nxu_u64 x2 asm("x2") = entry;
    register nxu_u64 x3 asm("x3") = context;

    asm volatile(
        "hvc #0"
        : "+r"(x0)
        : "r"(x1), "r"(x2), "r"(x3)
        : "memory"
    );

    return (nxu_s64)x0;
}
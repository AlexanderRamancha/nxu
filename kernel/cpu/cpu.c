#include <nxu/cpu.h>
#include <nxu/psci.h>

#include "uart/pl011.h"

/*
 * Architecture map
 *
 * MPIDR_EL1
 *     |
 *     v
 * GIC affinity
 *     |
 *     v
 * NXU CPU topology
 *     |
 *     v
 * logical CPU ID
 *
 * Startup:
 *
 * logical CPU ID
 *     |
 *     v
 * CPU topology
 *     |
 *     v
 * PSCI CPU_ON
 *     |
 *     v
 * secondary CPU
 *     |
 *     v
 * ONLINE
 */

static struct nxu_cpu
cpus[NXU_MAX_CPUS];

static nxu_u32
cpu_count;

static int
nxu_cpu_topology_init(void)
{
    cpu_count = 2U;

    cpus[0].logical_id =
        0U;

    cpus[0].psci_id =
        0U;

    cpus[0].gic_affinity =
        0U;

    cpus[0].state =
        NXU_CPU_ONLINE;

    cpus[1].logical_id =
        1U;

    cpus[1].psci_id =
        1U;

    cpus[1].gic_affinity =
        1U;

    cpus[1].state =
        NXU_CPU_OFFLINE;

    return 0;
}

int
nxu_cpu_init(void)
{
    nxu_u32 i;

    cpu_count =
        0U;

    for (
        i = 0U;
        i < NXU_MAX_CPUS;
        i++
    ) {
        cpus[i].logical_id =
            i;

        cpus[i].psci_id =
            0U;

        cpus[i].gic_affinity =
            0U;

        cpus[i].state =
            NXU_CPU_OFFLINE;
    }

    return nxu_cpu_topology_init();
}

struct nxu_cpu *
nxu_cpu_get(
    nxu_u32 logical_id
)
{
    if (logical_id >= cpu_count)
        return 0;

    return &cpus[logical_id];
}

nxu_u32
nxu_cpu_current_id(void)
{
    nxu_u64 mpidr;
    nxu_u64 affinity;
    nxu_u32 i;

    asm volatile(
        "mrs %0, mpidr_el1"
        : "=r"(mpidr)
        :
        : "memory"
    );

    affinity =
        mpidr &
        0xFF00FFFFFFULL;

    for (
        i = 0U;
        i < cpu_count;
        i++
    ) {
        if (
            cpus[i].gic_affinity ==
            affinity
        )
            return cpus[i].logical_id;
    }

    return NXU_MAX_CPUS;
}

void
nxu_cpu_online(
    nxu_u32 logical_id
)
{
    struct nxu_cpu *cpu;

    cpu =
        nxu_cpu_get(
            logical_id
        );

    if (cpu == 0)
        return;

    cpu->state =
        NXU_CPU_ONLINE;
}

int
nxu_cpu_start(
    nxu_u32 logical_id
)
{
    struct nxu_cpu *cpu;
    nxu_u64 entry;
    nxu_s64 result;

    cpu =
        nxu_cpu_get(
            logical_id
        );

    if (cpu == 0)
        return -1;

    if (
        cpu->state ==
        NXU_CPU_ONLINE
    )
        return 0;

    if (
        cpu->state ==
        NXU_CPU_STARTING
    )
        return -1;

    cpu->state =
        NXU_CPU_STARTING;

    extern void
    nxu_secondary_entry(void);

    entry =
        (nxu_u64)(
            (nxu_uptr)
            nxu_secondary_entry
        );

    result =
        nxu_psci_cpu_on(
            cpu->psci_id,
            entry,
            logical_id
        );

    uart_puts(
        "PSCI CPU_ON result: 0x"
    );

    uart_puthex(
        (nxu_u64)result
    );

    uart_puts(
        "\r\n"
    );

    if (result != 0) {

        cpu->state =
            NXU_CPU_OFFLINE;

        return (int)result;
    }

    return 0;
}
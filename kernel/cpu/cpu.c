#include <nxu/cpu.h>
#include <nxu/psci.h>
#include <nxu/panic.h>
#include <nxu/console.h>
#include <gic/gic.h>
#include <nxu/log.h>

/*
 * Architecture map
 *
 *   MPIDR_EL1
 *         |
 *         v
 *   hardware affinity
 *         |
 *         v
 *   GIC-discovered affinity
 *         |
 *         v
 *   logical CPU
 *
 * Topology is filled from GIC redistributor discovery.
 */

static struct nxu_cpu cpus[NXU_MAX_CPUS];
static nxu_u32 cpu_count;

int
nxu_cpu_init(nxu_u64 boot_affinity)
{
    nxu_u32 i;
    nxu_u32 count;

    nxu_console_puts("[CPU] hardware boot affinity = 0x");
    nxu_console_puthex((unsigned long)boot_affinity);
    nxu_console_puts("\r\n");

    count = nxu_gic.cpu_count;

    nxu_console_puts("[CPU] GIC reported CPU count = ");
    nxu_console_putu32((unsigned long)count);
    nxu_console_puts("\r\n");

    if (count == 0U || count > NXU_MAX_CPUS) {
        nxu_console_puts("[CPU] ERROR: invalid discovered CPU count\r\n");
        return -1;
    }

    cpu_count = 0U;

    nxu_console_puts("[CPU] constructing logical CPU table\r\n");

    for (i = 0U; i < count; i++) {
        cpus[i].logical_id   = i;
        cpus[i].psci_id      = nxu_gic.cpu[i].affinity;
        cpus[i].gic_affinity = nxu_gic.cpu[i].affinity;
        cpus[i].state        = NXU_CPU_OFFLINE;

        nxu_console_puts("[CPU] logical CPU ");
        nxu_console_putu32((unsigned long)i);

        nxu_console_puts(": affinity = 0x");
        nxu_console_puthex((unsigned long)cpus[i].gic_affinity);

        nxu_console_puts(", PSCI ID = 0x");
        nxu_console_puthex((unsigned long)cpus[i].psci_id);

        nxu_console_puts(", state = OFFLINE\r\n");

        cpu_count++;
    }

    nxu_console_puts("[CPU] matching boot hardware affinity\r\n");

    for (i = 0U; i < cpu_count; i++) {
        if (cpus[i].gic_affinity == boot_affinity) {
            cpus[i].state = NXU_CPU_ONLINE;

            nxu_console_puts("[CPU] boot affinity matched logical CPU ");
            nxu_console_putu32((unsigned long)cpus[i].logical_id);
            nxu_console_puts("\r\n");

            nxu_console_puts("[CPU] state: OFFLINE -> ONLINE\r\n");

            return 0;
        }
    }

    nxu_console_puts("[CPU] ERROR: boot affinity not found\r\n");

    return -1;
}

struct nxu_cpu *
nxu_cpu_get(nxu_u32 logical_id)
{
    if (logical_id >= cpu_count)
        return 0;

    return &cpus[logical_id];
}

nxu_u32
nxu_cpu_count(void)
{
    return cpu_count;
}

nxu_u32
nxu_cpu_current_id(void)
{
    nxu_u64 mpidr;
    nxu_u64 affinity;
    nxu_u32 i;

    asm volatile("mrs %0, mpidr_el1"
                 : "=r"(mpidr)
                 :
                 : "memory");

    affinity = mpidr & 0xFF00FFFFFFULL;

    for (i = 0U; i < cpu_count; i++) {
        if (cpus[i].gic_affinity == affinity)
            return cpus[i].logical_id;
    }

    return NXU_MAX_CPUS;
}

void
nxu_cpu_online(nxu_u32 logical_id)
{
    struct nxu_cpu *cpu = nxu_cpu_get(logical_id);

    if (cpu) {
        nxu_console_puts("[CPU] logical CPU ");
        nxu_console_putu32((unsigned long)logical_id);
        nxu_console_puts(": state -> ONLINE\r\n");

        cpu->state = NXU_CPU_ONLINE;


    struct nxu_log_event event = {
        .subsystem = NXU_LOG_SUBSYSTEM_CPU,
        .code = NXU_LOG_EVENT_CPU_ONLINE
    };

    if (nxu_log_event(
            &event,
            &logical_id,
            sizeof(logical_id)
        ) != 0) {
        nxu_console_puts(
            "[LOG] CPU_ONLINE record failed\r\n"
        );
    }
    }
}


int
nxu_cpu_start(nxu_u32 logical_id)
{
    struct nxu_cpu *cpu;
    nxu_u64 entry;
    nxu_s64 result;

    cpu = nxu_cpu_get(logical_id);

    if (!cpu) {
        nxu_console_puts("[CPU] ERROR: invalid logical CPU\r\n");
        return -1;
    }

    nxu_console_puts("[CPU] start request for logical CPU ");
    nxu_console_putu32((unsigned long)logical_id);
    nxu_console_puts("\r\n");

    if (cpu->state == NXU_CPU_ONLINE) {
        nxu_console_puts("[CPU] CPU already ONLINE\r\n");
        return 0;
    }

    if (cpu->state == NXU_CPU_STARTING) {
        nxu_console_puts("[CPU] ERROR: CPU already STARTING\r\n");
        return -1;
    }

    nxu_console_puts("[CPU] state: OFFLINE -> STARTING\r\n");

    cpu->state = NXU_CPU_STARTING;

    extern void nxu_secondary_entry(void);

    entry = (nxu_u64)(nxu_uptr)nxu_secondary_entry;

    nxu_console_puts("[CPU] PSCI target affinity = 0x");
    nxu_console_puthex((unsigned long)cpu->psci_id);
    nxu_console_puts("\r\n");

    nxu_console_puts("[CPU] secondary entry = 0x");
    nxu_console_puthex((unsigned long)entry);
    nxu_console_puts("\r\n");

    nxu_console_puts("[CPU] issuing PSCI CPU_ON\r\n");

    result = nxu_psci_cpu_on(cpu->psci_id, entry, logical_id);

#ifdef DEBUG
    nxu_console_puts("PSCI CPU_ON: 0x");
    nxu_console_puthex((unsigned long)result);
    nxu_console_puts("\r\n");
#endif

    if (result != 0) {
        nxu_console_puts("[CPU] PSCI CPU_ON failed\r\n");

        cpu->state = NXU_CPU_OFFLINE;

        nxu_console_puts("[CPU] state: STARTING -> OFFLINE\r\n");

        return (int)result;
    }

    nxu_console_puts("[CPU] PSCI CPU_ON returned SUCCESS\r\n");

    return 0;
}
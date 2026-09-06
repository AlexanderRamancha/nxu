#ifndef NXU_CPU_H
#define NXU_CPU_H

#include <nxu/types.h>

#define NXU_MAX_CPUS              16U
#define NXU_MAX_INTERRUPT_NESTING 16U

/*
 * Architecture map
 *
 *   platform / discovery
 *         |
 *         v
 *   CPU topology
 *         |
 *         +-- logical ID
 *         +-- PSCI ID
 *         +-- GIC affinity
 *         +-- lifecycle state
 *         +-- per-CPU interrupt context
 *
 * CPU objects are owned by the kernel.
 * Hardware affinity is discovered, not assumed.
 */

enum nxu_cpu_state {
    NXU_CPU_OFFLINE = 0,
    NXU_CPU_STARTING,
    NXU_CPU_ONLINE
};

struct nxu_interrupt_context_frame {
    nxu_u32 intid;
    nxu_u8  priority;
    nxu_u8  active;
};

struct nxu_interrupt_cpu_context {
    nxu_u32 depth;
    struct nxu_interrupt_context_frame frames[NXU_MAX_INTERRUPT_NESTING];
};

struct nxu_cpu {
    nxu_u32 logical_id;
    nxu_u64 psci_id;
    nxu_u64 gic_affinity;
    enum nxu_cpu_state state;
    struct nxu_interrupt_cpu_context interrupt_context;
};

int nxu_cpu_init(nxu_u64 boot_affinity);
struct nxu_cpu *nxu_cpu_get(nxu_u32 logical_id);
int nxu_cpu_start(nxu_u32 logical_id);
void nxu_cpu_online(nxu_u32 logical_id);
nxu_u32 nxu_cpu_current_id(void);
nxu_u32 nxu_cpu_count(void);

#endif
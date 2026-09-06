#ifndef NXU_GIC_H
#define NXU_GIC_H

#include <nxu/types.h>
#include <nxu/interrupt.h>

#ifndef NXU_MAX_CPUS
#define NXU_MAX_CPUS 16U
#endif

#define NXU_GIC_FEATURE_SINGLE_SECURITY  (1U << 0)
#define NXU_GIC_FEATURE_AFFINITY_ROUTING (1U << 1)

/*
 * Architecture map
 *
 *   platform bases
 *         |
 *         v
 *   discovery
 *         |
 *         +-- distributor capabilities
 *         +-- redistributor topology
 *         +-- CPU affinity
 *         |
 *         v
 *   initialization (default-deny)
 *         |
 *         v
 *   runtime (ack / eoi / sgi)
 *
 * GIC owns only hardware state.
 */

struct nxu_gic_cpu {
    nxu_u32 logical_id;
    nxu_u64 affinity;
    nxu_uptr redistributor_base;
    nxu_u32 initialized;
};

struct nxu_gic {
    nxu_uptr distributor_base;
    nxu_uptr redistributor_base;
    nxu_uptr redistributor_stride;

    nxu_u32 distributor_typer;
    nxu_u32 distributor_control;
    nxu_u32 interrupt_count;
    nxu_u32 intid_bits;
    nxu_u32 priority_bits;
    nxu_u32 implementation_id;
    nxu_u64 redistributor_typer;
    nxu_u32 cpu_count;
    nxu_u32 features;

    struct nxu_gic_cpu cpu[NXU_MAX_CPUS];
};

extern struct nxu_gic nxu_gic;

int nxu_gic_discover(void);
int nxu_gic_init(nxu_u32 boot_cpu_id);
int nxu_gic_cpu_init(nxu_u32 cpu_id);

nxu_uptr nxu_gic_get_redistributor(nxu_u32 cpu_id);
int nxu_gic_get_affinity(nxu_u32 cpu_id, nxu_u64 *affinity);

nxu_u32 nxu_gic_acknowledge_interrupt(void);
void nxu_gic_end_interrupt(nxu_u32 intid);
int nxu_gic_send_sgi(nxu_u32 target_cpu, nxu_u32 intid);

int nxu_gic_create_interrupt(nxu_u32 intid, struct nxu_interrupt *interrupt);
void nxu_gic_interrupt_backend_init(void);

int nxu_gic_configure_local_ppi(
    nxu_u32 cpu_id,
    nxu_u32 intid,
    nxu_u8 priority
);

#endif
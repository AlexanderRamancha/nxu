/*
 * Architecture map
 *
 *   platform bases
 *         |
 *         v
 *   discover TYPER / security / redistributors
 *         |
 *         v
 *   distributor init (Group 1, clear pending, default-deny)
 *         |
 *         v
 *   per-CPU redistributor wake + local Group 1 + CPU interface
 *         |
 *         v
 *   runtime: ICC_IAR1 / ICC_EOIR1 / ICC_SGI1R
 *
 * All enable bits start cleared.
 * Bounded wait on redistributor wake.
 */

#include "gic.h"
#include "gic_reg.h"

#include <nxu/mmio.h>
#include <nxu/panic.h>
#include <nxu/console.h>

#include "platform/platform.h"

struct nxu_gic nxu_gic;

static nxu_u32
gicd_read(nxu_u32 offset)
{
    return mmio_read32(nxu_gic.distributor_base + offset);
}

static void
gicd_write(nxu_u32 offset, nxu_u32 value)
{
    mmio_write32(nxu_gic.distributor_base + offset, value);
}


/*
 * ============================================================
 * GIC DISCOVERY
 * ============================================================
 */

static void
gic_discover_typer(void)
{
    nxu_u32 typer = gicd_read(GICD_TYPER);
    nxu_u32 it_lines =
        (typer >> GICD_TYPER_ITLINES_SHIFT) & GICD_TYPER_ITLINES_MASK;
    nxu_u32 id_bits =
        (typer >> GICD_TYPER_IDBITS_SHIFT) & GICD_TYPER_IDBITS_MASK;

    nxu_gic.distributor_typer = typer;
    nxu_gic.interrupt_count   = 32U * (it_lines + 1U);
    nxu_gic.intid_bits        = id_bits + 1U;
    nxu_gic.priority_bits     = 5U;

    nxu_console_puts("[GIC] GICD_TYPER = 0x");
    nxu_console_puthex((unsigned long)typer);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] interrupt capacity = ");
    nxu_console_putu32((unsigned long)nxu_gic.interrupt_count);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] INTID bits = ");
    nxu_console_putu32((unsigned long)nxu_gic.intid_bits);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] priority bits = ");
    nxu_console_putu32((unsigned long)nxu_gic.priority_bits);
    nxu_console_puts("\r\n");
}

static void
gic_discover_security(void)
{
    nxu_u32 ctlr = gicd_read(GICD_CTLR);

    nxu_gic.distributor_control = ctlr;

    nxu_console_puts("[GIC] GICD_CTLR = 0x");
    nxu_console_puthex((unsigned long)ctlr);
    nxu_console_puts("\r\n");

    if (ctlr & GICD_CTLR_DS) {
        nxu_gic.features |= NXU_GIC_FEATURE_SINGLE_SECURITY;

        nxu_console_puts("[GIC] security model: single-security-state\r\n");
    } else {
        nxu_console_puts("[GIC] security model: dual-security-state\r\n");
    }
}

static int
gic_discover_redistributors(void)
{
    nxu_uptr base = nxu_gic.redistributor_base;
    nxu_uptr stride = nxu_gic.redistributor_stride
                      ? nxu_gic.redistributor_stride
                      : GICR_FRAME_SIZE;
    nxu_u32 cpu;

    nxu_gic.cpu_count = 0U;

    nxu_console_puts("[GIC] discovering redistributors\r\n");

    for (cpu = 0U; cpu < NXU_MAX_CPUS; cpu++) {
        nxu_u64 typer = mmio_read64(base + GICR_TYPER);
        nxu_u64 affinity = typer >> 32;

        nxu_gic.cpu[cpu].logical_id         = cpu;
        nxu_gic.cpu[cpu].affinity           = affinity;
        nxu_gic.cpu[cpu].redistributor_base = base;
        nxu_gic.cpu[cpu].initialized        = 0U;

        nxu_gic.cpu_count++;

        nxu_console_puts("[GIC] CPU ");
        nxu_console_putu32((unsigned long)cpu);

        nxu_console_puts(": redistributor = 0x");
        nxu_console_puthex((unsigned long)base);

        nxu_console_puts(", affinity = 0x");
        nxu_console_puthex((unsigned long)affinity);

        nxu_console_puts("\r\n");

        if (typer & GICR_TYPER_LAST)
            break;

        base += stride;
    }

    nxu_console_puts("[GIC] redistributors discovered = ");
    nxu_console_putu32((unsigned long)nxu_gic.cpu_count);
    nxu_console_puts("\r\n");

    return (nxu_gic.cpu_count == 0U) ? -1 : 0;
}

int
nxu_gic_discover(void)
{
    const struct nxu_platform *platform = nxu_platform_get();

    if (!platform)
        return -1;

    nxu_console_puts("[GIC] platform information\r\n");

    nxu_console_puts("[GIC] distributor = 0x");
    nxu_console_puthex((unsigned long)platform->gic_distributor_base);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] redistributor = 0x");
    nxu_console_puthex((unsigned long)platform->gic_redistributor_base);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] redistributor stride = 0x");
    nxu_console_puthex((unsigned long)platform->gic_redistributor_stride);
    nxu_console_puts("\r\n");

    nxu_gic.distributor_base     = platform->gic_distributor_base;
    nxu_gic.redistributor_base   = platform->gic_redistributor_base;
    nxu_gic.redistributor_stride = platform->gic_redistributor_stride;

    nxu_gic.distributor_typer   = 0U;
    nxu_gic.distributor_control = 0U;
    nxu_gic.interrupt_count     = 0U;
    nxu_gic.intid_bits          = 0U;
    nxu_gic.priority_bits       = 0U;
    nxu_gic.implementation_id   = 0U;
    nxu_gic.cpu_count           = 0U;
    nxu_gic.features            = 0U;

    nxu_console_puts("[GIC] reading distributor properties\r\n");

    gic_discover_typer();

    nxu_gic.implementation_id = gicd_read(GICD_IIDR);

    nxu_console_puts("[GIC] GICD_IIDR = 0x");
    nxu_console_puthex((unsigned long)nxu_gic.implementation_id);
    nxu_console_puts("\r\n");

    gic_discover_security();

    if (gic_discover_redistributors() != 0)
        return -1;

    nxu_gic.features |= NXU_GIC_FEATURE_AFFINITY_ROUTING;

    nxu_console_puts("[GIC] affinity routing capability recorded\r\n");

    return 0;
}


/*
 * ============================================================
 * GIC REDISTRIBUTOR
 * ============================================================
 */

static int
gic_wake_redistributor(nxu_uptr rdist)
{
    nxu_u32 waker;

    nxu_console_puts("[GIC] waking redistributor at 0x");
    nxu_console_puthex((unsigned long)rdist);
    nxu_console_puts("\r\n");

    waker = mmio_read32(rdist + GICR_WAKER);

    nxu_console_puts("[GIC] GICR_WAKER before = 0x");
    nxu_console_puthex((unsigned long)waker);
    nxu_console_puts("\r\n");

    waker &= ~GICR_WAKER_PROCESSOR_SLEEP;

    mmio_write32(rdist + GICR_WAKER, waker);

    asm volatile("dsb sy" ::: "memory");

    for (nxu_u32 i = 0; i < 100000U; i++) {
        waker = mmio_read32(rdist + GICR_WAKER);

        if ((waker & GICR_WAKER_CHILDREN_ASLEEP) == 0U) {
            nxu_console_puts("[GIC] redistributor awake\r\n");
            return 0;
        }
    }

    nxu_console_puts("[GIC] ERROR: redistributor wake timeout\r\n");

    return -1;
}

static int
gic_configure_local_group1(nxu_uptr rdist)
{
    nxu_console_puts("[GIC] configuring local interrupts as Group 1\r\n");

    mmio_write32(rdist + GICR_IGROUPR0, 0xFFFFFFFFU);

    asm volatile("dsb sy" ::: "memory");

    if (mmio_read32(rdist + GICR_IGROUPR0) == 0xFFFFFFFFU) {
        nxu_console_puts("[GIC] local Group 1 configuration verified\r\n");
        return 0;
    }

    nxu_console_puts("[GIC] ERROR: local Group 1 verification failed\r\n");

    return -1;
}

static int
gic_disable_local_interrupts(nxu_uptr rdist)
{
    nxu_console_puts("[GIC] disabling local interrupt enables\r\n");

    mmio_write32(rdist + GICR_ICENABLER0, 0xFFFFFFFFU);

    asm volatile("dsb sy" ::: "memory");

    return 0;
}

static int
gic_clear_local_pending(nxu_uptr rdist)
{
    nxu_console_puts("[GIC] clearing local pending interrupts\r\n");

    mmio_write32(rdist + GICR_ICPENDR0, 0xFFFFFFFFU);

    asm volatile("dsb sy" ::: "memory");

    return 0;
}


/*
 * ============================================================
 * GIC CPU INTERFACE
 * ============================================================
 */

static int
gic_cpu_interface_init(void)
{
    nxu_u64 value;

    nxu_console_puts("[GIC] enabling system register interface\r\n");

    asm volatile("mrs %0, ICC_SRE_EL1"
                 : "=r"(value)
                 :
                 : "memory");

    value |= 1ULL;

    asm volatile("msr ICC_SRE_EL1, %0"
                 :
                 : "r"(value)
                 : "memory");

    asm volatile("isb" ::: "memory");

    nxu_console_puts("[GIC] configuring ICC_CTLR_EL1\r\n");

    asm volatile("mrs %0, ICC_CTLR_EL1"
                 : "=r"(value)
                 :
                 : "memory");

    value &= ~(1ULL << 1);

    asm volatile("msr ICC_CTLR_EL1, %0"
                 :
                 : "r"(value)
                 : "memory");

    asm volatile("isb" ::: "memory");

    nxu_console_puts("[GIC] setting priority mask = 0xFF\r\n");

    asm volatile("msr ICC_PMR_EL1, %0"
                 :
                 : "r"(0xFFULL)
                 : "memory");

    asm volatile("isb" ::: "memory");

    nxu_console_puts("[GIC] enabling Group 1 interrupts\r\n");

    asm volatile("msr ICC_IGRPEN1_EL1, %0"
                 :
                 : "r"(1ULL)
                 : "memory");

    asm volatile("isb" ::: "memory");

    nxu_console_puts("[GIC] CPU interface ready\r\n");

    return 0;
}


/*
 * ============================================================
 * GIC DISTRIBUTOR
 * ============================================================
 */

static int
gic_distributor_init(void)
{
    nxu_u32 registers = (nxu_gic.interrupt_count + 31U) / 32U;
    nxu_u32 i;

    nxu_console_puts("[GIC] initializing distributor\r\n");

    nxu_console_puts("[GIC] distributor register groups = ");
    nxu_console_putu32((unsigned long)registers);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] disabling distributor before configuration\r\n");

    gicd_write(GICD_CTLR, 0U);

    asm volatile("dsb sy" ::: "memory");

    nxu_console_puts("[GIC] configuring SPI Group 1\r\n");

    for (i = 1U; i < registers; i++)
        gicd_write(GICD_IGROUPR + (i * 4U), 0xFFFFFFFFU);

    nxu_console_puts("[GIC] clearing SPI pending state\r\n");

    for (i = 1U; i < registers; i++)
        gicd_write(GICD_ICPENDR + (i * 4U), 0xFFFFFFFFU);

    nxu_console_puts("[GIC] disabling SPI interrupts\r\n");

    for (i = 1U; i < registers; i++)
        gicd_write(GICD_ICENABLER + (i * 4U), 0xFFFFFFFFU);

    nxu_console_puts("[GIC] enabling distributor Group 1\r\n");

    gicd_write(GICD_CTLR,
               GICD_CTLR_ARE_NS |
               GICD_CTLR_ENABLE_GRP1_NS);

    asm volatile("dsb sy" ::: "memory");

    nxu_gic.distributor_control = gicd_read(GICD_CTLR);

    nxu_console_puts("[GIC] distributor control after init = 0x");
    nxu_console_puthex((unsigned long)nxu_gic.distributor_control);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] distributor initialized\r\n");

    return 0;
}


/*
 * ============================================================
 * PER-CPU GIC INITIALIZATION
 * ============================================================
 */

int
nxu_gic_cpu_init(nxu_u32 cpu_id)
{
    nxu_uptr rdist;

    if (cpu_id >= nxu_gic.cpu_count)
        return -1;

    rdist = nxu_gic.cpu[cpu_id].redistributor_base;

    nxu_console_puts("[GIC] initializing logical CPU ");
    nxu_console_putu32((unsigned long)cpu_id);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] CPU affinity = 0x");
    nxu_console_puthex((unsigned long)nxu_gic.cpu[cpu_id].affinity);
    nxu_console_puts("\r\n");

    nxu_console_puts("[GIC] redistributor base = 0x");
    nxu_console_puthex((unsigned long)rdist);
    nxu_console_puts("\r\n");

    if (gic_wake_redistributor(rdist) != 0)
        return -1;

    if (gic_disable_local_interrupts(rdist) != 0)
        return -1;

    if (gic_clear_local_pending(rdist) != 0)
        return -1;

    if (gic_configure_local_group1(rdist) != 0)
        return -1;

    if (gic_cpu_interface_init() != 0)
        return -1;

    nxu_gic.cpu[cpu_id].initialized = 1U;

    nxu_console_puts("[GIC] CPU ");
    nxu_console_putu32((unsigned long)cpu_id);
    nxu_console_puts(" GIC initialization complete\r\n");

    return 0;
}


/*
 * ============================================================
 * GIC INITIALIZATION
 * ============================================================
 */

int
nxu_gic_init(nxu_u32 boot_cpu_id)
{
    nxu_console_puts("[GIC] initialization starting\r\n");

    nxu_console_puts("[GIC] boot CPU logical ID = ");
    nxu_console_putu32((unsigned long)boot_cpu_id);
    nxu_console_puts("\r\n");

    if (nxu_gic.interrupt_count == 0U ||
        nxu_gic.cpu_count == 0U)
        return -1;

    if (gic_distributor_init() != 0)
        return -1;

    if (nxu_gic_cpu_init(boot_cpu_id) != 0)
        return -1;

    nxu_console_puts("[GIC] initialization complete\r\n");

    return 0;
}


/*
 * ============================================================
 * RUNTIME ACCESS
 * ============================================================
 */

nxu_uptr
nxu_gic_get_redistributor(nxu_u32 cpu_id)
{
    if (cpu_id >= nxu_gic.cpu_count)
        return 0U;

    return nxu_gic.cpu[cpu_id].redistributor_base;
}

int
nxu_gic_get_affinity(nxu_u32 cpu_id, nxu_u64 *affinity)
{
    if (!affinity || cpu_id >= nxu_gic.cpu_count)
        return -1;

    *affinity = nxu_gic.cpu[cpu_id].affinity;

    return 0;
}

nxu_u32
nxu_gic_acknowledge_interrupt(void)
{
    nxu_u64 value;

    asm volatile("mrs %0, ICC_IAR1_EL1"
                 : "=r"(value)
                 :
                 : "memory");

    return (nxu_u32)value;
}

void
nxu_gic_end_interrupt(nxu_u32 intid)
{
    asm volatile("msr ICC_EOIR1_EL1, %0"
                 :
                 : "r"((nxu_u64)intid)
                 : "memory");

    asm volatile("isb" ::: "memory");
}

int
nxu_gic_send_sgi(nxu_u32 target_cpu, nxu_u32 intid)
{
    nxu_u64 affinity;
    nxu_u64 value;
    nxu_u32 aff0;
    nxu_u32 aff1;
    nxu_u32 aff2;
    nxu_u32 aff3;

    if (target_cpu >= nxu_gic.cpu_count || intid > 15U)
        return -1;

    if (nxu_gic_get_affinity(target_cpu, &affinity) != 0)
        return -1;

    aff0 = (nxu_u32)(affinity & 0xFFULL);
    aff1 = (nxu_u32)((affinity >> 8) & 0xFFULL);
    aff2 = (nxu_u32)((affinity >> 16) & 0xFFULL);
    aff3 = (nxu_u32)((affinity >> 24) & 0xFFULL);

    if (aff0 >= 16U)
        return -1;

    value  = ((nxu_u64)aff3) << 48;
    value |= ((nxu_u64)aff2) << 32;
    value |= ((nxu_u64)(intid & 0xFU)) << 24;
    value |= ((nxu_u64)aff1) << 16;
    value |= 1ULL << aff0;

    asm volatile("msr ICC_SGI1R_EL1, %0"
                 :
                 : "r"(value)
                 : "memory");

    asm volatile("isb" ::: "memory");

    return 0;
}
#include "gic.h"
#include "gic_reg.h"

#include <nxu/mmio.h>
#include <uart/pl011.h>

#include "../platform/platform.h"


/*
 * Architecture map
 *
 *     Platform
 *        |
 *        +-- GIC Distributor base
 *        +-- GIC Redistributor base
 *        |
 *        v
 *     GIC discovery
 *        |
 *        +-- Distributor capabilities
 *        +-- Security model
 *        +-- Redistributor topology
 *        +-- CPU affinity
 *        |
 *        v
 *     GIC initialization
 *        |
 *        +-- Distributor
 *        +-- Redistributor
 *        +-- CPU interface
 *        |
 *        v
 *     Runtime
 *        |
 *        +-- acknowledge interrupt
 *        +-- end interrupt
 *        +-- send SGI
 *
 * This file implements GICv3 hardware behaviour.
 *
 * Platform-specific physical addresses are supplied
 * by the platform layer.
 */


/*
 * ==========================================================================
 * Global GIC state
 * ==========================================================================
 */

struct nxu_gic nxu_gic;


/*
 * ==========================================================================
 * Distributor access
 * ==========================================================================
 */

static nxu_u32
gicd_read(
    nxu_u32 offset
)
{
    return mmio_read32(
        nxu_gic.distributor_base + offset
    );
}


static void
gicd_write(
    nxu_u32 offset,
    nxu_u32 value
)
{
    mmio_write32(
        nxu_gic.distributor_base + offset,
        value
    );
}


/*
 * ==========================================================================
 * Distributor discovery
 * ==========================================================================
 */

static void
gic_discover_typer(void)
{
    nxu_u32 typer;
    nxu_u32 it_lines;
    nxu_u32 id_bits;


    typer =
        gicd_read(GICD_TYPER);


    nxu_gic.distributor_typer =
        typer;


    it_lines =
        (typer >> GICD_TYPER_ITLINES_SHIFT) &
        GICD_TYPER_ITLINES_MASK;


    id_bits =
        (typer >> GICD_TYPER_IDBITS_SHIFT) &
        GICD_TYPER_IDBITS_MASK;


    /*
     * ITLinesNumber encodes the number of
     * 32-interrupt blocks minus one.
     */
    nxu_gic.interrupt_count =
        32U * (it_lines + 1U);


    /*
     * IDbits encodes the number of implemented
     * INTID bits minus one.
     */
    nxu_gic.intid_bits =
        id_bits + 1U;
}


static void
gic_discover_security(void)
{
    nxu_u32 ctlr;


    ctlr =
        gicd_read(GICD_CTLR);


    nxu_gic.distributor_control =
        ctlr;


    if (ctlr & GICD_CTLR_DS)
        nxu_gic.features |=
            NXU_GIC_FEATURE_SINGLE_SECURITY;
}


/*
 * ==========================================================================
 * Redistributor discovery
 * ==========================================================================
 *
 * Each Redistributor corresponds to one processing element.
 *
 * NXU records:
 *
 *     logical CPU
 *     GIC affinity
 *     Redistributor address
 *
 * GICR_TYPER.LAST identifies the final Redistributor.
 */

static int
gic_discover_redistributors(void)
{
    nxu_uptr base;
    nxu_u32 cpu;


    base =
        nxu_gic.redistributor_base;


    nxu_gic.cpu_count =
        0U;


    for (
        cpu = 0U;
        cpu < NXU_MAX_CPUS;
        cpu++
    ) {

        nxu_u64 typer;
        nxu_u64 affinity;


        typer =
            mmio_read64(
                base + GICR_TYPER
            );


        affinity =
            typer >> 32;


        nxu_gic.cpu[cpu].logical_id =
            cpu;


        nxu_gic.cpu[cpu].affinity =
            affinity;


        nxu_gic.cpu[cpu].redistributor_base =
            base;


        nxu_gic.cpu[cpu].initialized =
            0U;


        nxu_gic.cpu_count++;


        if (typer & GICR_TYPER_LAST)
            break;


        base +=
            GICR_FRAME_SIZE;
    }


    if (nxu_gic.cpu_count == 0U)
        return -1;


    return 0;
}


/*
 * ==========================================================================
 * GIC discovery
 * ==========================================================================
 */

int
nxu_gic_discover(void)
{
    const struct nxu_platform *platform;

    platform =
        nxu_platform_get();

    if (platform == 0)
        return -1;

    nxu_gic.distributor_base =
        platform->gic_distributor_base;

    nxu_gic.redistributor_base =
        platform->gic_redistributor_base;

    nxu_gic.distributor_typer = 0U;
    nxu_gic.distributor_control = 0U;
    nxu_gic.interrupt_count = 0U;
    nxu_gic.intid_bits = 0U;
    nxu_gic.priority_bits = 0U;
    nxu_gic.implementation_id = 0U;
    nxu_gic.redistributor_typer = 0U;
    nxu_gic.cpu_count = 0U;
    nxu_gic.features = 0U;

    uart_puts("GIC: typer\r\n");

    gic_discover_typer();

    uart_puts("GIC: iidr\r\n");

    nxu_gic.implementation_id =
        gicd_read(GICD_IIDR);

    uart_puts("GIC: security\r\n");

    gic_discover_security();

    uart_puts("GIC: redistributors\r\n");

    if (gic_discover_redistributors() != 0)
        return -1;

    uart_puts("GIC: topology ready\r\n");

    nxu_gic.features |=
        NXU_GIC_FEATURE_AFFINITY_ROUTING;

    nxu_gic.priority_bits =
        5U;

    return 0;
}

/*
 * ==========================================================================
 * Redistributor wake-up
 * ==========================================================================
 */

static int
gic_wake_redistributor(
    nxu_uptr rdist
)
{
    nxu_u32 waker;


    waker =
        mmio_read32(
            rdist + GICR_WAKER
        );


    waker &=
        ~GICR_WAKER_PROCESSOR_SLEEP;


    mmio_write32(
        rdist + GICR_WAKER,
        waker
    );


    asm volatile(
        "dsb sy"
        ::: "memory"
    );


    /*
     * Wait until the Redistributor reports that
     * its children are awake.
     */
    do {

        waker =
            mmio_read32(
                rdist + GICR_WAKER
            );

    } while (
        waker &
        GICR_WAKER_CHILDREN_ASLEEP
    );


    return 0;
}


/*
 * ==========================================================================
 * CPU-local interrupt groups
 * ==========================================================================
 */

static int
gic_configure_local_group1(
    nxu_uptr rdist
)
{
    nxu_u32 value;


    /*
     * SGIs and PPIs use Group 1.
     */
    mmio_write32(
        rdist + GICR_IGROUPR0,
        0xFFFFFFFFU
    );


    asm volatile(
        "dsb sy"
        ::: "memory"
    );


    value =
        mmio_read32(
            rdist + GICR_IGROUPR0
        );


    if (value != 0xFFFFFFFFU)
        return -1;


    return 0;
}


/*
 * ==========================================================================
 * CPU-local interrupt state
 * ==========================================================================
 */

static int
gic_disable_local_interrupts(
    nxu_uptr rdist
)
{
    mmio_write32(
        rdist + GICR_ICENABLER0,
        0xFFFFFFFFU
    );


    asm volatile(
        "dsb sy"
        ::: "memory"
    );


    return 0;
}


static int
gic_clear_local_pending(
    nxu_uptr rdist
)
{
    mmio_write32(
        rdist + GICR_ICPENDR0,
        0xFFFFFFFFU
    );


    asm volatile(
        "dsb sy"
        ::: "memory"
    );


    return 0;
}


/*
 * ==========================================================================
 * CPU interface
 * ==========================================================================
 */

static int
gic_cpu_interface_init(void)
{
    nxu_u64 value;


    /*
     * Enable access to the GIC CPU interface
     * through system registers.
     */
    asm volatile(
        "mrs %0, ICC_SRE_EL1"
        : "=r"(value)
        :
        : "memory"
    );


    value |=
        1ULL;


    asm volatile(
        "msr ICC_SRE_EL1, %0"
        :
        : "r"(value)
        : "memory"
    );


    asm volatile(
        "isb"
        ::: "memory"
    );


    /*
     * Use GIC EOI mode 0.
     */
    asm volatile(
        "mrs %0, ICC_CTLR_EL1"
        : "=r"(value)
        :
        : "memory"
    );


    value &=
        ~(1ULL << 1);


    asm volatile(
        "msr ICC_CTLR_EL1, %0"
        :
        : "r"(value)
        : "memory"
    );


    asm volatile(
        "isb"
        ::: "memory"
    );


    /*
     * Allow all implemented priorities.
     */
    asm volatile(
        "msr ICC_PMR_EL1, %0"
        :
        : "r"(0xFFULL)
        : "memory"
    );


    asm volatile(
        "isb"
        ::: "memory"
    );


    /*
     * Enable Non-secure Group 1 interrupts.
     */
    asm volatile(
        "msr ICC_IGRPEN1_EL1, %0"
        :
        : "r"(1ULL)
        : "memory"
    );


    asm volatile(
        "isb"
        ::: "memory"
    );


    return 0;
}


/*
 * ==========================================================================
 * Distributor initialization
 * ==========================================================================
 */

static int
gic_distributor_init(void)
{
    nxu_u32 ctlr;
    nxu_u32 registers;
    nxu_u32 i;


    /*
     * Disable the Distributor while configuring it.
     */
    gicd_write(
        GICD_CTLR,
        0U
    );


    asm volatile(
        "dsb sy"
        ::: "memory"
    );


    registers =
        (nxu_gic.interrupt_count + 31U) /
        32U;


    /*
     * SPIs are Group 1.
     *
     * Register zero contains SGIs and PPIs,
     * so it is intentionally excluded.
     */
    for (
        i = 1U;
        i < registers;
        i++
    ) {

        gicd_write(
            GICD_IGROUPR +
            (i * 4U),
            0xFFFFFFFFU
        );
    }


    /*
     * Clear stale SPI pending state.
     */
    for (
        i = 1U;
        i < registers;
        i++
    ) {

        gicd_write(
            GICD_ICPENDR +
            (i * 4U),
            0xFFFFFFFFU
        );
    }


    /*
     * Default-deny:
     *
     * SPIs remain disabled until the
     * interrupt manager explicitly enables them.
     */
    for (
        i = 1U;
        i < registers;
        i++
    ) {

        gicd_write(
            GICD_ICENABLER +
            (i * 4U),
            0xFFFFFFFFU
        );
    }


    /*
     * Enable Non-secure Group 1 and
     * Non-secure affinity routing.
     */
    ctlr =
        GICD_CTLR_ARE_NS |
        GICD_CTLR_ENABLE_GRP1_NS;


    gicd_write(
        GICD_CTLR,
        ctlr
    );


    asm volatile(
        "dsb sy"
        ::: "memory"
    );


    nxu_gic.distributor_control =
        gicd_read(GICD_CTLR);


    return 0;
}


/*
 * ==========================================================================
 * Per-CPU GIC initialization
 * ==========================================================================
 *
 * This function must execute on the CPU whose
 * Redistributor is being initialized.
 */

int
nxu_gic_cpu_init(
    nxu_u32 cpu_id
)
{
    nxu_uptr rdist;


    if (cpu_id >= nxu_gic.cpu_count)
        return -1;


    rdist =
        nxu_gic.cpu[cpu_id]
            .redistributor_base;


    if (gic_wake_redistributor(
            rdist
        ) != 0)
        return -1;


    /*
     * Default-deny local interrupts.
     */
    if (gic_disable_local_interrupts(
            rdist
        ) != 0)
        return -1;


    /*
     * Clear stale local pending state.
     */
    if (gic_clear_local_pending(
            rdist
        ) != 0)
        return -1;


    /*
     * Configure SGIs and PPIs as Group 1.
     */
    if (gic_configure_local_group1(
            rdist
        ) != 0)
        return -1;


    /*
     * Initialize the CPU interface.
     */
    if (gic_cpu_interface_init() != 0)
        return -1;


    nxu_gic.cpu[cpu_id]
        .initialized = 1U;


    return 0;
}


/*
 * ==========================================================================
 * Global GIC initialization
 * ==========================================================================
 */

int
nxu_gic_init(void)
{
    /*
     * Discovery must complete first.
     */
    if (nxu_gic.interrupt_count == 0U ||
        nxu_gic.cpu_count == 0U)
        return -1;


    /*
     * Initialize the Distributor once.
     */
    if (gic_distributor_init() != 0)
        return -1;


    /*
     * CPU0 initializes its local Redistributor
     * and CPU interface.
     */
    if (nxu_gic_cpu_init(0U) != 0)
        return -1;


    return 0;
}


/*
 * ==========================================================================
 * Redistributor lookup
 * ==========================================================================
 */

nxu_uptr
nxu_gic_get_redistributor(
    nxu_u32 cpu_id
)
{
    if (cpu_id >= nxu_gic.cpu_count)
        return 0U;


    return nxu_gic.cpu[cpu_id]
        .redistributor_base;
}


/*
 * ==========================================================================
 * CPU affinity lookup
 * ==========================================================================
 */

int
nxu_gic_get_affinity(
    nxu_u32 cpu_id,
    nxu_u64 *affinity
)
{
    if (affinity == 0)
        return -1;


    if (cpu_id >= nxu_gic.cpu_count)
        return -1;


    *affinity =
        nxu_gic.cpu[cpu_id]
            .affinity;


    return 0;
}


/*
 * ==========================================================================
 * Interrupt acknowledge
 * ==========================================================================
 */

nxu_u32
nxu_gic_acknowledge_interrupt(void)
{
    nxu_u64 value;


    asm volatile(
        "mrs %0, ICC_IAR1_EL1"
        : "=r"(value)
        :
        : "memory"
    );


    return (nxu_u32)value;
}


/*
 * ==========================================================================
 * End of interrupt
 * ==========================================================================
 */

void
nxu_gic_end_interrupt(
    nxu_u32 intid
)
{
    asm volatile(
        "msr ICC_EOIR1_EL1, %0"
        :
        : "r"((nxu_u64)intid)
        : "memory"
    );


    asm volatile(
        "isb"
        ::: "memory"
    );
}


/*
 * ==========================================================================
 * Send SGI
 * ==========================================================================
 *
 * ICC_SGI1R_EL1:
 *
 *     Aff3       [55:48]
 *     Aff2       [39:32]
 *     INTID      [27:24]
 *     Aff1       [23:16]
 *     TargetList [15:0]
 *
 * NXU currently sends an SGI to one target CPU.
 */

int
nxu_gic_send_sgi(
    nxu_u32 target_cpu,
    nxu_u32 intid
)
{
    nxu_u64 affinity;
    nxu_u64 value;

    nxu_u32 aff0;
    nxu_u32 aff1;
    nxu_u32 aff2;
    nxu_u32 aff3;


    if (target_cpu >=
        nxu_gic.cpu_count)
        return -1;


    if (intid > 15U)
        return -1;


    if (nxu_gic_get_affinity(
            target_cpu,
            &affinity
        ) != 0)
        return -1;


    aff0 =
        (nxu_u32)(
            affinity & 0xFFULL
        );


    aff1 =
        (nxu_u32)(
            (affinity >> 8) &
            0xFFULL
        );


    aff2 =
        (nxu_u32)(
            (affinity >> 16) &
            0xFFULL
        );


    aff3 =
        (nxu_u32)(
            (affinity >> 24) &
            0xFFULL
        );


    /*
     * TargetList contains Aff0 values.
     */
    if (aff0 >= 16U)
        return -1;


    value =
        0ULL;


    value |=
        ((nxu_u64)aff3)
        << 48;


    value |=
        ((nxu_u64)aff2)
        << 32;


    value |=
        ((nxu_u64)(intid & 0xFU))
        << 24;


    value |=
        ((nxu_u64)aff1)
        << 16;


    value |=
        1ULL << aff0;


    asm volatile(
        "msr ICC_SGI1R_EL1, %0"
        :
        : "r"(value)
        : "memory"
    );


    asm volatile(
        "isb"
        ::: "memory"
    );


    return 0;
}
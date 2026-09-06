/*
 * Architecture map
 *
 *   interrupt manager
 *         |
 *         v
 *   gic_backend
 *         |
 *         +-- configure (priority, trigger, route, group)
 *         +-- enable
 *         +-- disable
 *         |
 *         v
 *   GICD / GICR registers
 *
 * Pure hardware backend.
 * No policy decisions.
 */

#include <nxu/interrupt.h>
#include <nxu/interrupt_manager.h>
#include <nxu/interrupt_backend.h>
#include <nxu/mmio.h>
#include <nxu/cpu.h>

#include "gic.h"
#include "gic_reg.h"

static enum nxu_interrupt_type
gic_classify_intid(
    nxu_u32 intid
)
{
    if (intid < 16U)
        return NXU_INTERRUPT_SGI;

    if (intid < 32U)
        return NXU_INTERRUPT_PPI;

    return NXU_INTERRUPT_SPI;
}

/* gicd_priority_address: function. */
static nxu_uptr
gicd_priority_address(
    nxu_u32 intid
)
{
    return nxu_gic.distributor_base +
           GICD_IPRIORITYR +
           ((nxu_uptr)(intid / 4U) * 4U);
}

/* gicd_config_address: function. */
static nxu_uptr
gicd_config_address(
    nxu_u32 intid
)
{
    return nxu_gic.distributor_base +
           GICD_ICFGR +
           ((nxu_uptr)(intid / 16U) * 4U);
}

/* gicd_enable_address: function. */
static nxu_uptr
gicd_enable_address(
    nxu_u32 intid
)
{
    return nxu_gic.distributor_base +
           GICD_ISENABLER +
           ((nxu_uptr)(intid / 32U) * 4U);
}

/* gicd_disable_address: function. */
static nxu_uptr
gicd_disable_address(
    nxu_u32 intid
)
{
    return nxu_gic.distributor_base +
           GICD_ICENABLER +
           ((nxu_uptr)(intid / 32U) * 4U);
}

/* gicd_group_address: function. */
static nxu_uptr
gicd_group_address(
    nxu_u32 intid
)
{
    return nxu_gic.distributor_base +
           GICD_IGROUPR +
           ((nxu_uptr)(intid / 32U) * 4U);
}

static

/* gic_encode_priority: function. */
nxu_u8
gic_encode_priority(
    nxu_u8 nxu_priority
)
{
    nxu_u32 levels;
    nxu_u32 level;
    nxu_u32 inverted;

    if (nxu_gic.priority_bits == 0U)
        return 0U;

    inverted =
        255U - (nxu_u32)nxu_priority;

    if (nxu_gic.priority_bits >= 8U)
        return (nxu_u8)inverted;

    levels =
        1U << nxu_gic.priority_bits;

    level =
        (inverted * (levels - 1U)) / 255U;

    return (nxu_u8)(
        level <<
        (8U - nxu_gic.priority_bits)
    );
}

/* gic_configure_spi_group1: function. */
static int
gic_configure_spi_group1(
    nxu_u32 intid
)
{
    nxu_uptr address;
    nxu_u32 value;
    nxu_u32 bit;

    if (intid < 32U)
        return -1;

    address =
        gicd_group_address(intid);

    bit =
        1U << (intid % 32U);

    value =
        mmio_read32(address);

    value |= bit;

    mmio_write32(
        address,
        value
    );

    return 0;
}

/* gic_configure_spi_priority: function. */
static int
gic_configure_spi_priority(
    nxu_u32 intid,
    nxu_u8 priority
)
{
    nxu_uptr address;
    nxu_u32 value;
    nxu_u32 shift;

    address =
        gicd_priority_address(intid);

    shift =
        (intid % 4U) * 8U;

    value =
        mmio_read32(address);

    value &=
        ~(0xFFU << shift);

    value |=
        ((nxu_u32)priority << shift);

    mmio_write32(
        address,
        value
    );

    return 0;
}

/* gic_configure_spi_trigger: function. */
static int
gic_configure_spi_trigger(
    nxu_u32 intid,
    enum nxu_interrupt_trigger trigger
)
{
    nxu_uptr address;
    nxu_u32 value;
    nxu_u32 shift;
    nxu_u32 field;

    if (intid < 32U)
        return -1;

    address =
        gicd_config_address(intid);

    shift =
        (intid % 16U) * 2U;

    value =
        mmio_read32(address);

    if (trigger == NXU_INTERRUPT_EDGE)
        field = 2U;
    else
        field = 0U;

    value &=
        ~(3U << shift);

    value |=
        field << shift;

    mmio_write32(
        address,
        value
    );

    return 0;
}

/* gic_route_spi: function. */
static int
gic_route_spi(
    nxu_u32 intid,
    nxu_u32 target_cpu
)
{
    nxu_u64 affinity;
    nxu_uptr address;

    if (intid < 32U)
        return -1;

    if (target_cpu >=
        nxu_gic.cpu_count)
        return -1;

    if (nxu_gic_get_affinity(
            target_cpu,
            &affinity
        ) != 0)
        return -1;

    address =
        nxu_gic.distributor_base +
        GICD_IROUTER +
        ((nxu_uptr)(intid - 32U) * 8U);

    affinity &=
        ~(1ULL << 31);

    mmio_write64(
        address,
        affinity
    );

    return 0;
}

/* gic_local_priority: function. */
static int
gic_local_priority(
    nxu_u32 cpu_id,
    nxu_u32 intid,
    nxu_u8 priority
)
{
    nxu_uptr rdist;
    nxu_uptr address;
    nxu_u32 value;
    nxu_u32 shift;

    if (cpu_id >=
        nxu_gic.cpu_count)
        return -1;

    if (intid >= 32U)
        return -1;

    rdist =
        nxu_gic_get_redistributor(
            cpu_id
        );

    if (rdist == 0U)
        return -1;

    address =
        rdist +
        GICR_IPRIORITYR0 +
        ((nxu_uptr)(intid / 4U) * 4U);

    shift =
        (intid % 4U) * 8U;

    value =
        mmio_read32(address);

    value &=
        ~(0xFFU << shift);

    value |=
        ((nxu_u32)priority << shift);

    mmio_write32(
        address,
        value
    );

    return 0;
}

/* gic_local_group1: function. */
static int
gic_local_group1(
    nxu_u32 cpu_id,
    nxu_u32 intid
)
{
    nxu_uptr rdist;
    nxu_u32 value;
    nxu_u32 mask;

    if (cpu_id >=
        nxu_gic.cpu_count)
        return -1;

    if (intid >= 32U)
        return -1;

    rdist =
        nxu_gic_get_redistributor(
            cpu_id
        );

    if (rdist == 0U)
        return -1;

    mask =
        1U << intid;

    value =
        mmio_read32(
            rdist + GICR_IGROUPR0
        );

    if ((value & mask) == 0U)
        return -1;

    return 0;
}

/* gic_local_enable: function. */
static int
gic_local_enable(
    nxu_u32 cpu_id,
    nxu_u32 intid
)
{
    nxu_uptr rdist;
    nxu_u32 mask;
    nxu_u32 value;

    if (cpu_id >=
        nxu_gic.cpu_count)
        return -1;

    if (intid >= 32U)
        return -1;

    rdist =
        nxu_gic_get_redistributor(
            cpu_id
        );

    if (rdist == 0U)
        return -1;

    mask =
        1U << intid;

    mmio_write32(
        rdist + GICR_ISENABLER0,
        mask
    );

    asm volatile(
        "dsb sy"
        ::: "memory"
    );

    value =
        mmio_read32(
            rdist + GICR_ISENABLER0
        );

    if ((value & mask) == 0U)
        return -1;

    return 0;
}

/* gic_local_disable: function. */
static int
gic_local_disable(
    nxu_u32 cpu_id,
    nxu_u32 intid
)
{
    nxu_uptr rdist;
    nxu_u32 mask;
    nxu_u32 value;

    if (cpu_id >=
        nxu_gic.cpu_count)
        return -1;

    if (intid >= 32U)
        return -1;

    rdist =
        nxu_gic_get_redistributor(
            cpu_id
        );

    if (rdist == 0U)
        return -1;

    mask =
        1U << intid;

    mmio_write32(
        rdist + GICR_ICENABLER0,
        mask
    );

    asm volatile(
        "dsb sy"
        ::: "memory"
    );

    value =
        mmio_read32(
            rdist + GICR_ISENABLER0
        );

    if (value & mask)
        return -1;

    return 0;
}

int
nxu_gic_configure_local_ppi(
    nxu_u32 cpu_id,
    nxu_u32 intid,
    nxu_u8 priority
)
{
    if (intid < 16U || intid >= 32U)
        return -1;

    if (gic_local_group1(cpu_id, intid) != 0)
        return -1;

    if (gic_local_priority(cpu_id, intid, priority) != 0)
        return -1;

    if (gic_local_enable(cpu_id, intid) != 0)
        return -1;

    return 0;
}

/* nxu_gic_create_interrupt: function. */
int
nxu_gic_create_interrupt(
    nxu_u32 intid,
    struct nxu_interrupt *interrupt
)
{

    if (interrupt == 0)
        return -1;

    if (intid >=
        nxu_gic.interrupt_count)
        return -1;

    interrupt->intid =
        intid;

    interrupt->type =
        gic_classify_intid(intid);

    if (interrupt->type ==
        NXU_INTERRUPT_SGI) {

        interrupt->trigger =
            NXU_INTERRUPT_EDGE;

    } else {

        interrupt->trigger =
            NXU_INTERRUPT_LEVEL;
    }

    interrupt->priority =
        0U;

    interrupt->target_cpu =
        0U;

    interrupt->state =
        NXU_INTERRUPT_DISABLED;

    interrupt->handler =
        0;

    interrupt->handler_context =
        0;

    return nxu_interrupt_register(
        interrupt
    );
}

/* gic_configure_interrupt: function. */
static int
gic_configure_interrupt(
    struct nxu_interrupt *interrupt,
    const struct nxu_interrupt_config *config
)
{
    nxu_u8 priority;

    if (interrupt == 0 ||
        config == 0)
        return -1;

    if (config->target_cpu >=
        nxu_gic.cpu_count)
        return -1;

    priority =
        gic_encode_priority(
            config->priority
        );

    switch (interrupt->type) {

    case NXU_INTERRUPT_SGI:

        if (config->trigger !=
            NXU_INTERRUPT_EDGE)
            return -1;

        if (gic_local_group1(
                config->target_cpu,
                interrupt->intid
            ) != 0)
            return -1;

        if (gic_local_priority(
                config->target_cpu,
                interrupt->intid,
                priority
            ) != 0)
            return -1;

        return 0;

    case NXU_INTERRUPT_PPI:

        if (gic_local_group1(
                config->target_cpu,
                interrupt->intid
            ) != 0)
            return -1;

        if (gic_local_priority(
                config->target_cpu,
                interrupt->intid,
                priority
            ) != 0)
            return -1;

        return 0;

    case NXU_INTERRUPT_SPI:

        if (gic_configure_spi_group1(
                interrupt->intid
            ) != 0)
            return -1;

        if (gic_configure_spi_priority(
                interrupt->intid,
                priority
            ) != 0)
            return -1;

        if (gic_configure_spi_trigger(
                interrupt->intid,
                config->trigger
            ) != 0)
            return -1;

        if (gic_route_spi(
                interrupt->intid,
                config->target_cpu
            ) != 0)
            return -1;

        return 0;
    }

    return -1;
}

/* gic_enable_interrupt: function. */
static int
gic_enable_interrupt(
    struct nxu_interrupt *interrupt
)
{
    nxu_u32 mask;

    if (interrupt == 0)
        return -1;

    switch (interrupt->type) {

    case NXU_INTERRUPT_SGI:

        return gic_local_enable(
            interrupt->target_cpu,
            interrupt->intid
        );

    case NXU_INTERRUPT_PPI:

        return gic_local_enable(
            interrupt->target_cpu,
            interrupt->intid
        );

    case NXU_INTERRUPT_SPI:

        mask =
            1U <<
            (interrupt->intid % 32U);

        mmio_write32(
            gicd_enable_address(
                interrupt->intid
            ),
            mask
        );

        asm volatile(
            "dsb sy"
            ::: "memory"
        );

        if ((mmio_read32(
                gicd_enable_address(
                    interrupt->intid
                )
            ) & mask) == 0U)
            return -1;

        return 0;
    }

    return -1;
}

/* gic_disable_interrupt: function. */
static int
gic_disable_interrupt(
    struct nxu_interrupt *interrupt
)
{
    nxu_uptr address;
    nxu_u32 mask;
    nxu_u32 value;

    if (interrupt == 0)
        return -1;

    switch (interrupt->type) {

    case NXU_INTERRUPT_SGI:

        return gic_local_disable(
            interrupt->target_cpu,
            interrupt->intid
        );

    case NXU_INTERRUPT_PPI:

        return gic_local_disable(
            interrupt->target_cpu,
            interrupt->intid
        );

    case NXU_INTERRUPT_SPI:

        address =
            gicd_disable_address(
                interrupt->intid
            );

        mask =
            1U <<
            (interrupt->intid % 32U);

        mmio_write32(
            address,
            mask
        );

        asm volatile(
            "dsb sy"
            ::: "memory"
        );

        value =
            mmio_read32(
                gicd_enable_address(
                    interrupt->intid
                )
            );

        if (value & mask)
            return -1;

        return 0;
    }

    return -1;
}

static const struct nxu_interrupt_backend
gic_backend = {

    .configure =
        gic_configure_interrupt,

    .enable =
        gic_enable_interrupt,

    .disable =
        gic_disable_interrupt
};

/* nxu_gic_interrupt_backend_init: function. */
void
/* nxu_gic_interrupt_backend_init: function. */
nxu_gic_interrupt_backend_init(void)
{
    nxu_interrupt_backend_register(
        &gic_backend
    );
}
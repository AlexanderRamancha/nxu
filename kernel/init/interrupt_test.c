#include <nxu/types.h>
#include <nxu/interrupt.h>
#include <nxu/interrupt_manager.h>
#include <nxu/mmio.h>
#include <gic/gic.h>
#include <gic/gic_reg.h>
#include <uart/pl011.h>
#include <nxu/cpu.h>

static volatile nxu_u32 nxu_test_sgi1_seen;
static volatile nxu_u32 nxu_test_spi32_seen;
static volatile nxu_u32 nxu_test_spi33_seen;

static volatile nxu_u32 nxu_test_cpu1_context_ok;
static volatile nxu_u32 nxu_test_cpu0_context_ok;
static volatile nxu_u32 nxu_test_preempt_context_ok;

static volatile nxu_u32 nxu_test_order_index;
static volatile nxu_u32 nxu_test_order[4];

static int
nxu_test_pend_spi(
    nxu_u32 intid
)
{
    nxu_u32 register_index;
    nxu_u32 bit;
    nxu_uptr address;

    if (intid < 32U)
        return -1;

    if (intid >= nxu_gic.interrupt_count)
        return -1;

    register_index =
        intid / 32U;

    bit =
        intid % 32U;

    address =
        nxu_gic.distributor_base +
        GICD_ISPENDR +
        ((nxu_uptr)register_index * 4U);

    mmio_write32(
        address,
        1U << bit
    );

    asm volatile(
        "dsb sy"
        ::: "memory"
    );

    return 0;
}

/* Verify CPU1 owns its interrupt context. */
static void
nxu_test_sgi1_handler(
    struct nxu_interrupt *interrupt
)
{
    (void)interrupt;

    if (
        nxu_cpu_current_id() == 1U &&
        nxu_interrupt_in_context() &&
        nxu_interrupt_nesting_depth() == 1U &&
        nxu_interrupt_current_intid() == 1U
    )
        nxu_test_cpu1_context_ok = 1U;

    nxu_test_sgi1_seen =
        1U;
}

/* Test lower-priority SPI execution and nested context. */
static void
nxu_test_spi32_handler(
    struct nxu_interrupt *interrupt
)
{
    (void)interrupt;

    nxu_test_spi32_seen =
        1U;

    if (
        nxu_cpu_current_id() == 0U &&
        nxu_interrupt_nesting_depth() == 1U &&
        nxu_interrupt_current_intid() == 32U
    )
        nxu_test_cpu0_context_ok = 1U;

    nxu_test_order[
        nxu_test_order_index++
    ] = 32U;

    if (nxu_test_order_index == 1U) {

        if (
            nxu_test_pend_spi(
                33U
            ) != 0
        )
            return;

        asm volatile(
            "dsb sy"
            ::: "memory"
        );

        asm volatile(
            "msr daifclr, #2"
            ::: "memory"
        );

        for (
            volatile nxu_u32 i = 0U;
            i < 1000000U;
            i++
        ) {
            asm volatile(
                "nop"
                ::: "memory"
            );
        }

        asm volatile(
            "msr daifset, #2"
            ::: "memory"
        );
    }

    if (
        nxu_cpu_current_id() == 0U &&
        nxu_interrupt_nesting_depth() == 1U &&
        nxu_interrupt_current_intid() == 32U
    )
        nxu_test_cpu0_context_ok = 1U;

    nxu_test_order[
        nxu_test_order_index++
    ] = 320U;
}

/* Test the nested higher-priority interrupt context. */
static void
nxu_test_spi33_handler(
    struct nxu_interrupt *interrupt
)
{
    (void)interrupt;

    nxu_test_spi33_seen =
        1U;

    if (
        nxu_cpu_current_id() == 0U &&
        nxu_interrupt_nesting_depth() == 2U &&
        nxu_interrupt_current_intid() == 33U &&
        nxu_interrupt_current_priority() == 0xB0U
    )
        nxu_test_preempt_context_ok = 1U;

    nxu_test_order[
        nxu_test_order_index++
    ] = 33U;

    nxu_test_order[
        nxu_test_order_index++
    ] = 330U;
}

/* Run the interrupt and CPU-local context regression. */
int
nxu_interrupt_final_test(void)
{
    struct nxu_interrupt spi32;
    struct nxu_interrupt spi33;
    struct nxu_interrupt sgi1;

    struct nxu_interrupt_config spi32_config;
    struct nxu_interrupt_config spi33_config;
    struct nxu_interrupt_config sgi_config;

    nxu_test_sgi1_seen = 0U;
    nxu_test_spi32_seen = 0U;
    nxu_test_spi33_seen = 0U;

    nxu_test_cpu1_context_ok = 0U;
    nxu_test_cpu0_context_ok = 0U;
    nxu_test_preempt_context_ok = 0U;

    nxu_test_order_index = 0U;

    nxu_test_order[0] = 0U;
    nxu_test_order[1] = 0U;
    nxu_test_order[2] = 0U;
    nxu_test_order[3] = 0U;

    if (
        nxu_gic_create_interrupt(
            32U,
            &spi32
        ) != 0
    )
        return -1;

    if (
        nxu_interrupt_set_handler(
            &spi32,
            nxu_test_spi32_handler,
            0
        ) != 0
    )
        return -1;

    spi32_config.trigger =
        NXU_INTERRUPT_LEVEL;

    spi32_config.priority =
        0x20U;

    spi32_config.target_cpu =
        0U;

    if (
        nxu_interrupt_configure(
            &spi32,
            &spi32_config
        ) != 0
    )
        return -1;

    if (
        nxu_interrupt_enable(
            &spi32
        ) != 0
    )
        return -1;

    if (
        nxu_gic_create_interrupt(
            33U,
            &spi33
        ) != 0
    )
        return -1;

    if (
        nxu_interrupt_set_handler(
            &spi33,
            nxu_test_spi33_handler,
            0
        ) != 0
    )
        return -1;

    spi33_config.trigger =
        NXU_INTERRUPT_LEVEL;

    spi33_config.priority =
        0xB0U;

    spi33_config.target_cpu =
        0U;

    if (
        nxu_interrupt_configure(
            &spi33,
            &spi33_config
        ) != 0
    )
        return -1;

    if (
        nxu_interrupt_enable(
            &spi33
        ) != 0
    )
        return -1;

    if (
        nxu_gic_create_interrupt(
            1U,
            &sgi1
        ) != 0
    )
        return -1;

    if (
        nxu_interrupt_set_handler(
            &sgi1,
            nxu_test_sgi1_handler,
            0
        ) != 0
    )
        return -1;

    sgi_config.trigger =
        NXU_INTERRUPT_EDGE;

    sgi_config.priority =
        0x80U;

    sgi_config.target_cpu =
        1U;

    if (
        nxu_interrupt_configure(
            &sgi1,
            &sgi_config
        ) != 0
    )
        return -1;

    if (
        nxu_interrupt_enable(
            &sgi1
        ) != 0
    )
        return -1;

    if (
        nxu_test_pend_spi(
            32U
        ) != 0
    )
        return -1;

    if (
        nxu_gic_send_sgi(
            1U,
            1U
        ) != 0
    )
        return -1;

    asm volatile(
        "msr daifclr, #2"
        ::: "memory"
    );

    asm volatile(
        "dsb sy"
        ::: "memory"
    );

    if (!nxu_test_spi32_seen)
        return -1;

    if (!nxu_test_spi33_seen)
        return -1;

    if (!nxu_test_sgi1_seen)
        return -1;

    if (!nxu_test_cpu0_context_ok)
        return -1;

    if (!nxu_test_preempt_context_ok)
        return -1;

    if (!nxu_test_cpu1_context_ok)
        return -1;

    if (nxu_test_order_index != 4U)
        return -1;

    if (nxu_test_order[0] != 32U)
        return -1;

    if (nxu_test_order[1] != 33U)
        return -1;

    if (nxu_test_order[2] != 330U)
        return -1;

    if (nxu_test_order[3] != 320U)
        return -1;

    uart_puts(
        "CPU-local interrupt context: PASS\r\n"
    );

    uart_puts(
        "Priority preemption: SPI32 -> SPI33 -> SPI32\r\n"
    );

    return 0;
}
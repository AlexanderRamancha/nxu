#include <nxu/timer.h>
#include <nxu/interrupt_manager.h>
#include <nxu/interrupt.h>
#include <gic/gic.h>
#include <uart/pl011.h>

#define NXU_TIMER_PPI 30U
#define NXU_TIMER_DEMO_PERIOD_US 1000000U

static struct nxu_interrupt nxu_timer_interrupt;
static nxu_u64 nxu_timer_freq;

static nxu_u64
nxu_read_counter(void)
{
    nxu_u64 value;

    asm volatile(
        "mrs %0, cntpct_el0"
        : "=r"(value)
        :
        : "memory"
    );

    return value;
}

static nxu_u64
nxu_read_frequency(void)
{
    nxu_u64 value;

    asm volatile(
        "mrs %0, cntfrq_el0"
        : "=r"(value)
        :
        : "memory"
    );

    return value;
}

static void
nxu_write_compare(
    nxu_u64 deadline
)
{
    asm volatile(
        "msr cntp_cval_el0, %0"
        :
        : "r"(deadline)
        : "memory"
    );
}

static void
nxu_timer_enable_hw(void)
{
    nxu_u64 control;

    control = 1U;

    asm volatile(
        "msr cntp_ctl_el0, %0"
        :
        : "r"(control)
        : "memory"
    );

    asm volatile(
        "isb"
        ::: "memory"
    );
}

static void
nxu_timer_disable_hw(void)
{
    nxu_u64 control;

    control = 0U;

    asm volatile(
        "msr cntp_ctl_el0, %0"
        :
        : "r"(control)
        : "memory"
    );

    asm volatile(
        "isb"
        ::: "memory"
    );
}

nxu_u64
nxu_timer_now(void)
{
    return nxu_read_counter();
}

nxu_u64
nxu_timer_frequency(void)
{
    return nxu_timer_freq;
}

int
nxu_timer_arm_ticks(
    nxu_u64 ticks
)
{
    nxu_u64 deadline;

    if (ticks == 0U)
        return -1;

    deadline =
        nxu_timer_now() + ticks;

    nxu_write_compare(deadline);
    nxu_timer_enable_hw();

    return 0;
}

int
nxu_timer_arm_us(
    nxu_u64 microseconds
)
{
    nxu_u64 ticks;

    if (microseconds == 0U ||
        nxu_timer_freq == 0U)
        return -1;

    ticks =
        (nxu_timer_freq / 1000000U) *
        microseconds;

    if (ticks == 0U)
        ticks = 1U;

    return nxu_timer_arm_ticks(ticks);
}

void
nxu_timer_stop(void)
{
    nxu_timer_disable_hw();
}

static void
nxu_timer_interrupt_handler(
    struct nxu_interrupt *interrupt
)
{
    (void)interrupt;

    uart_puts(
        "NXU timer: tick\r\n"
    );

    (void)nxu_timer_arm_us(
        NXU_TIMER_DEMO_PERIOD_US
    );
}

int
nxu_timer_init(void)
{
    struct nxu_interrupt_config config;

    nxu_timer_freq =
        nxu_read_frequency();

    if (nxu_timer_freq == 0U)
        return -1;

    if (nxu_gic_create_interrupt(
            NXU_TIMER_PPI,
            &nxu_timer_interrupt
        ) != 0)
        return -1;

    if (nxu_interrupt_set_handler(
            &nxu_timer_interrupt,
            nxu_timer_interrupt_handler,
            0
        ) != 0)
        return -1;

    config.trigger =
        NXU_INTERRUPT_LEVEL;

    config.priority =
        0x40U;

    config.target_cpu =
        0U;

    if (nxu_interrupt_configure(
            &nxu_timer_interrupt,
            &config
        ) != 0)
        return -1;

    if (nxu_interrupt_enable(
            &nxu_timer_interrupt
        ) != 0)
        return -1;

    return nxu_timer_arm_us(
        NXU_TIMER_DEMO_PERIOD_US
    );
}

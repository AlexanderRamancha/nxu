#include <nxu/types.h>
#include <nxu/cpu.h>
#include <nxu/timer.h>
#include <gic/gic.h>
#include <nxu/interrupt_manager.h>
#include <nxu/interrupt_backend.h>

#include <platform/platform.h>

#include <uart/pl011.h>


#ifdef NXU_RUN_INTERRUPT_TEST

extern int
nxu_interrupt_final_test(void);

#endif


static void
nxu_kernel_halt(void)
{
    for (;;) {

        asm volatile(
            "wfi"
            ::: "memory"
        );
    }
}


void
nxu_secondary_cpu_main(
    nxu_u64 cpu_id
)
{
    nxu_u32 logical_id;


    logical_id =
        (nxu_u32)cpu_id;


    if (nxu_gic_cpu_init(
            logical_id
        ) != 0) {

        nxu_kernel_halt();
    }


    asm volatile(
        "msr daifclr, #2"
        ::: "memory"
    );


    for (;;) {

        asm volatile(
            "wfi"
            ::: "memory"
        );
    }
}


void
kernel_main(void)
{
    int result;


    /*
     * CPU subsystem.
     */
    result =
        nxu_cpu_init();

    if (result != 0)
        nxu_kernel_halt();


    /*
     * Platform.
     */
    result =
        nxu_platform_init();

    if (result != 0)
        nxu_kernel_halt();


    /*
     * GIC discovery.
     */
    result =
        nxu_gic_discover();

    if (result != 0)
        nxu_kernel_halt();


    /*
     * GIC initialization.
     */
    result =
        nxu_gic_init();

    if (result != 0)
        nxu_kernel_halt();


    /*
     * Interrupt manager.
     */
    nxu_interrupt_manager_init(
        nxu_gic.interrupt_count
    );


    /*
     * Architecture-specific interrupt backend.
     */
    nxu_gic_interrupt_backend_init();


    /*
     * Generic Timer.
     *
     * The timer uses the ARM Generic Timer PPI, so the
     * interrupt subsystem must already be ready.
     */
    result =
        nxu_timer_init();

    if (result != 0)
        nxu_kernel_halt();


    /*
     * Start CPU1.
     */
    result =
        nxu_cpu_start(
            1U
        );

    if (result != 0)
        nxu_kernel_halt();


#ifdef NXU_RUN_INTERRUPT_TEST

    /*
     * Development / hardware bring-up regression test.
     *
     * The test result is visible through the
     * platform UART.
     */
    result =
        nxu_interrupt_final_test();

    if (result != 0) {

        uart_puts(
            "NXU interrupt regression test: FAIL\r\n"
        );

        nxu_kernel_halt();
    }


    uart_puts(
        "NXU interrupt regression test: PASS\r\n"
    );

#endif


    /*
     * Normal kernel idle.
     */
    for (;;) {

        asm volatile(
            "wfi"
            ::: "memory"
        );
    }
}
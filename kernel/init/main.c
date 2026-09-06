#include <nxu/types.h>
#include <nxu/boot.h>
#include <nxu/cpu.h>
#include <nxu/interrupt_manager.h>
#include <nxu/interrupt_backend.h>
#include <nxu/timer.h>
#include <nxu/console.h>
#include <gic/gic.h>
#include <platform/platform.h>
#include <nxu/log.h>

struct nxu_boot_info nxu_boot_info;

#ifdef NXU_RUN_INTERRUPT_TEST
extern int nxu_interrupt_final_test(void);
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
nxu_secondary_cpu_main(nxu_u64 cpu_id)
{
    nxu_u32 logical_id;

    logical_id = (nxu_u32)cpu_id;

    if (nxu_gic_cpu_init(logical_id) != 0)
    nxu_kernel_halt();

nxu_cpu_online(logical_id);

asm volatile(
    "msr daifclr, #2"
    ::: "memory"
);
}

void
nxu_kernel_main(struct nxu_boot_info *boot_info)
{
    int result;


    if (boot_info == 0)
        nxu_kernel_halt();


    /*
     * Console must come first so early failures are visible.
     */
    nxu_console_init();

    nxu_console_puts(
        "NXU boot starting...\r\n"
    );


    /*
     * ------------------------------------------------------------------------
     * EARLY PLATFORM MEMORY
     * ------------------------------------------------------------------------
     *
     * Establish the physical memory ranges that are safe to access during
     * early boot.
     *
     * This must happen BEFORE DTB validation.
     */
    result =
        nxu_platform_memory_init();

    if (result != 0) {
        nxu_console_puts(
            "[MEMORY] early memory init failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_console_puts(
        "[MEMORY] early memory map ready\r\n"
    );


    /*
     * ------------------------------------------------------------------------
     * BOOT CONTRACT VALIDATION
     * ------------------------------------------------------------------------
     */
    if (
        nxu_boot_validate(
            boot_info
        ) != 0
    ) {
        nxu_console_puts(
            "[BOOT] validation failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_console_puts(
        "[BOOT] validation... ok\r\n"
    );


    /*
     * Report validated boot information.
     */
    nxu_console_puts(
        "[BOOT] boot CPU affinity = 0x"
    );

    nxu_console_puthex(
        boot_info->boot_cpu_affinity
    );

    nxu_console_puts(
        "\r\n"
    );


    nxu_console_puts(
        "[BOOT] DTB address = 0x"
    );

    nxu_console_puthex(
        (nxu_u64)boot_info->dtb_address
    );

    nxu_console_puts(
        "\r\n"
    );


    /*
     * ------------------------------------------------------------------------
     * Existing platform initialization
     * ------------------------------------------------------------------------
     */

    result =
        nxu_platform_init();

    if (result != 0) {
        nxu_console_puts(
            "platform init failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_console_puts(
        "platform init... ok\r\n"
    );


    /*
     * ------------------------------------------------------------------------
     * Everything below this point remains your existing code.
     * ------------------------------------------------------------------------
     */

    /*
     * GIC discovery
     */
    result =
        nxu_gic_discover();

    if (result != 0) {
        nxu_console_puts(
            "gic discover failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_console_puts(
        "gic discover... ok\r\n"
    );


    /*
     * CPU topology
     */
    result =
        nxu_cpu_init(
            boot_info->boot_cpu_affinity
        );

    if (result != 0) {
        nxu_console_puts(
            "cpu init failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_log_init();

    nxu_console_puts(
        "cpu init... ok\r\n"
    );

    /*
     * GIC initialization
     */
    result =
        nxu_gic_init(
            nxu_cpu_current_id()
        );

    if (result != 0) {
        nxu_console_puts(
            "gic init failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_console_puts(
        "gic init... ok\r\n"
    );


    /*
     * Interrupt manager
     */
    nxu_interrupt_manager_init(
        nxu_gic.interrupt_count
    );

    nxu_console_puts(
        "interrupt manager... ok\r\n"
    );


    /*
     * Interrupt backend
     */
    nxu_gic_interrupt_backend_init();

    nxu_console_puts(
        "interrupt backend... ok\r\n"
    );


    /*
     * Timer
     */
    result =
        nxu_timer_init();

    if (result != 0) {
        nxu_console_puts(
            "timer init failed\r\n"
        );

        nxu_kernel_halt();
    }

    nxu_console_puts(
        "timer init... ok\r\n"
    );

    /* Start CPU 1 */
	result = nxu_cpu_start(1U);

	if (result != 0) {
		nxu_console_puts("cpu1 start failed\r\n");
		nxu_kernel_halt();
	}

	nxu_console_puts("cpu1 start... ok\r\n");

	/* -------------------------------------------------- */
/* Minimal Timer Test                                 */
/* -------------------------------------------------- */

    nxu_console_puts(
        "timer test starting...\r\n"
    );

    asm volatile(
        "msr daifclr, #2"
        ::: "memory"
    );

    nxu_console_puts(
        "waiting for timer...\r\n"
    );

{
    nxu_u64 start;
    nxu_u64 now;
    nxu_u64 frequency;

    start = nxu_timer_now();
    frequency = nxu_timer_frequency();

    while (1) {
        now = nxu_timer_now();

        if (now - start >= frequency + (frequency / 2U))
            break;
    }
}


nxu_timer_stop();

nxu_console_puts(
    "timer test complete\r\n"
);

/* -------------------------------------------------- */

#ifdef NXU_RUN_INTERRUPT_TEST
	result = nxu_interrupt_final_test();
	if (result != 0) {
		nxu_console_puts("NXU interrupt regression test: FAIL\r\n");
		nxu_kernel_halt();
	}
	nxu_console_puts("NXU interrupt regression test: PASS\r\n");
#endif

	nxu_console_puts("NXU idle\r\n");

	for (;;) {
		asm volatile("wfi" ::: "memory");
	}
}

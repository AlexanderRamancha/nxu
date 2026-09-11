#include <nxu/interrupt_manager.h>

/*
 * ARM64 exception boundary
 *
 * The exception layer only transfers control to the NXU interrupt
 * manager. It has no knowledge of GIC operations or interrupt
 * objects.
 */

void
nxu_irq_handler(void)
{
    /*
     * The manager owns the complete interrupt operation:
     * identify -> validate -> dispatch -> complete.
     */
    for (;;) {
        if (nxu_interrupt_handle() != 0)
            break;
    }
}

void
handle_sync_exception(void)
{
    nxu_u64 esr;
    nxu_u64 elr;
    nxu_u64 far;

    asm volatile(
        "mrs %0, esr_el1"
        : "=r"(esr)
        :
        : "memory"
    );

    asm volatile(
        "mrs %0, elr_el1"
        : "=r"(elr)
        :
        : "memory"
    );

    asm volatile(
        "mrs %0, far_el1"
        : "=r"(far)
        :
        : "memory"
    );

    (void)esr;
    (void)elr;
    (void)far;

    for (;;) {
        asm volatile("wfi" ::: "memory");
    }
}

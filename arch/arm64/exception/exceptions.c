#include <nxu/interrupt_manager.h>
#include <gic/gic.h>

/*
 * Architecture map
 *
 *     ARM64 IRQ vector
 *            |
 *            v
 *     nxu_irq_handler()
 *            |
 *            v
 *     ICC_IAR1_EL1
 *            |
 *            v
 *     interrupt manager
 *            |
 *            v
 *        handler
 *            |
 *            v
 *     ICC_EOIR1_EL1
 *            |
 *            v
 *       acknowledge next
 *
 * The exception layer does not contain interrupt policy.
 */

#define NXU_GIC_SPECIAL_INTID 1020U


void
nxu_irq_handler(void)
{
    nxu_u32 intid;

    for (;;) {

        intid =
            nxu_gic_acknowledge_interrupt();

        if (intid >= NXU_GIC_SPECIAL_INTID)
            break;

        nxu_interrupt_dispatch(intid);

        nxu_gic_end_interrupt(intid);
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
        asm volatile(
            "wfi"
            ::: "memory"
        );
    }
}
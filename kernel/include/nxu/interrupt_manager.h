#ifndef NXU_INTERRUPT_MANAGER_H
#define NXU_INTERRUPT_MANAGER_H

#include <nxu/interrupt.h>

/*
 * Architecture map
 *
 *   subsystem
 *       |
 *       v
 *   opaque IRQ reference
 *       |
 *       v
 *   interrupt manager
 *       |
 *       +-- private IRQ object
 *       +-- relationship / lifecycle
 *       +-- dispatch
 *       |
 *       v
 *   interrupt backend
 *       |
 *       v
 *   hardware controller
 *
 * The manager is the single NXU interrupt authority boundary.
 * Subsystems do not access interrupt objects or hardware state.
 */

void nxu_interrupt_manager_init(nxu_u32 interrupt_count);

int nxu_interrupt_create(
    nxu_u32 intid,
    enum nxu_interrupt_type type,
    nxu_interrupt_handler handler,
    void *context,
    struct nxu_interrupt **out
);

int nxu_interrupt_configure(
    struct nxu_interrupt *interrupt,
    const struct nxu_interrupt_config *config
);

int nxu_interrupt_enable(struct nxu_interrupt *interrupt);
int nxu_interrupt_disable(struct nxu_interrupt *interrupt);

/*
 * Complete interrupt operation. The manager obtains the hardware
 * event through the backend, validates the private relationship,
 * dispatches the handler, and asks the backend to complete hardware.
 *
 * Return:
 *   0  = normal event handled or safely consumed
 *   1  = no normal NXU event (controller special/spurious value)
 *  -1  = manager/backend fault
 */
int nxu_interrupt_handle(void);

nxu_u32 nxu_interrupt_nesting_depth(void);
nxu_u32 nxu_interrupt_current_intid(void);
nxu_u8  nxu_interrupt_current_priority(void);
int     nxu_interrupt_in_context(void);

#endif

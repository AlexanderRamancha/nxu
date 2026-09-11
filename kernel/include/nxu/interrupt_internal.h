#ifndef NXU_INTERRUPT_INTERNAL_H
#define NXU_INTERRUPT_INTERNAL_H

#include <nxu/interrupt.h>

/*
 * Private interrupt-manager representation.
 *
 * Only the interrupt manager and trusted hardware backends should
 * include this header. Device subsystems must use the opaque public
 * interface instead.
 */

struct nxu_interrupt {
    nxu_u32 intid;
    enum nxu_interrupt_type type;
    enum nxu_interrupt_trigger trigger;
    nxu_u8 priority;
    nxu_u32 target_cpu;
    enum nxu_interrupt_state state;

    nxu_interrupt_handler handler;
    void *handler_context;

    nxu_u8 allocated;
};

#endif

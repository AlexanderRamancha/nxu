#ifndef NXU_INTERRUPT_H
#define NXU_INTERRUPT_H

#include <nxu/types.h>

#define NXU_MAX_INTERRUPT_NESTING 16U

/*
 * Architecture map
 *
 *   GIC
 *     |
 *     | INTID
 *     v
 *   exception entry
 *     |
 *     v
 *   interrupt manager
 *     |
 *     +-- interrupt object
 *     |     +-- INTID
 *     |     +-- type
 *     |     +-- configuration
 *     |     +-- handler
 *     |
 *     +-- CPU-local nesting context
 *
 * GIC owns hardware pending/active state.
 * Exception layer owns register context.
 * Manager owns software objects and nesting.
 */

enum nxu_interrupt_type {
    NXU_INTERRUPT_SGI = 0,
    NXU_INTERRUPT_PPI = 1,
    NXU_INTERRUPT_SPI = 2
};

enum nxu_interrupt_trigger {
    NXU_INTERRUPT_LEVEL = 0,
    NXU_INTERRUPT_EDGE  = 1
};

enum nxu_interrupt_state {
    NXU_INTERRUPT_DISABLED = 0,
    NXU_INTERRUPT_ENABLED  = 1
};

struct nxu_interrupt_config {
    nxu_u8 priority;
    enum nxu_interrupt_trigger trigger;
    nxu_u32 target_cpu;
};

struct nxu_interrupt {
    nxu_u32 intid;
    enum nxu_interrupt_type type;
    enum nxu_interrupt_trigger trigger;
    nxu_u8 priority;
    nxu_u32 target_cpu;
    enum nxu_interrupt_state state;
    void (*handler)(struct nxu_interrupt *interrupt);
    void *handler_context;
};

#endif
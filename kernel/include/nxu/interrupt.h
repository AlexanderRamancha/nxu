#ifndef NXU_INTERRUPT_H
#define NXU_INTERRUPT_H

#include <nxu/types.h>

#define NXU_MAX_INTERRUPT_NESTING 16U

/*
 * Architecture map
 *
 * GIC
 *  |
 *  | INTID
 *  v
 * ARM64 exception entry
 *  |
 *  v
 * NXU IRQ handler
 *  |
 *  v
 * Interrupt manager
 *  |
 *  +-- interrupt object
 *  |     +-- INTID
 *  |     +-- type
 *  |     +-- configuration
 *  |     +-- handler
 *  |
 *  +-- current CPU
 *        |
 *        v
 *   CPU-local interrupt context
 *        |
 *        +-- nesting depth
 *        +-- interrupt frames
 *
 * GIC owns hardware pending/active state.
 * ARM64 owns exception register context.
 * CPU-local NXU state owns software interrupt nesting.
 */

enum nxu_interrupt_type {
    NXU_INTERRUPT_SGI = 0,
    NXU_INTERRUPT_PPI = 1,
    NXU_INTERRUPT_SPI = 2
};

enum nxu_interrupt_trigger {
    NXU_INTERRUPT_LEVEL = 0,
    NXU_INTERRUPT_EDGE = 1
};

enum nxu_interrupt_state {
    NXU_INTERRUPT_DISABLED = 0,
    NXU_INTERRUPT_ENABLED = 1
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

    void (*handler)(
        struct nxu_interrupt *interrupt
    );

    void *handler_context;
};

struct nxu_ppi {
    struct nxu_interrupt base;
    nxu_u32 cpu;
};

struct nxu_spi {
    struct nxu_interrupt base;
    nxu_u32 target_cpu;
};

struct nxu_sgi {
    struct nxu_interrupt base;
};

int
nxu_interrupt_set_handler(
    struct nxu_interrupt *interrupt,
    void (*handler)(
        struct nxu_interrupt *interrupt
    ),
    void *context
);

int
nxu_interrupt_configure(
    struct nxu_interrupt *interrupt,
    const struct nxu_interrupt_config *config
);

#endif
#ifndef NXU_INTERRUPT_H
#define NXU_INTERRUPT_H

#include <nxu/types.h>

#define NXU_MAX_INTERRUPT_NESTING 16U

/*
 * NXU interrupt boundary
 *
 * Interrupts are hardware events, not subsystem authority.
 * The interrupt manager owns interrupt objects and their
 * relationships. Subsystems receive only an opaque reference
 * and handler context; they do not see the internal object.
 *
 * Hardware-controller details stay behind the interrupt backend.
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

/* The manager owns the actual interrupt object. */
struct nxu_interrupt;

struct nxu_interrupt_config {
    nxu_u8 priority;
    enum nxu_interrupt_trigger trigger;
    nxu_u32 target_cpu;
};

typedef void (*nxu_interrupt_handler)(void *context);

#endif

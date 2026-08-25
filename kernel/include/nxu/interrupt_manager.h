#ifndef NXU_INTERRUPT_MANAGER_H
#define NXU_INTERRUPT_MANAGER_H

#include <nxu/interrupt.h>

void
nxu_interrupt_manager_init(
    nxu_u32 interrupt_count
);

int
nxu_interrupt_register(
    struct nxu_interrupt *interrupt
);

struct nxu_interrupt *
nxu_interrupt_lookup(
    nxu_u32 intid
);

int
nxu_interrupt_configure(
    struct nxu_interrupt *interrupt,
    const struct nxu_interrupt_config *config
);

int
nxu_interrupt_enable(
    struct nxu_interrupt *interrupt
);

int
nxu_interrupt_disable(
    struct nxu_interrupt *interrupt
);

int
nxu_interrupt_set_handler(
    struct nxu_interrupt *interrupt,
    void (*handler)(
        struct nxu_interrupt *interrupt
    ),
    void *context
);

int
nxu_interrupt_dispatch(
    nxu_u32 intid
);

nxu_u32
nxu_interrupt_nesting_depth(void);

nxu_u32
nxu_interrupt_current_intid(void);

nxu_u8
nxu_interrupt_current_priority(void);

int
nxu_interrupt_in_context(void);

#endif
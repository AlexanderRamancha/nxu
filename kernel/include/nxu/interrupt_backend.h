#ifndef NXU_INTERRUPT_BACKEND_H
#define NXU_INTERRUPT_BACKEND_H

#include <nxu/interrupt.h>

struct nxu_interrupt_backend {
    int (*configure)(
        struct nxu_interrupt *interrupt,
        const struct nxu_interrupt_config *config
    );

    int (*enable)(
        struct nxu_interrupt *interrupt
    );

    int (*disable)(
        struct nxu_interrupt *interrupt
    );
};

void nxu_interrupt_backend_register(
    const struct nxu_interrupt_backend *backend
);

const struct nxu_interrupt_backend *
nxu_interrupt_backend_get(void);

#endif
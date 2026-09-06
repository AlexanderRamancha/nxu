#ifndef NXU_INTERRUPT_BACKEND_H
#define NXU_INTERRUPT_BACKEND_H

#include <nxu/interrupt.h>

/*
 * Architecture map
 *
 *   interrupt manager
 *         |
 *         v
 *   backend (GIC or future controller)
 *         |
 *         v
 *   hardware registers
 *
 * Backend performs only hardware operations.
 * Policy stays in the manager.
 */

struct nxu_interrupt_backend {
    int (*configure)(struct nxu_interrupt *interrupt,
                     const struct nxu_interrupt_config *config);
    int (*enable)(struct nxu_interrupt *interrupt);
    int (*disable)(struct nxu_interrupt *interrupt);
};

void nxu_interrupt_backend_register(const struct nxu_interrupt_backend *backend);
const struct nxu_interrupt_backend *nxu_interrupt_backend_get(void);

#endif
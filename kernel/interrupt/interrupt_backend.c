/*
 * Architecture map
 *
 *   register(backend)
 *         |
 *         v
 *   single active backend pointer
 *
 *   get()
 *         |
 *         v
 *   return active backend
 *
 * Thin indirection only.
 */

#include <nxu/interrupt_backend.h>

static const struct nxu_interrupt_backend *active_backend;

void
nxu_interrupt_backend_register(const struct nxu_interrupt_backend *backend)
{
    active_backend = backend;
}

const struct nxu_interrupt_backend *
nxu_interrupt_backend_get(void)
{
    return active_backend;
}
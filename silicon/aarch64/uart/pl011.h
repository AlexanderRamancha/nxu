/*
 * NXU MAP
 * Layer: silicon UART (ops API)
 * Owns: PL011 prototypes
 * Consumes: base as parameter
 * Produces: op prototypes
 * Hardware: PL011
 * Forbidden: board UART address
 * Invariant: operations only
 */
#ifndef NXU_PL011_H
#define NXU_PL011_H

#include <nxu/types.h>

void nxu_pl011_putc(nxu_u64 base, char c);
void nxu_pl011_write(nxu_u64 base, const char *s);

#endif

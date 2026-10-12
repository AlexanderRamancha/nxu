#ifndef NXU_EXCEPTION_H
#define NXU_EXCEPTION_H

/*
 * NXU MAP
 * Layer: foundation exception
 * Owns: S7 VBAR install; C entry from silicon vectors; fault capture
 * Consumes: bound nxu_cpu_exec_context, GIC IAR/EOI ops
 * Produces: ESR/FAR in context on sync; INTID in context on IRQ
 * Hardware: VBAR, ESR, FAR (via silicon)
 * Forbidden: continue after unexpected class; handler function pointers
 * Invariant: VBAR is NXU's before any unmask; unexpected class → halt
 */

#include <nxu/types.h>

void nxu_s7_exception(void);
void nxu_exception(nxu_u32 kind);
void nxu_irq_c(void);

#endif

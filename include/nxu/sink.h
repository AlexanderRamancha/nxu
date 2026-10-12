#ifndef NXU_SINK_H
#define NXU_SINK_H

/*
 * NXU MAP
 * Layer: foundation observe
 * Owns: optional UART bind and character write
 * Consumes: UART PA after ownership
 * Produces: bytes on PL011
 * Hardware: PL011 ops (address as parameter)
 * Forbidden: UART as architecture; print before ownership
 * Invariant: mapping is TCB; printing is not architecture
 */

#include <nxu/types.h>

extern nxu_u32 nxu_sink_ready;
extern nxu_u64 nxu_sink_base;

void nxu_sink_bind(nxu_u64 uart_pa);
void nxu_sink_write(const char *s);
void nxu_sink_hex(nxu_u64 v, nxu_u32 nibbles);
void nxu_sink_nl(void);

#endif

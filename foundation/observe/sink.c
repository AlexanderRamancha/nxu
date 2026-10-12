/*
 * NXU MAP
 *
 *                    FOUNDATION OBSERVE
 *                           │
 *                           ▼
 *                      UART SINK
 *                           │
 *                           ▼
 *                    SILICON PL011
 *
 * Layer: foundation observe
 * Owns: bind of accepted UART PA; byte helpers
 * Consumes: UART PA from facts after S6 map
 * Produces: characters
 * Hardware: PL011 (ops only)
 * Forbidden: board UART address; earlycon as architecture
 * Invariant: no bind → writes are silent
 */

#include <nxu/sink.h>
#include <silicon/aarch64/uart/pl011.h>

nxu_u32 nxu_sink_ready;
nxu_u64 nxu_sink_base;

void
nxu_sink_bind(nxu_u64 uart_pa)
{
	nxu_sink_base = uart_pa;
	nxu_sink_ready = 1U;
}

void
nxu_sink_write(const char *s)
{
	if (!nxu_sink_ready || s == NXU_NULL)
		return;
	nxu_pl011_write(nxu_sink_base, s);
}

void
nxu_sink_hex(nxu_u64 v, nxu_u32 nibbles)
{
	const char *d = "0123456789abcdef";
	char buf[17];
	nxu_u32 i;

	if (!nxu_sink_ready)
		return;
	if (nibbles == 0U || nibbles > 16U)
		nibbles = 16U;
	for (i = nibbles; i > 0U; i--)
		buf[nibbles - i] = d[(v >> ((i - 1U) * 4U)) & 0xFULL];
	buf[nibbles] = 0;
	nxu_pl011_write(nxu_sink_base, buf);
}

void
nxu_sink_nl(void)
{
	nxu_sink_write("\n");
}

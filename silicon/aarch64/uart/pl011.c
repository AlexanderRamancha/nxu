/*
 * NXU MAP
 * Layer: silicon UART
 * Owns: PL011 putc/write
 * Consumes: base address as parameter
 * Produces: characters on the device
 * Hardware: PL011
 * Forbidden: knowing QEMU/FVP UART addresses
 * Invariant: operations only
 */
#include <silicon/aarch64/uart/pl011.h>

static nxu_u32
pl011_r(nxu_u64 base, nxu_u32 off)
{
	return *(volatile nxu_u32 *)(nxu_uptr)(base + (nxu_u64)off);
}

static void
pl011_w(nxu_u64 base, nxu_u32 off, nxu_u32 v)
{
	*(volatile nxu_u32 *)(nxu_uptr)(base + (nxu_u64)off) = v;
}

void
nxu_pl011_putc(nxu_u64 base, char c)
{
	nxu_u32 timeout;

	if ((pl011_r(base, 0x30U) & 1U) == 0U)
		pl011_w(base, 0x30U, 0x301U);
	timeout = 1000000U;
	while ((pl011_r(base, 0x18U) & (1U << 5)) != 0U) {
		if (--timeout == 0U)
			return;
	}
	pl011_w(base, 0U, (nxu_u32)(nxu_u8)c);
}

void
nxu_pl011_write(nxu_u64 base, const char *s)
{
	while (*s != 0) {
		if (*s == '\n')
			nxu_pl011_putc(base, '\r');
		nxu_pl011_putc(base, *s);
		s++;
	}
}

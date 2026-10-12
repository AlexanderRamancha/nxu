/*
 * NXU MAP
 * Layer: foundation contract (S2)
 * Owns: ARM64 image-header structural check
 * Consumes: nxu_image.base (PC-relative footprint)
 * Produces: accept or halt
 * Hardware: none
 * Forbidden: treating the Linux header as a trust model
 * Invariant: bad magic / flags / size → halt
 */
#include <nxu/s0.h>
#include <nxu/intake.h>
#include <nxu/image.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>

void
nxu_s2_contract(void)
{
	const nxu_u8 *hdr;
	nxu_u32 magic;
	nxu_u64 flags;
	nxu_u64 image_size;

	hdr = (const nxu_u8 *)(nxu_uptr)nxu_image.base;
	magic = (nxu_u32)hdr[56] | ((nxu_u32)hdr[57] << 8) |
	    ((nxu_u32)hdr[58] << 16) | ((nxu_u32)hdr[59] << 24);
	if (magic != 0x644d5241U)
		nxu_fail(NXU_FAIL_BAD_HEADER, magic, 0U);

	flags = 0U;
	image_size = 0U;
	{
		nxu_u32 i;
		for (i = 0U; i < 8U; i++) {
			flags |= (nxu_u64)hdr[24 + i] << (i * 8U);
			image_size |= (nxu_u64)hdr[16 + i] << (i * 8U);
		}
	}
	if ((flags & 0xAULL) != 0xAULL)
		nxu_fail(NXU_FAIL_BAD_HEADER, flags, 1U);
	if (image_size < 64U)
		nxu_fail(NXU_FAIL_BAD_HEADER, image_size, 2U);

	nxu_chronicle_put(NXU_REC_S2_HEADER, magic, image_size);
	nxu_chronicle_put(NXU_REC_S2_EL, nxu_control_el(), nxu_intake.mpidr);
}

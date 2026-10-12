/*
 * NXU MAP
 * Layer: foundation intake (S0)
 * Owns: chronicle init, image footprint from PC, MMU-on-entry reject
 * Consumes: nxu_intake residue, linker symbols
 * Produces: nxu_image.base/size; S0 chronicle records
 * Hardware: none (residue already captured)
 * Forbidden: incoming SP; firmware page tables
 * Invariant: SCTLR.M=1 rejects
 */
#include <nxu/s0.h>
#include <nxu/intake.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/image.h>

#define NXU_SCTLR_M (1ULL)

void
nxu_s0(void)
{
	nxu_chronicle_init();
	nxu_image.base = nxu_intake.pc;
	nxu_image.size = (nxu_u64)(nxu_uptr)_image_end - nxu_image.base;
	nxu_image.text_end = (nxu_u64)(nxu_uptr)__text_end;
	nxu_image.rodata_end = (nxu_u64)(nxu_uptr)__rodata_end;
	nxu_image.data_end = (nxu_u64)(nxu_uptr)__data_end;

	nxu_chronicle_put(NXU_REC_S0_ENTER, nxu_intake.currentel, nxu_intake.mpidr);
	nxu_chronicle_put(NXU_REC_S0_STACK, nxu_intake.sp_in, nxu_intake.sp_nxu);
	nxu_chronicle_put(NXU_REC_S0_VBAR, nxu_intake.vbar, 0U);

	if ((nxu_intake.sctlr & NXU_SCTLR_M) != 0U)
		nxu_fail(NXU_FAIL_MMU_ON_ENTRY, nxu_intake.sctlr, 0U);
}

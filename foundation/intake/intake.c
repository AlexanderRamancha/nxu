/*
 * NXU MAP
 * Layer: foundation intake (S1)
 * Owns: control_el; intake record shape; EL/DAIF reject
 * Consumes: nxu_intake filled by silicon entry
 * Produces: S1 chronicle records of EL, MPIDR, SCTLR, PC, x0–x3, DAIF
 * Hardware: none (values observed at entry)
 * Forbidden: inheriting firmware stack or tables
 * Invariant: steady EL is 1; DAIF masked; NXU SP ≠ incoming SP
 */
#include <nxu/intake.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/config.h>

struct nxu_intake nxu_intake;

typedef char nxu_intake_layout[(sizeof(struct nxu_intake) == 112) ? 1 : -1];

nxu_u32
nxu_control_el(void)
{
	return (nxu_u32)((nxu_intake.currentel >> 2) & 3ULL);
}

void
nxu_s1_intake(void)
{
	nxu_u32 el;

	el = nxu_control_el();
	nxu_chronicle_put(NXU_REC_S1_EL, nxu_intake.currentel, el);
	nxu_chronicle_put(NXU_REC_S1_MPIDR, nxu_intake.mpidr, 0U);
	nxu_chronicle_put(NXU_REC_S1_SCTLR, nxu_intake.sctlr, 0U);
	nxu_chronicle_put(NXU_REC_S1_PC, nxu_intake.pc, nxu_intake.x0);
	nxu_chronicle_put(NXU_REC_S1_X0, nxu_intake.x0, nxu_intake.x1);
	nxu_chronicle_put(NXU_REC_S1_X23, nxu_intake.x2, nxu_intake.x3);
	nxu_chronicle_put(NXU_REC_S1_TTBR, nxu_intake.ttbr0, nxu_intake.tcr);
	nxu_chronicle_put(NXU_REC_S1_DAIF, nxu_intake.daif, nxu_intake.sp_nxu);

	if (el != NXU_EL1)
		nxu_fail(NXU_FAIL_BAD_EL, nxu_intake.currentel, 0U);

	/* DAIF was set at entry. Bits I and F must be masked (set). */
	if ((nxu_intake.daif & 0xC0ULL) != 0xC0ULL)
		nxu_fail(NXU_FAIL_DAIF, nxu_intake.daif, 0U);

	/* Incoming SP is recorded and unused as NXU's stack. */
	if (nxu_intake.sp_nxu == 0U)
		nxu_fail(NXU_FAIL_STACK, 0U, 0U);
	if (nxu_intake.sp_nxu == nxu_intake.sp_in)
		nxu_fail(NXU_FAIL_STACK, nxu_intake.sp_in, 1U);
}

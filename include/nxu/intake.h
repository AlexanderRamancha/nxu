/*
 * NXU MAP
 * Layer: foundation intake (type)
 * Owns: residue record layout
 * Forbidden: using incoming SP
 * Invariant: two boards differ in values, not shape
 */
#ifndef NXU_INTAKE_H
#define NXU_INTAKE_H

#include <nxu/types.h>

struct nxu_intake {
	nxu_u64 x0;
	nxu_u64 x1;
	nxu_u64 x2;
	nxu_u64 x3;
	nxu_u64 pc;
	nxu_u64 sp_in;
	nxu_u64 currentel;
	nxu_u64 mpidr;
	nxu_u64 sctlr;
	nxu_u64 daif;
	nxu_u64 sp_nxu;
	nxu_u64 ttbr0;
	nxu_u64 tcr;
	nxu_u64 vbar;
};

extern struct nxu_intake nxu_intake;

nxu_u32 nxu_control_el(void);
void nxu_s1_intake(void);

#endif

#ifndef NXU_S0_H
#define NXU_S0_H

/*
 * NXU MAP
 * Layer: foundation sequencer declarations
 * Owns: S0 and foundation() symbols
 * Consumes: none
 * Produces: stage entry points for enter.c
 * Hardware: none
 * Forbidden: research symbols
 * Invariant: S0–S14 order is enter.c, not this header
 */

void nxu_s0(void);
void nxu_foundation(void);
void nxu_s2_contract(void);
void nxu_s14_freeze(void);

#endif

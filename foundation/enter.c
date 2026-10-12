/*
 * NXU MAP
 *
 *                    UNTRUSTED RESIDUE
 *                           │
 *                           ▼
 *                    SILICON ENTRY
 *                           │
 *                           ▼
 *                    FOUNDATION ENTER
 *                           │
 *        S0 S1 S2 S3 S4 S5 S6 S7 S8 S9 S10 S11 S12 S13 S14
 *                           │
 *                           ▼
 *                 OBSERVE / BLACK BOX / CONSOLE
 *
 * Layer: foundation sequencer
 * Owns: S0–S14 call order only
 * Consumes: every foundation stage
 * Produces: freeze then halt
 * Hardware: none
 * Forbidden: skipping a stage; research; board-named paths
 * Invariant: ownership-phase order; no use before ownership
 */

#include <nxu/s0.h>
#include <nxu/intake.h>
#include <nxu/facts.h>
#include <nxu/space.h>
#include <nxu/exception.h>
#include <nxu/cpu.h>
#include <nxu/signal.h>
#include <nxu/time.h>
#include <nxu/halt.h>
#include <nxu/room.h>

void
nxu_foundation(void)
{
	nxu_s0();
	nxu_s1_intake();
	nxu_s2_contract();
	nxu_s3_dtb();
	nxu_s4_facts();
	nxu_s5_ownership();
	nxu_s6_mmu();
	nxu_s7_exception();
	nxu_s8_present();
	nxu_s9_init();
	nxu_s10_gic();
	nxu_s11_time();
	nxu_s12_smp();
	nxu_s13_ipi();
	nxu_s14_freeze();
	nxu_rooms_run();
	nxu_halt();
}

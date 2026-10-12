/*
 * NXU MAP
 * Layer: foundation signal (type)
 * Owns: signal identity/owner/route
 * Forbidden: 1-of-N; handler pointers
 * Invariant: enabled implies one owner
 */
#ifndef NXU_SIGNAL_H
#define NXU_SIGNAL_H

#include <nxu/types.h>
#include <nxu/config.h>

struct nxu_signal {
	nxu_u32 intid;
	nxu_u32 owner;
	nxu_u32 route_cpu;
	nxu_u32 enabled;
};

extern struct nxu_signal nxu_sig_timer;
extern struct nxu_signal nxu_sig_ipi;

void nxu_s10_gic(void);
void nxu_s13_ipi(void);
void nxu_irq_dispatch(nxu_u32 intid);

#endif

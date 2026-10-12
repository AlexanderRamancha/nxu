/*
 * NXU MAP
 * Layer: foundation time (type)
 * Owns: deadline op prototypes
 * Forbidden: HZ/jiffies
 * Invariant: one-shot TCB signal
 */
#ifndef NXU_TIME_H
#define NXU_TIME_H

#include <nxu/types.h>

void nxu_s11_time(void);
nxu_u64 nxu_cntpct(void);
nxu_u64 nxu_cntfrq(void);
void nxu_timer_program(nxu_u64 ticks);
nxu_u32 nxu_timer_istatus(void);
void nxu_timer_ack(void);

#endif

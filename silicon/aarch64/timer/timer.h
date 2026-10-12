/*
 * NXU MAP
 * Layer: silicon timer (ops API)
 * Owns: timer op prototypes
 * Consumes: tick parameter
 * Produces: op prototypes
 * Hardware: generic timer
 * Forbidden: time policy
 * Invariant: operations only
 */
#ifndef NXU_SILICON_TIMER_H
#define NXU_SILICON_TIMER_H

#include <nxu/types.h>

nxu_u64 nxu_timer_cntpct(void);
nxu_u64 nxu_timer_cntfrq(void);
void nxu_timer_enable_el1(nxu_u32 ticks);
void nxu_timer_enable_el2(nxu_u32 ticks);
nxu_u64 nxu_timer_ctl_el1(void);
nxu_u64 nxu_timer_cval_el1(void);
nxu_u64 nxu_timer_ctl_el2(void);
nxu_u64 nxu_timer_tval_el1(void);
nxu_u64 nxu_timer_tval_el2(void);
nxu_u32 nxu_timer_istatus_el1(void);
nxu_u32 nxu_timer_istatus_el2(void);
void nxu_timer_mask_el1(void);
void nxu_timer_mask_el2(void);

#endif

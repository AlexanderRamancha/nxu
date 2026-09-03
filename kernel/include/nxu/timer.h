#ifndef NXU_TIMER_H
#define NXU_TIMER_H

#include <nxu/types.h>

int
nxu_timer_init(void);

nxu_u64
nxu_timer_now(void);

nxu_u64
nxu_timer_frequency(void);

int
nxu_timer_arm_ticks(
    nxu_u64 ticks
);

int
nxu_timer_arm_us(
    nxu_u64 microseconds
);

void
nxu_timer_stop(void);

#endif

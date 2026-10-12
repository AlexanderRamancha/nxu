#ifndef NXU_ROOM_H
#define NXU_ROOM_H

#include <nxu/types.h>

/*
 * A room is an EL0 execution interval over a private physical region.
 * The kernel map denies EL0. Only the active room's pages are visible to EL0.
 */

void nxu_rooms_run(void);
nxu_u32 nxu_room_exception(nxu_u64 esr, nxu_u64 elr, nxu_u64 arg0);
void nxu_room_irq(void);

#endif

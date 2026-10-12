#ifndef NXU_BBOX_H
#define NXU_BBOX_H

/*
 * NXU MAP
 *
 *                    FOUNDATION OBSERVE
 *                           │
 *                 ┌─────────┴─────────┐
 *                 ▼                   ▼
 *             CHRONICLE            BLACK BOX
 *                 │                   │
 *                 ▼                   ▼
 *              CONSOLE            FAIL / HALT
 *
 * Layer: foundation observe
 * Owns: bounded preserved evidence; immortal last-failure slot
 * Consumes: event code + two values from stages / fail
 * Produces: ring + last-failure retained after halt
 * Hardware: none
 * Forbidden: logging framework; authority; UART as storage
 * Invariant: last-failure is never overwritten by a non-failure
 */

#include <nxu/types.h>

void nxu_bbox_init(void);
void nxu_bbox_put(nxu_u32 code, nxu_u64 a, nxu_u64 b);
void nxu_bbox_fail(nxu_u32 code, nxu_u64 a, nxu_u64 b);
void nxu_bbox_dump(void);

#endif

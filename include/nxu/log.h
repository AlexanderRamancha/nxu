#ifndef NXU_LOG_H
#define NXU_LOG_H

/*
 * NXU MAP
 *
 *                 semantic event
 *                       │
 *                       ▼
 *                  log record
 *                       │
 *            ┌──────────┴──────────┐
 *            ▼                     ▼
 *        chronicle              console
 *
 * Layer: foundation observe
 * Owns: event identity and the record passed to console
 * Consumes: event, two or three values, failure location
 * Produces: one chronicle slot; one console presentation
 * Hardware: none
 * Forbidden: allocation; halt on a full ring; UART policy
 * Invariant: a log call does not change architectural state
 */

#include <nxu/types.h>

#define NXU_SEV_NOTE 1U
#define NXU_SEV_FAIL 2U

#define NXU_SUB_BOOT 1U
#define NXU_SUB_FACT 2U
#define NXU_SUB_SPACE 3U
#define NXU_SUB_EXC  4U
#define NXU_SUB_CPU  5U
#define NXU_SUB_GIC  6U
#define NXU_SUB_TIME 7U
#define NXU_SUB_SMP  8U
#define NXU_SUB_FAIL 9U

/* Events that already occur on the boot path. Ids sit above chronicle stage codes. */
#define NXU_EV_BOOT       0x00021001U
#define NXU_EV_DTB        0x00021002U
#define NXU_EV_MEMORY     0x00021003U
#define NXU_EV_MMU        0x00021004U
#define NXU_EV_EXCEPTION  0x00021005U
#define NXU_EV_FAULT      0x00021006U
#define NXU_EV_CPU        0x00021007U
#define NXU_CPU_PRESENT   1U
#define NXU_CPU_INIT      2U
#define NXU_CPU_GIC       3U
#define NXU_CPU_EL1       4U
#define NXU_CPU_VBAR      5U
#define NXU_CPU_CLUSTER   6U
#define NXU_CPU_DETECT    7U
#define NXU_CPU_ENTER     8U
#define NXU_CPU_RDIST     16U
#define NXU_IPI_SEND      0U
#define NXU_IPI_RECV      1U
#define NXU_EV_GIC        0x00021008U
#define NXU_EV_GIC_INIT   0x00021009U
#define NXU_EV_TIMER      0x0002100AU
#define NXU_EV_TIMER_FIRE 0x0002100BU
#define NXU_EV_CPU_ONLINE 0x0002100CU
#define NXU_EV_CPU_SET    0x0002100DU
#define NXU_EV_IPI        0x0002100EU
#define NXU_EV_KERNEL     0x0002100FU
#define NXU_EV_FAIL       0x00021010U
#define NXU_EV_MACHINE    0x00021011U
#define NXU_EV_BOUND      0x00021012U
#define NXU_EV_ROOM       0x00021013U
#define NXU_EV_PULSE      0x00021014U
#define NXU_EV_APP        0x00021015U

/* Selectors for NXU_EV_MACHINE. v0 is the selector, v1 is the observed value. */
#define NXU_MACH_WAKER     1U
#define NXU_MACH_GICD_CTLR 2U
#define NXU_MACH_IGROUPR0  3U
#define NXU_MACH_ISENABLER0 4U
#define NXU_MACH_ISPENDR0  5U
#define NXU_MACH_IGRPMODR0 6U
#define NXU_MACH_DAIF      7U
#define NXU_MACH_HCR       8U
#define NXU_MACH_ISR       9U
#define NXU_MACH_IGRPEN1   10U
#define NXU_MACH_HPPIR1    11U
#define NXU_MACH_HPPIR0    12U
#define NXU_MACH_T0        13U
#define NXU_MACH_TNOW      14U
#define NXU_MACH_TVAL      15U
#define NXU_MACH_ISTATUS   16U
#define NXU_MACH_CNTP_CTL  17U
#define NXU_MACH_CNTP_CVAL 18U

struct nxu_log_record {
	nxu_u32 event;
	nxu_u32 severity;
	nxu_u32 subsystem;
	nxu_u32 cpu;
	nxu_u64 v0;
	nxu_u64 v1;
	nxu_u64 v2;
	const char *func;
	const char *file;
	nxu_u32 line;
};

void nxu_log(nxu_u32 event, nxu_u32 severity, nxu_u32 subsystem,
    nxu_u64 v0, nxu_u64 v1, nxu_u64 v2,
    const char *func, const char *file, nxu_u32 line);

#define NXU_LOG(ev, sub, v0, v1) \
	nxu_log((ev), NXU_SEV_NOTE, (sub), (v0), (v1), 0, NXU_NULL, NXU_NULL, 0)

#define NXU_LOG3(ev, sub, v0, v1, v2) \
	nxu_log((ev), NXU_SEV_NOTE, (sub), (v0), (v1), (v2), NXU_NULL, NXU_NULL, 0)

#endif

#ifndef NXU_GIC_REGS_H
#define NXU_GIC_REGS_H


/*
 * ==========================================================================
 * GICv3 Distributor
 * ==========================================================================
 */

#define GICD_CTLR               0x0000U
#define GICD_TYPER              0x0004U
#define GICD_IIDR               0x0008U

#define GICD_IGROUPR            0x0080U
#define GICD_ISENABLER          0x0100U
#define GICD_ICENABLER          0x0180U

#define GICD_ISPENDR            0x0200U
#define GICD_ICPENDR            0x0280U

#define GICD_IPRIORITYR         0x0400U
#define GICD_ICFGR              0x0C00U

#define GICD_IROUTER            0x6100U


/*
 * ==========================================================================
 * GICD_CTLR
 * ==========================================================================
 */

#define GICD_CTLR_ENABLE_GRP1_NS   (1U << 1)
#define GICD_CTLR_ARE_NS            (1U << 4)
#define GICD_CTLR_DS                (1U << 6)


/*
 * ==========================================================================
 * GICD_TYPER
 * ==========================================================================
 */

#define GICD_TYPER_ITLINES_SHIFT    0U
#define GICD_TYPER_ITLINES_MASK     0x1FU

#define GICD_TYPER_IDBITS_SHIFT     19U
#define GICD_TYPER_IDBITS_MASK      0x1FU


/*
 * ==========================================================================
 * GICv3 Redistributor
 * ==========================================================================
 *
 * The Redistributor has two logical regions:
 *
 *     Redistributor frame
 *         0x00000 ...
 *
 *     SGI/PPI frame
 *         0x10000 ...
 *
 * Therefore SGI/PPI registers MUST include:
 *
 *     GICR_SGI_BASE
 *
 * in their address.
 */

#define GICR_CTLR               0x0000U
#define GICR_IIDR               0x0004U
#define GICR_TYPER              0x0008U
#define GICR_STATUSR            0x0010U
#define GICR_WAKER              0x0014U


/*
 * ==========================================================================
 * Redistributor SGI/PPI frame
 * ==========================================================================
 */

#define GICR_SGI_BASE           0x10000U

#define GICR_IGROUPR0           (GICR_SGI_BASE + 0x0080U)
#define GICR_ISENABLER0         (GICR_SGI_BASE + 0x0100U)
#define GICR_ICENABLER0         (GICR_SGI_BASE + 0x0180U)

#define GICR_ISPENDR0           (GICR_SGI_BASE + 0x0200U)
#define GICR_ICPENDR0           (GICR_SGI_BASE + 0x0280U)

#define GICR_ISACTIVER0         (GICR_SGI_BASE + 0x0300U)
#define GICR_ICACTIVER0         (GICR_SGI_BASE + 0x0380U)

#define GICR_IPRIORITYR0        (GICR_SGI_BASE + 0x0400U)

#define GICR_ICFGR0             (GICR_SGI_BASE + 0x0C00U)
#define GICR_ICFGR1             (GICR_SGI_BASE + 0x0C04U)

#define GICR_IGRPMODR0          (GICR_SGI_BASE + 0x0D00U)
#define GICR_NSACR              (GICR_SGI_BASE + 0x0E00U)


/*
 * ==========================================================================
 * GICR_WAKER
 * ==========================================================================
 */

#define GICR_WAKER_PROCESSOR_SLEEP   (1U << 1)
#define GICR_WAKER_CHILDREN_ASLEEP   (1U << 2)


/*
 * ==========================================================================
 * GICR_TYPER
 * ==========================================================================
 */

#define GICR_TYPER_LAST              (1ULL << 4)


/*
 * ==========================================================================
 * Redistributor frame size
 * ==========================================================================
 *
 * QEMU's GICv3 Redistributors are separated by 128 KiB.
 */

#define GICR_FRAME_SIZE              0x20000UL


#endif
#ifndef NXU_CONFIG_H
#define NXU_CONFIG_H

/*
 * NXU MAP
 * Layer: foundation config
 * Owns: compile-time bounds (CPU, stack, DTB cap, chronicle, black box)
 * Forbidden: board addresses; CPU count as topology truth
 * Invariant: DTB cap is not platform RAM size
 */

#define NXU_CPU_BOUND        8U
#define NXU_STACK_SIZE       0x10000U
#define NXU_DTB_PROBE_CAP    0x100000U
#define NXU_MEM_FACT_MAX     8U
#define NXU_PT_PAGES         64U
#define NXU_CHRONICLE_CAP    128U
#define NXU_BBOX_CAP         32U
#define NXU_OWNER_MAX        16U

#define NXU_EL1 1U
#define NXU_EL2 2U
#define NXU_EL3 3U

#define NXU_PSCI_NONE 0U
#define NXU_PSCI_SMC  1U
#define NXU_PSCI_HVC  2U

#define NXU_CPU_PRESENT     1U
#define NXU_CPU_INITIALIZED 2U
#define NXU_CPU_AVAILABLE   3U
#define NXU_CPU_STARTING    4U
#define NXU_CPU_ONLINE      5U
#define NXU_CPU_FAILED      6U

#define NXU_OWNER_UNKNOWN   0U
#define NXU_OWNER_FIRMWARE  1U
#define NXU_OWNER_IMAGE     2U
#define NXU_OWNER_NXU       3U
#define NXU_OWNER_MMIO      4U
#define NXU_OWNER_RESERVED  5U

#define NXU_GRADE_OBSERVED     1U
#define NXU_GRADE_SINGLE       2U
#define NXU_GRADE_CORROBORATED 3U

#define NXU_SRC_OBS   (1U << 0)
#define NXU_SRC_DTB   (1U << 1)
#define NXU_SRC_BOARD (1U << 2)

#define NXU_CTL_GICD 1U
#define NXU_CTL_GICR 2U
#define NXU_CTL_UART 3U

#define NXU_CTX_INERT   0U
#define NXU_CTX_ARMED   1U
#define NXU_CTX_RUNNING 2U
#define NXU_CTX_WAITING 3U
#define NXU_CTX_FAULTED 4U

#define NXU_SIG_OWNER_NONE 0U
#define NXU_SIG_OWNER_TCB  1U

#define NXU_SGI_IPI 0U

#define NXU_PAGE_SIZE 0x1000ULL
#define NXU_BLOCK_2M  0x200000ULL

#endif

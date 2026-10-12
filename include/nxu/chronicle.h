/*
 * NXU MAP
 * Layer: foundation observe (type)
 * Owns: record codes and per-CPU chronicle layout
 * Forbidden: logging framework
 * Invariant: last-failure slot never overwritten by non-failure
 */
#ifndef NXU_CHRONICLE_H
#define NXU_CHRONICLE_H

#include <nxu/types.h>
#include <nxu/config.h>

#define NXU_REC_S0_ENTER          0x00000001U
#define NXU_REC_S0_STACK          0x00000002U
#define NXU_REC_S0_VBAR           0x00000003U
#define NXU_REC_S1_EL              0x00000101U
#define NXU_REC_S1_MPIDR           0x00000102U
#define NXU_REC_S1_SCTLR           0x00000103U
#define NXU_REC_S1_PC              0x00000104U
#define NXU_REC_S1_X0              0x00000105U
#define NXU_REC_S1_TTBR            0x00000106U
#define NXU_REC_S1_DAIF            0x00000107U
#define NXU_REC_S1_X23             0x00000108U
#define NXU_REC_S2_HEADER          0x00000201U
#define NXU_REC_S2_EL              0x00000202U
#define NXU_REC_S3_DTB             0x00000301U
#define NXU_REC_S3_DTB_ABSENT      0x00000302U
#define NXU_REC_S4_RAM             0x00000401U
#define NXU_REC_S4_GRADE           0x00000402U
#define NXU_REC_S4_UART            0x00000403U
#define NXU_REC_S4_GIC             0x00000404U
#define NXU_REC_S4_PSCI            0x00000405U
#define NXU_REC_S4_CPU             0x00000406U
#define NXU_REC_S4_TIMER           0x00000407U
#define NXU_REC_S5_OWNER           0x00000501U
#define NXU_REC_S6_MMU             0x00000601U
#define NXU_REC_S7_CTX             0x00000701U
#define NXU_REC_S7_VBAR            0x00000702U
#define NXU_REC_S7_FAULT           0x00000703U
#define NXU_REC_S8_CPU             0x00000801U
#define NXU_REC_S8_PRESENT         0x00000802U
#define NXU_REC_S9_INIT            0x00000901U
#define NXU_REC_S10_GIC            0x00000A01U
#define NXU_REC_S10_RDIST          0x00000A02U
#define NXU_REC_S10_FREEZE         0x00000A03U
#define NXU_REC_S11_FREQ           0x00000B01U
#define NXU_REC_S11_PPI            0x00000B02U
#define NXU_REC_S11_FIRE           0x00000B03U
#define NXU_REC_S12_ON             0x00000C01U
#define NXU_REC_S12_ONLINE         0x00000C02U
#define NXU_REC_S13_IPI            0x00000D01U
#define NXU_REC_S13_RECV           0x00000D02U
#define NXU_REC_S13_TARGET         0x00000D03U
#define NXU_REC_S14_FREEZE         0x00000E01U
#define NXU_REC_IRQ                0x00000F01U
#define NXU_REC_SEC_ENTER          0x00001001U

#define NXU_FAIL_MMU_ON_ENTRY      0x80000001U
#define NXU_FAIL_BAD_EL            0x80000002U
#define NXU_FAIL_BAD_HEADER        0x80000003U
#define NXU_FAIL_DTB               0x80000004U
#define NXU_FAIL_DTB_OVERLAP       0x80000005U
#define NXU_FAIL_NO_RAM            0x80000006U
#define NXU_FAIL_IMAGE_RAM         0x80000007U
#define NXU_FAIL_FACT_MISMATCH     0x80000008U
#define NXU_FAIL_WRAP              0x80000009U
#define NXU_FAIL_PT                0x8000000AU
#define NXU_FAIL_CPU               0x8000000BU
#define NXU_FAIL_GIC               0x8000000CU
#define NXU_FAIL_TIMER             0x8000000DU
#define NXU_FAIL_PSCI              0x8000000EU
#define NXU_FAIL_TIMEOUT           0x8000000FU
#define NXU_FAIL_UNBOUND_IRQ       0x80000010U
#define NXU_FAIL_SYNC              0x80000011U
#define NXU_FAIL_SERROR            0x80000012U
#define NXU_FAIL_STACK             0x80000013U
#define NXU_FAIL_INVARIANT         0x80000014U
#define NXU_FAIL_DAIF              0x80000015U
#define NXU_FAIL_WRONG_TARGET      0x80000016U

struct nxu_chronicle_record {
	nxu_u32 seq;
	nxu_u32 code;
	nxu_u64 a;
	nxu_u64 b;
};

struct nxu_chronicle {
	nxu_u32 next;
	nxu_u32 count;
	nxu_u32 fail_code;
	nxu_u64 fail_a;
	nxu_u64 fail_b;
	const char *fail_func;
	const char *fail_file;
	nxu_u32 fail_line;
	nxu_u8  fail_valid;
	struct nxu_chronicle_record rec[NXU_CHRONICLE_CAP];
};

extern struct nxu_chronicle nxu_cpu_chronicle[NXU_CPU_BOUND];

void nxu_chronicle_init(void);
void nxu_chronicle_put(nxu_u32 code, nxu_u64 a, nxu_u64 b);
void nxu_chronicle_fail(nxu_u32 code, nxu_u64 a, nxu_u64 b);
struct nxu_chronicle *nxu_chronicle_this(void);

#endif

/*
 * NXU MAP
 * Layer: silicon translate
 * Owns: page-table programming and SCTLR.M enable
 * Consumes: PA ranges and perms from foundation
 * Produces: TTBR/TCR/MAIR programmed; MMU on
 * Hardware: stage-1 tables, TLBI
 * Forbidden: board addresses; ownership decisions
 * Invariant: operations only
 */
#include <silicon/aarch64/translate/mmu.h>
#include <silicon/aarch64/sysreg.h>
#include <nxu/config.h>
#include <nxu/range.h>
#include <nxu/image.h>
#include <nxu/fail.h>
#include <nxu/chronicle.h>

#define PTE_VALID   (1ULL << 0)
#define PTE_TABLE   (1ULL << 1)
#define PTE_PAGE    (1ULL << 1)
#define PTE_AF      (1ULL << 10)
#define PTE_ISH     (3ULL << 8)
#define PTE_AP_RW   (0ULL << 6)
#define PTE_AP_RO   (2ULL << 6)
#define PTE_NX      (1ULL << 54)
#define PTE_PXN     (1ULL << 53)
#define PTE_ATTR(i) ((nxu_u64)(i) << 2)

static nxu_u64 nxu_pt[NXU_PT_PAGES][512] __attribute__((aligned(4096)));
static nxu_u32 nxu_pt_used;
static nxu_u64 nxu_ttbr0;
static nxu_u64 nxu_tcr_val;
static nxu_u64 nxu_mair_val;

static nxu_u64
pt_pa(nxu_u32 idx)
{
	return nxu_pa_of(&nxu_pt[idx][0]);
}

static nxu_u32
pt_alloc(void)
{
	nxu_u32 i;

	if (nxu_pt_used >= NXU_PT_PAGES)
		nxu_fail(NXU_FAIL_PT, nxu_pt_used, 0U);
	i = nxu_pt_used;
	nxu_pt_used++;
	return i;
}

static nxu_u32
pt_index(nxu_u64 pa)
{
	nxu_u32 i;

	for (i = 0U; i < nxu_pt_used; i++) {
		if (pt_pa(i) == pa)
			return i;
	}
	nxu_fail(NXU_FAIL_PT, pa, 1U);
	return 0U;
}

static nxu_u64
leaf_attr(nxu_u32 perm)
{
	nxu_u64 a;

	a = PTE_VALID | PTE_AF | PTE_ISH;
	if (perm == NXU_MAP_DEV) {
		a |= PTE_ATTR(0) | PTE_AP_RW | PTE_NX | PTE_PXN | PTE_PAGE;
		return a;
	}
	a |= PTE_ATTR(1);
	if (perm == NXU_MAP_RX)
		a |= PTE_AP_RO | PTE_NX | PTE_PAGE;
	else if (perm == NXU_MAP_USER_RX)
		a |= (3ULL << 6) | PTE_PXN | PTE_PAGE;
	else if (perm == NXU_MAP_USER_RW)
		a |= (1ULL << 6) | PTE_NX | PTE_PXN | PTE_PAGE;
	else if (perm == NXU_MAP_R)
		a |= PTE_AP_RO | PTE_NX | PTE_PXN | PTE_PAGE;
	else
		a |= PTE_AP_RW | PTE_NX | PTE_PXN | PTE_PAGE;
	return a;
}

void
nxu_mmu_init_pool(void)
{
	nxu_pt_used = 0U;
	(void)pt_alloc();
	nxu_ttbr0 = pt_pa(0U);
	nxu_mair_val = 0x000000000000FF00ULL;
	if (nxu_current_el() == 2U) {
		nxu_tcr_val =
		    (16ULL << 0) |
		    (1ULL << 8) |
		    (1ULL << 10) |
		    (3ULL << 12) |
		    (5ULL << 16);
	} else {
		nxu_tcr_val =
		    (16ULL << 0) |
		    (1ULL << 8) |
		    (1ULL << 10) |
		    (3ULL << 12) |
		    (5ULL << 32) |
		    (1ULL << 23);
	}
}

static void
map_page(nxu_u64 pa, nxu_u32 perm)
{
	nxu_u32 l0;
	nxu_u32 l1;
	nxu_u32 l2;
	nxu_u32 l3;
	nxu_u64 e;
	nxu_u32 t0 = 0U;
	nxu_u32 t1;
	nxu_u32 t2;
	nxu_u32 t3;

	l0 = (nxu_u32)((pa >> 39) & 0x1FFULL);
	l1 = (nxu_u32)((pa >> 30) & 0x1FFULL);
	l2 = (nxu_u32)((pa >> 21) & 0x1FFULL);
	l3 = (nxu_u32)((pa >> 12) & 0x1FFULL);

	e = nxu_pt[t0][l0];
	if ((e & PTE_VALID) == 0U) {
		t1 = pt_alloc();
		nxu_pt[t0][l0] = pt_pa(t1) | PTE_VALID | PTE_TABLE;
	} else {
		t1 = pt_index(e & 0x0000FFFFFFFFF000ULL);
	}

	e = nxu_pt[t1][l1];
	if ((e & PTE_VALID) == 0U) {
		t2 = pt_alloc();
		nxu_pt[t1][l1] = pt_pa(t2) | PTE_VALID | PTE_TABLE;
	} else {
		t2 = pt_index(e & 0x0000FFFFFFFFF000ULL);
	}

	e = nxu_pt[t2][l2];
	if ((e & PTE_VALID) == 0U) {
		t3 = pt_alloc();
		nxu_pt[t2][l2] = pt_pa(t3) | PTE_VALID | PTE_TABLE;
	} else {
		t3 = pt_index(e & 0x0000FFFFFFFFF000ULL);
	}

	nxu_pt[t3][l3] = (pa & 0x0000FFFFFFFFF000ULL) | leaf_attr(perm);
}

void
nxu_mmu_tlbi(void)
{
	asm volatile("tlbi vmalle1" ::: "memory");
	nxu_dsb();
	nxu_isb();
}

nxu_u64
nxu_mmu_leaf(nxu_u64 pa)
{
	nxu_u32 l0;
	nxu_u32 l1;
	nxu_u32 l2;
	nxu_u32 l3;
	nxu_u64 e;
	nxu_u32 t1;
	nxu_u32 t2;
	nxu_u32 t3;

	l0 = (nxu_u32)((pa >> 39) & 0x1FFULL);
	l1 = (nxu_u32)((pa >> 30) & 0x1FFULL);
	l2 = (nxu_u32)((pa >> 21) & 0x1FFULL);
	l3 = (nxu_u32)((pa >> 12) & 0x1FFULL);
	e = nxu_pt[0][l0];
	if ((e & PTE_VALID) == 0U)
		return 0U;
	t1 = pt_index(e & 0x0000FFFFFFFFF000ULL);
	e = nxu_pt[t1][l1];
	if ((e & PTE_VALID) == 0U)
		return 0U;
	t2 = pt_index(e & 0x0000FFFFFFFFF000ULL);
	e = nxu_pt[t2][l2];
	if ((e & PTE_VALID) == 0U)
		return 0U;
	t3 = pt_index(e & 0x0000FFFFFFFFF000ULL);
	return nxu_pt[t3][l3];
}

void
nxu_mmu_map(nxu_u64 pa, nxu_u64 size, nxu_u32 perm)
{
	nxu_u64 off;

	if (!nxu_range_ok(pa, size))
		nxu_fail(NXU_FAIL_WRAP, pa, size);
	if ((pa & (NXU_PAGE_SIZE - 1ULL)) != 0U)
		nxu_fail(NXU_FAIL_PT, pa, 2U);
	if ((size & (NXU_PAGE_SIZE - 1ULL)) != 0U)
		nxu_fail(NXU_FAIL_PT, size, 3U);
	for (off = 0U; off < size; off += NXU_PAGE_SIZE)
		map_page(pa + off, perm);
}

nxu_u64
nxu_mmu_probe(nxu_u64 va)
{
	nxu_u64 par;

	/* PAR_EL1 semantics: bit[0]=1 means fault, DFSC in bits[5:0];
	 * bit[0]=0 means translated, output address in bits[52:12]. */
	asm volatile("at s1e1r, %0" :: "r"(va) : "memory");
	par = nxu_mrs(S3_0_C7_C4_0);
	nxu_isb();
	return par;
}

nxu_u64
nxu_mmu_ttbr(void)
{
	return nxu_ttbr0;
}

nxu_u64
nxu_mmu_tcr(void)
{
	return nxu_tcr_val;
}

nxu_u64
nxu_mmu_mair(void)
{
	return nxu_mair_val;
}

void
nxu_mmu_enable(nxu_u32 el)
{
	nxu_u64 sctlr;

	if (el != NXU_EL1)
		nxu_fail(NXU_FAIL_BAD_EL, el, 3U);
	nxu_dsb();
	nxu_msr(mair_el1, nxu_mair_val);
	nxu_msr(tcr_el1, nxu_tcr_val);
	nxu_msr(ttbr0_el1, nxu_ttbr0);
	nxu_isb();
	asm volatile("tlbi vmalle1" ::: "memory");
	nxu_dsb();
	nxu_isb();
	sctlr = nxu_mrs(sctlr_el1);
	sctlr |= (1ULL << 0) | (1ULL << 2) | (1ULL << 12) | (1ULL << 19);
	sctlr &= ~(1ULL << 1);
	nxu_msr(sctlr_el1, sctlr);
	nxu_isb();
}

/*
 * NXU MAP
 * Layer: foundation facts decoder (S3)
 * Owns: bounded DTB probe; candidate extraction; wrap-safe membership
 * Consumes: x0 pointer; image footprint for overlap
 * Produces: nxu_dtb_cand (not accepted facts)
 * Hardware: none; untrusted blob read in bootstrap window
 * Forbidden: MMIO use; policy; treating DTB as truth
 * Invariant: bad magic / oversized / overlap / wrap → halt
 */
#include <nxu/facts.h>
#include <nxu/fdt.h>
#include <nxu/intake.h>
#include <nxu/image.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/range.h>
#include <nxu/board.h>

#define FDT_MAGIC 0xd00dfeedU
#define FDT_BEGIN_NODE 0x00000001U
#define FDT_END_NODE   0x00000002U
#define FDT_PROP       0x00000003U
#define FDT_NOP        0x00000004U
#define FDT_END        0x00000009U

struct nxu_dtb_cand nxu_dtb_cand;

struct nxu_fdt_node {
	nxu_u32 ac;
	nxu_u32 sc;
	nxu_u32 reg_ac;
	nxu_u32 reg_sc;
	nxu_u32 is_mem;
	nxu_u32 is_cpu;
	nxu_u32 is_gic;
	nxu_u32 is_uart;
	nxu_u32 is_psci;
	nxu_u32 method;
	nxu_u32 nreg;
	struct nxu_range regs[4];
};

static nxu_u32
be32(const nxu_u8 *p)
{
	return ((nxu_u32)p[0] << 24) | ((nxu_u32)p[1] << 16) |
	    ((nxu_u32)p[2] << 8) | (nxu_u32)p[3];
}

static nxu_u64
be_cells(const nxu_u8 *p, nxu_u32 cells)
{
	nxu_u64 v = 0U;
	nxu_u32 i;

	for (i = 0U; i < cells; i++)
		v = (v << 32) | (nxu_u64)be32(p + i * 4U);
	return v;
}

static int
str_eq(const char *a, const char *b)
{
	while (*a != 0 && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static int
str_has(const char *list, nxu_u32 len, const char *want)
{
	nxu_u32 i = 0U;

	while (i < len) {
		const char *s = list + i;
		nxu_u32 n = 0U;
		while (i + n < len && s[n] != 0)
			n++;
		if (str_eq(s, want))
			return 1;
		i += n + 1U;
	}
	return 0;
}

static nxu_u32
align4(nxu_u32 v)
{
	return (v + 3U) & ~3U;
}

static void
add_mem(struct nxu_dtb_cand *c, nxu_u64 base, nxu_u64 size)
{
	if (!nxu_range_ok(base, size))
		nxu_fail(NXU_FAIL_WRAP, base, size);
	if (c->mem_count >= NXU_MEM_FACT_MAX)
		nxu_fail(NXU_FAIL_DTB, base, 3U);
	c->mem[c->mem_count].base = base;
	c->mem[c->mem_count].size = size;
	c->mem_count++;
}

static void
add_cpu(struct nxu_dtb_cand *c, nxu_u64 mpidr)
{
	nxu_u32 i;

	mpidr &= 0xFF00FFFFFFULL;
	for (i = 0U; i < c->cpu_count; i++) {
		if (c->cpu_mpidr[i] == mpidr)
			return;
	}
	if (c->cpu_count >= NXU_CPU_BOUND)
		nxu_fail(NXU_FAIL_CPU, mpidr, 0U);
	c->cpu_mpidr[c->cpu_count] = mpidr;
	c->cpu_count++;
}

static void
parse_reg(const nxu_u8 *p, nxu_u32 len, nxu_u32 ac, nxu_u32 sc,
    struct nxu_range *out, nxu_u32 max, nxu_u32 *count)
{
	nxu_u32 step;
	nxu_u32 off;

	if (ac == 0U)
		ac = 1U;
	if (sc == 0U) {
		if (len == ac * 4U && max > 0U) {
			out[0].base = be_cells(p, ac);
			out[0].size = 0U;
			*count = 1U;
		}
		return;
	}
	step = (ac + sc) * 4U;
	if (step == 0U)
		return;
	off = 0U;
	*count = 0U;
	while (off + step <= len && *count < max) {
		out[*count].base = be_cells(p + off, ac);
		out[*count].size = be_cells(p + off + ac * 4U, sc);
		(*count)++;
		off += step;
	}
}

void
nxu_s3_dtb(void)
{
	nxu_u64 ptr;
	const nxu_u8 *blob;
	nxu_u32 totalsize;
	nxu_u32 off_struct;
	nxu_u32 off_strings;
	nxu_u32 size_struct;
	nxu_u32 size_strings;
	nxu_u32 pos;
	nxu_u32 depth;
	struct nxu_fdt_node st[16];
	struct nxu_dtb_cand *c;

	c = &nxu_dtb_cand;
	c->valid = 0U;
	c->mem_count = 0U;
	c->cpu_count = 0U;
	c->uart_valid = 0U;
	c->psci_conduit = 0U;
	c->timer_ppi = 0U;
	c->gicd.base = 0U;
	c->gicd.size = 0U;
	c->gicr.base = 0U;
	c->gicr.size = 0U;
	c->base = 0U;
	c->size = 0U;

	ptr = nxu_intake.x0;
	if (ptr == 0U) {
		nxu_chronicle_put(NXU_REC_S3_DTB_ABSENT, 0U, 0U);
		return;
	}
	if ((ptr & 7ULL) != 0U)
		nxu_fail(NXU_FAIL_DTB, ptr, 0U);

	blob = (const nxu_u8 *)(nxu_uptr)ptr;
	if (be32(blob) != FDT_MAGIC)
		nxu_fail(NXU_FAIL_DTB, ptr, 1U);
	totalsize = be32(blob + 4);
	if (totalsize < 40U || totalsize > NXU_DTB_PROBE_CAP)
		nxu_fail(NXU_FAIL_DTB, totalsize, 2U);
	if (!nxu_range_ok(ptr, totalsize))
		nxu_fail(NXU_FAIL_WRAP, ptr, totalsize);
	if (nxu_range_overlap(ptr, totalsize, nxu_image.base, nxu_image.size))
		nxu_fail(NXU_FAIL_DTB_OVERLAP, ptr, totalsize);

	off_struct = be32(blob + 8);
	off_strings = be32(blob + 12);
	size_struct = be32(blob + 36);
	size_strings = be32(blob + 32);
	if (off_struct + size_struct > totalsize)
		nxu_fail(NXU_FAIL_DTB, off_struct, 4U);
	if (off_strings + size_strings > totalsize)
		nxu_fail(NXU_FAIL_DTB, off_strings, 5U);

	c->base = ptr;
	c->size = totalsize;
	st[0].ac = 2U;
	st[0].sc = 1U;
	st[0].reg_ac = 2U;
	st[0].reg_sc = 1U;
	st[0].is_mem = 0U;
	st[0].is_cpu = 0U;
	st[0].is_gic = 0U;
	st[0].is_uart = 0U;
	st[0].is_psci = 0U;
	st[0].method = 0U;
	st[0].nreg = 0U;
	depth = 0U;
	pos = off_struct;

	while (pos + 4U <= off_struct + size_struct) {
		nxu_u32 tok = be32(blob + pos);
		struct nxu_fdt_node *n;
		pos += 4U;
		if (tok == FDT_NOP)
			continue;
		if (tok == FDT_END)
			break;
		if (tok == FDT_BEGIN_NODE) {
			nxu_u32 nlen = 0U;
			const char *nm = (const char *)(blob + pos);
			while (pos + nlen < off_struct + size_struct && nm[nlen] != 0)
				nlen++;
			if (depth + 1U >= 16U)
				nxu_fail(NXU_FAIL_DTB, depth, 6U);
			depth++;
			st[depth].ac = st[depth - 1U].ac;
			st[depth].sc = st[depth - 1U].sc;
			st[depth].reg_ac = st[depth - 1U].ac;
			st[depth].reg_sc = st[depth - 1U].sc;
			st[depth].is_mem = 0U;
			st[depth].is_cpu = 0U;
			st[depth].is_gic = 0U;
			st[depth].is_uart = 0U;
			st[depth].is_psci = 0U;
			st[depth].method = 0U;
			st[depth].nreg = 0U;
			pos = align4(pos + nlen + 1U);
			continue;
		}
		if (tok == FDT_END_NODE) {
			nxu_u32 k;

			n = &st[depth];
			if (n->is_mem) {
				for (k = 0U; k < n->nreg; k++)
					add_mem(c, n->regs[k].base, n->regs[k].size);
			}
			if (n->is_cpu && n->nreg > 0U)
				add_cpu(c, n->regs[0].base);
			if (n->is_gic && n->nreg >= 2U && c->gicd.size == 0U) {
				c->gicd = n->regs[0];
				c->gicr = n->regs[1];
			}
			if (n->is_uart && n->nreg > 0U) {
				c->uart = n->regs[0];
				c->uart_valid = 1U;
			}
			if (n->is_psci && n->method != 0U)
				c->psci_conduit = n->method;
			if (depth == 0U)
				nxu_fail(NXU_FAIL_DTB, 0U, 7U);
			depth--;
			continue;
		}
		if (tok != FDT_PROP)
			nxu_fail(NXU_FAIL_DTB, tok, 8U);
		{
			nxu_u32 plen = be32(blob + pos);
			nxu_u32 nameoff = be32(blob + pos + 4);
			const char *pname;
			const nxu_u8 *pval;

			pos += 8U;
			if (nameoff >= size_strings)
				nxu_fail(NXU_FAIL_DTB, nameoff, 9U);
			pname = (const char *)(blob + off_strings + nameoff);
			pval = blob + pos;
			if (pos + plen > off_struct + size_struct)
				nxu_fail(NXU_FAIL_DTB, plen, 10U);
			n = &st[depth];
			if (str_eq(pname, "#address-cells") && plen == 4U)
				n->ac = be32(pval);
			else if (str_eq(pname, "#size-cells") && plen == 4U)
				n->sc = be32(pval);
			else if (str_eq(pname, "device_type") && plen >= 4U) {
				if (str_eq((const char *)pval, "memory"))
					n->is_mem = 1U;
				if (str_eq((const char *)pval, "cpu"))
					n->is_cpu = 1U;
			} else if (str_eq(pname, "compatible")) {
				if (str_has((const char *)pval, plen, "arm,gic-v3"))
					n->is_gic = 1U;
				if (str_has((const char *)pval, plen, "arm,pl011") ||
				    str_has((const char *)pval, plen, "arm,pl011-axi"))
					n->is_uart = 1U;
				if (str_has((const char *)pval, plen, "arm,psci") ||
				    str_has((const char *)pval, plen, "arm,psci-0.2") ||
				    str_has((const char *)pval, plen, "arm,psci-1.0"))
					n->is_psci = 1U;
			} else if (str_eq(pname, "reg")) {
				parse_reg(pval, plen, n->reg_ac, n->reg_sc, n->regs, 4U, &n->nreg);
			} else if (str_eq(pname, "method")) {
				if (str_eq((const char *)pval, "hvc"))
					n->method = NXU_PSCI_HVC;
				else if (str_eq((const char *)pval, "smc"))
					n->method = NXU_PSCI_SMC;
			}
			pos = align4(pos + plen);
		}
	}

	c->valid = 1U;
	nxu_chronicle_put(NXU_REC_S3_DTB, c->base, c->size);
}

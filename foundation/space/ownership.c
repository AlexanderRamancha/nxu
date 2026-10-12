/*
 * NXU MAP
 * Layer: foundation space (S5)
 * Owns: physical ownership table
 * Consumes: accepted RAM, image, DTB range, MMIO facts
 * Produces: owner classes; unknown unmapped later
 * Hardware: none
 * Forbidden: mapping unknown; overflowing address+size
 * Invariant: wrap-safe ranges; IMAGE/NXU/MMIO/RESERVED only
 */
#include <nxu/space.h>
#include <nxu/facts.h>
#include <nxu/image.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>

struct nxu_space nxu_space;

static nxu_u64
page_up(nxu_u64 n)
{
	return (n + NXU_PAGE_SIZE - 1ULL) & ~(NXU_PAGE_SIZE - 1ULL);
}

static void
add_owner(nxu_u64 base, nxu_u64 size, nxu_u32 owner)
{
	if (!nxu_range_ok(base, size))
		nxu_fail(NXU_FAIL_WRAP, base, size);
	if (nxu_space.count >= NXU_OWNER_MAX)
		nxu_fail(NXU_FAIL_INVARIANT, owner, 0U);
	nxu_space.region[nxu_space.count].range.base = base;
	nxu_space.region[nxu_space.count].range.size = size;
	nxu_space.region[nxu_space.count].owner = owner;
	nxu_space.count++;
	nxu_chronicle_put(NXU_REC_S5_OWNER, base, ((nxu_u64)owner << 32) | size);
}

nxu_u32
nxu_space_owner(nxu_u64 pa, nxu_u64 size)
{
	nxu_u32 i;
	nxu_u32 owner;
	nxu_u64 best;

	owner = NXU_OWNER_UNKNOWN;
	best = ~0ULL;
	for (i = 0U; i < nxu_space.count; i++) {
		if (!nxu_range_contains(nxu_space.region[i].range.base,
		    nxu_space.region[i].range.size, pa, size == 0U ? 1U : size))
			continue;
		if (nxu_space.region[i].range.size <= best) {
			best = nxu_space.region[i].range.size;
			owner = nxu_space.region[i].owner;
		}
	}
	return owner;
}

void
nxu_s5_ownership(void)
{
	nxu_u32 i;
	nxu_u64 img;

	nxu_space.count = 0U;
	nxu_space.armed = 0U;
	nxu_space.frozen = 0U;

	for (i = 0U; i < nxu_facts.mem_count; i++)
		add_owner(nxu_facts.mem[i].range.base, nxu_facts.mem[i].range.size,
		    NXU_OWNER_NXU);

	img = page_up(nxu_image.size);
	add_owner(nxu_image.base, img, NXU_OWNER_IMAGE);

	if (nxu_facts.dtb_valid && nxu_facts.dtb_size != 0U)
		add_owner(nxu_facts.dtb_base, page_up(nxu_facts.dtb_size),
		    NXU_OWNER_RESERVED);

	add_owner(nxu_facts.gicd.range.base, page_up(nxu_facts.gicd.range.size),
	    NXU_OWNER_MMIO);
	add_owner(nxu_facts.gicr.range.base, page_up(nxu_facts.gicr.range.size),
	    NXU_OWNER_MMIO);
	if (nxu_facts.uart_valid)
		add_owner(nxu_facts.uart.range.base, page_up(nxu_facts.uart.range.size),
		    NXU_OWNER_MMIO);
	if (nxu_facts.pwrc.size != 0U)
		add_owner(nxu_facts.pwrc.base, page_up(nxu_facts.pwrc.size),
		    NXU_OWNER_MMIO);
}

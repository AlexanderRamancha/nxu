/*
 * NXU MAP
 * Layer: foundation space (S6)
 * Owns: which physical ranges are mapped and with which perm
 * Consumes: ownership table, image splits, silicon MMU ops
 * Produces: identity map of image+accepted MMIO; MMU on
 * Hardware: TTBR/TCR/MAIR/SCTLR via silicon
 * Forbidden: WX; default-allow; identity-map all RAM
 * Invariant: after arm, MMU on, unknown inaccessible
 */
#include <nxu/space.h>
#include <nxu/image.h>
#include <nxu/facts.h>
#include <nxu/intake.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/sink.h>
#include <nxu/console.h>
#include <nxu/log.h>
#include <silicon/aarch64/translate/mmu.h>

static nxu_u64
page_up(nxu_u64 n)
{
	return (n + NXU_PAGE_SIZE - 1ULL) & ~(NXU_PAGE_SIZE - 1ULL);
}

static nxu_u64
page_down(nxu_u64 n)
{
	return n & ~(NXU_PAGE_SIZE - 1ULL);
}

void
nxu_s6_mmu(void)
{
	nxu_u64 text_end;
	nxu_u64 ro_end;
	nxu_u64 img_end;
	nxu_u32 i;

	nxu_mmu_init_pool();

	text_end = page_up(nxu_image.text_end);
	ro_end = page_up(nxu_image.rodata_end);
	img_end = page_up(nxu_image.base + nxu_image.size);
	if (text_end <= nxu_image.base)
		text_end = nxu_image.base + NXU_PAGE_SIZE;
	if (ro_end < text_end)
		ro_end = text_end;

	/* Image only. Remainder of RAM stays unmapped (default deny). */
	nxu_mmu_map(nxu_image.base, text_end - nxu_image.base, NXU_MAP_RX);
	if (ro_end > text_end)
		nxu_mmu_map(text_end, ro_end - text_end, NXU_MAP_R);
	if (img_end > ro_end)
		nxu_mmu_map(ro_end, img_end - ro_end, NXU_MAP_RW);

	for (i = 0U; i < nxu_space.count; i++) {
		if (nxu_space.region[i].owner == NXU_OWNER_MMIO) {
			nxu_u64 b = page_down(nxu_space.region[i].range.base);
			nxu_u64 s = page_up(nxu_space.region[i].range.size +
			    (nxu_space.region[i].range.base - b));
			nxu_mmu_map(b, s, NXU_MAP_DEV);
		}
	}

	if (nxu_space_owner(nxu_image.base, NXU_PAGE_SIZE) != NXU_OWNER_IMAGE)
		nxu_fail(NXU_FAIL_INVARIANT, 5U, 0U);

	nxu_mmu_enable(nxu_control_el());
	nxu_space.armed = 1U;
	nxu_chronicle_put(NXU_REC_S6_MMU, nxu_mmu_ttbr(), nxu_control_el());

	if (nxu_facts.uart_valid)
		nxu_sink_bind(nxu_facts.uart.range.base);
	NXU_LOG(NXU_EV_BOOT, NXU_SUB_BOOT, nxu_intake.pc, 0U);
	if (nxu_facts.dtb_valid)
		NXU_LOG(NXU_EV_DTB, NXU_SUB_FACT, nxu_facts.dtb_base, 1U);
	else
		NXU_LOG(NXU_EV_DTB, NXU_SUB_FACT, 0U, 0U);
	NXU_LOG(NXU_EV_MEMORY, NXU_SUB_FACT, nxu_facts.mem[0].range.base, 0U);
	NXU_LOG(NXU_EV_MMU, NXU_SUB_SPACE, nxu_mmu_ttbr(), nxu_control_el());
	nxu_debug("MMU", "identity map of image and accepted MMIO");
	nxu_debug_u64("ttbr", nxu_mmu_ttbr());
	nxu_debug_u64("el", nxu_control_el());
}

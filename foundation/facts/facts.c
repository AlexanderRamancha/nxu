/*
 * NXU MAP
 * Layer: foundation facts (S4)
 * Owns: corroboration and acceptance of load-bearing facts
 * Consumes: observation, nxu_dtb_cand, board candidates
 * Produces: nxu_facts with source mask and grade
 * Hardware: CNTFRQ (observed)
 * Forbidden: board addresses as architecture; skip on mismatch
 * Invariant: disagreeing sources halt; image ⊂ RAM
 */
#include <nxu/facts.h>
#include <nxu/fdt.h>
#include <nxu/board.h>
#include <nxu/intake.h>
#include <nxu/image.h>
#include <nxu/chronicle.h>
#include <nxu/fail.h>
#include <nxu/range.h>
#include <silicon/aarch64/timer/timer.h>

struct nxu_facts nxu_facts;

static int
range_eq(struct nxu_range a, struct nxu_range b)
{
	return a.base == b.base && a.size == b.size;
}

static void
merge_ctl(struct nxu_ctl_fact *out, struct nxu_range dtb, nxu_u32 dtb_ok,
    struct nxu_range board, nxu_u32 board_ok, nxu_u32 kind, nxu_u32 required)
{
	out->kind = kind;
	out->range.base = 0U;
	out->range.size = 0U;
	out->source = 0U;
	out->grade = 0U;

	if (dtb_ok && board_ok) {
		if (!range_eq(dtb, board))
			nxu_fail(NXU_FAIL_FACT_MISMATCH, dtb.base, board.base);
		out->range = dtb;
		out->source = NXU_SRC_DTB | NXU_SRC_BOARD;
		out->grade = NXU_GRADE_CORROBORATED;
		return;
	}
	if (dtb_ok) {
		out->range = dtb;
		out->source = NXU_SRC_DTB;
		out->grade = NXU_GRADE_SINGLE;
		return;
	}
	if (board_ok) {
		out->range = board;
		out->source = NXU_SRC_BOARD;
		out->grade = NXU_GRADE_SINGLE;
		return;
	}
	if (required)
		nxu_fail(NXU_FAIL_FACT_MISMATCH, kind, 0U);
}

void
nxu_s4_facts(void)
{
	nxu_u32 i;
	nxu_u32 have_ram;
	nxu_u64 boot_mpidr;
	nxu_u32 isa_ppi;

	nxu_facts.mem_count = 0U;
	nxu_facts.cpu_count = 0U;
	nxu_facts.uart_valid = 0U;
	nxu_facts.frozen = 0U;
	nxu_facts.dtb_valid = nxu_dtb_cand.valid;
	nxu_facts.dtb_base = nxu_dtb_cand.base;
	nxu_facts.dtb_size = nxu_dtb_cand.size;
	nxu_facts.control_el = nxu_control_el();

	have_ram = 0U;
	if (nxu_dtb_cand.valid && nxu_dtb_cand.mem_count > 0U) {
		for (i = 0U; i < nxu_dtb_cand.mem_count && i < NXU_MEM_FACT_MAX; i++) {
			nxu_facts.mem[i].range = nxu_dtb_cand.mem[i];
			nxu_facts.mem[i].source = NXU_SRC_DTB;
			nxu_facts.mem[i].grade = NXU_GRADE_SINGLE;
			if (nxu_board.present && i < nxu_board.ram_count) {
				if (!range_eq(nxu_dtb_cand.mem[i], nxu_board.ram[i]))
					nxu_fail(NXU_FAIL_FACT_MISMATCH,
					    nxu_dtb_cand.mem[i].base, nxu_board.ram[i].base);
				nxu_facts.mem[i].source = NXU_SRC_DTB | NXU_SRC_BOARD;
				nxu_facts.mem[i].grade = NXU_GRADE_CORROBORATED;
			}
			nxu_facts.mem_count++;
		}
		have_ram = 1U;
	} else if (nxu_board.present && nxu_board.ram_count > 0U) {
		nxu_facts.mem[0].range = nxu_board.ram[0];
		nxu_facts.mem[0].source = NXU_SRC_BOARD;
		nxu_facts.mem[0].grade = NXU_GRADE_SINGLE;
		nxu_facts.mem_count = 1U;
		have_ram = 1U;
	}
	if (!have_ram)
		nxu_fail(NXU_FAIL_NO_RAM, 0U, 0U);
	if (!nxu_range_contains(nxu_facts.mem[0].range.base,
	    nxu_facts.mem[0].range.size, nxu_image.base, nxu_image.size))
		nxu_fail(NXU_FAIL_IMAGE_RAM, nxu_image.base, nxu_facts.mem[0].range.base);

	nxu_chronicle_put(NXU_REC_S4_RAM, nxu_facts.mem[0].range.base,
	    nxu_facts.mem[0].range.size);
	nxu_chronicle_put(NXU_REC_S4_GRADE, nxu_facts.mem[0].grade,
	    nxu_facts.mem[0].source);

	merge_ctl(&nxu_facts.gicd, nxu_dtb_cand.gicd,
	    nxu_dtb_cand.valid && nxu_dtb_cand.gicd.size != 0U,
	    nxu_board.gicd, nxu_board.present && nxu_board.gicd.size != 0U,
	    NXU_CTL_GICD, 1U);
	merge_ctl(&nxu_facts.gicr, nxu_dtb_cand.gicr,
	    nxu_dtb_cand.valid && nxu_dtb_cand.gicr.size != 0U,
	    nxu_board.gicr, nxu_board.present && nxu_board.gicr.size != 0U,
	    NXU_CTL_GICR, 1U);
	nxu_chronicle_put(NXU_REC_S4_GIC, nxu_facts.gicd.range.base,
	    nxu_facts.gicr.range.base);

	merge_ctl(&nxu_facts.uart, nxu_dtb_cand.uart, nxu_dtb_cand.uart_valid,
	    nxu_board.uart, nxu_board.present && nxu_board.uart.size != 0U,
	    NXU_CTL_UART, 0U);
	if (nxu_facts.uart.range.size != 0U)
		nxu_facts.uart_valid = 1U;
	if (nxu_facts.uart_valid)
		nxu_chronicle_put(NXU_REC_S4_UART, nxu_facts.uart.range.base,
		    nxu_facts.uart.grade);

	if (nxu_dtb_cand.psci_conduit != 0U && nxu_board.present &&
	    nxu_board.psci_conduit != 0U &&
	    nxu_dtb_cand.psci_conduit != nxu_board.psci_conduit)
		nxu_fail(NXU_FAIL_FACT_MISMATCH, nxu_dtb_cand.psci_conduit,
		    nxu_board.psci_conduit);
	if (nxu_dtb_cand.psci_conduit != 0U)
		nxu_facts.psci_conduit = nxu_dtb_cand.psci_conduit;
	else
		nxu_facts.psci_conduit = nxu_board.psci_conduit;
	nxu_facts.psci_source = 0U;
	if (nxu_dtb_cand.psci_conduit)
		nxu_facts.psci_source |= NXU_SRC_DTB;
	if (nxu_board.present && nxu_board.psci_conduit)
		nxu_facts.psci_source |= NXU_SRC_BOARD;
	if (nxu_board.present)
		nxu_facts.pwrc = nxu_board.pwrc;
	nxu_chronicle_put(NXU_REC_S4_PSCI, nxu_facts.psci_conduit,
	    nxu_facts.psci_source);

	boot_mpidr = nxu_intake.mpidr & 0xFF00FFFFFFULL;
	nxu_facts.cpu_count = 0U;
	if (nxu_dtb_cand.valid && nxu_dtb_cand.cpu_count > 0U) {
		for (i = 0U; i < nxu_dtb_cand.cpu_count; i++) {
			nxu_u32 src;
			nxu_u32 grade;

			src = NXU_SRC_DTB;
			grade = NXU_GRADE_SINGLE;
			if (nxu_board.present && nxu_board.cpu_count > 0U) {
				nxu_u32 ok = 0U;
				nxu_u32 j;
				for (j = 0U; j < nxu_board.cpu_count; j++) {
					if ((nxu_board.cpu_mpidr[j] & 0xFF00FFFFFFULL) ==
					    (nxu_dtb_cand.cpu_mpidr[i] & 0xFF00FFFFFFULL))
						ok = 1U;
				}
				if (!ok)
					nxu_fail(NXU_FAIL_CPU, nxu_dtb_cand.cpu_mpidr[i], 1U);
				src |= NXU_SRC_BOARD;
				grade = NXU_GRADE_CORROBORATED;
			}
			if (nxu_dtb_cand.cpu_mpidr[i] == boot_mpidr) {
				src |= NXU_SRC_OBS;
				grade = NXU_GRADE_CORROBORATED;
			}
			nxu_facts.cpu[nxu_facts.cpu_count].mpidr =
			    nxu_dtb_cand.cpu_mpidr[i] & 0xFF00FFFFFFULL;
			nxu_facts.cpu[nxu_facts.cpu_count].state = NXU_CPU_PRESENT;
			nxu_facts.cpu[nxu_facts.cpu_count].source = src;
			nxu_facts.cpu[nxu_facts.cpu_count].grade = grade;
			nxu_facts.cpu_count++;
			nxu_chronicle_put(NXU_REC_S4_CPU,
			    nxu_facts.cpu[nxu_facts.cpu_count - 1U].mpidr, grade);
		}
	} else if (nxu_board.present && nxu_board.cpu_count > 0U) {
		if (nxu_board.cpu_count > NXU_CPU_BOUND)
			nxu_fail(NXU_FAIL_CPU, nxu_board.cpu_count, 5U);
		for (i = 0U; i < nxu_board.cpu_count; i++) {
			nxu_u64 m;
			nxu_u32 src;
			nxu_u32 grade;

			m = nxu_board.cpu_mpidr[i] & 0xFF00FFFFFFULL;
			src = NXU_SRC_BOARD;
			grade = NXU_GRADE_SINGLE;
			if (m == boot_mpidr) {
				src |= NXU_SRC_OBS;
				grade = NXU_GRADE_CORROBORATED;
			}
			nxu_facts.cpu[nxu_facts.cpu_count].mpidr = m;
			nxu_facts.cpu[nxu_facts.cpu_count].state = NXU_CPU_PRESENT;
			nxu_facts.cpu[nxu_facts.cpu_count].source = src;
			nxu_facts.cpu[nxu_facts.cpu_count].grade = grade;
			nxu_facts.cpu_count++;
			nxu_chronicle_put(NXU_REC_S4_CPU, m, grade);
		}
	} else {
		nxu_facts.cpu[0].mpidr = boot_mpidr;
		nxu_facts.cpu[0].state = NXU_CPU_PRESENT;
		nxu_facts.cpu[0].source = NXU_SRC_OBS;
		nxu_facts.cpu[0].grade = NXU_GRADE_OBSERVED;
		nxu_facts.cpu_count = 1U;
		nxu_chronicle_put(NXU_REC_S4_CPU, boot_mpidr, NXU_GRADE_OBSERVED);
	}
	{
		nxu_u32 found = 0U;
		for (i = 0U; i < nxu_facts.cpu_count; i++) {
			if (nxu_facts.cpu[i].mpidr == boot_mpidr)
				found = 1U;
		}
		if (!found)
			nxu_fail(NXU_FAIL_CPU, boot_mpidr, 2U);
	}

	/* CNTP is the EL1 physical timer. GICv3 recommends INTID 30.
	 * A DTB that names another PPI is a mismatch, not a second timer. */
	isa_ppi = 30U;
	if (nxu_dtb_cand.timer_ppi != 0U && nxu_dtb_cand.timer_ppi != isa_ppi)
		nxu_fail(NXU_FAIL_FACT_MISMATCH, nxu_dtb_cand.timer_ppi, isa_ppi);
	nxu_facts.timer_ppi = isa_ppi;
	nxu_facts.timer_freq = nxu_timer_cntfrq();
	if (nxu_facts.timer_freq == 0U && nxu_board.present)
		nxu_facts.timer_freq = nxu_board.timer_freq;
	if (nxu_facts.timer_freq == 0U)
		nxu_fail(NXU_FAIL_TIMER, 0U, 0U);
	nxu_chronicle_put(NXU_REC_S4_TIMER, nxu_facts.timer_freq, nxu_facts.timer_ppi);
}

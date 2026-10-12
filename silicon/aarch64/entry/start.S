/*
 * NXU MAP
 * Layer: silicon entry
 * Owns: DAIF mask, NXU stack, residue capture, halt VBAR, secondary SP
 * Consumes: hardware registers, image stack
 * Produces: nxu_intake; SP = NXU; DAIF masked
 * Hardware: CurrentEL, MPIDR, SCTLR, TTBR, TCR, VBAR, DAIF
 * Forbidden: incoming SP as NXU SP; firmware tables; board addresses
 * Invariant: steady execution is EL1; incoming SP unused
 */
/*
 * Location-observed entry. Incoming SP is not NXU's stack.
 * Image header is an intake shape, not a trust model.
 */

	.section .text.header, "ax"
	.align  2
	.global _start
_start:
	b	nxu_entry
	.word	0
	.quad	0
	.quad	_image_end - _start
	.quad	0xA
	.quad	0
	.quad	0
	.quad	0
	.word	0x644d5241
	.word	0

	.section .text
	.align  2
nxu_entry:
	msr	daifset, #0xf
	/* A released secondary finds its context in the gate. The boot
	 * CPU's slots are empty, so it falls through.
	 */
	mrs	x9, mpidr_el1
	and	x10, x9, #0xffffff
	ubfx	x8, x9, #32, #8
	orr	x10, x10, x8, lsl #24
	adrp	x11, nxu_cpu_gate
	add	x11, x11, :lo12:nxu_cpu_gate
	mov	x12, #8
1:
	ldr	x13, [x11]
	cmp	x13, x10
	b.ne	2f
	ldr	w14, [x11, #16]
	cbz	w14, 2f
	ldr	x0, [x11, #8]
	b	nxu_secondary_entry
2:
	add	x11, x11, #32
	subs	x12, x12, #1
	b.ne	1b

	/* Entry EL is not the kernel EL. x21 is the EL1 continuation. */
	mrs	x24, CurrentEL
	cmp	x24, #12
	b.ne	10f
	adr	x21, el1_arrive
	b	el3_to_el1
10:
	cmp	x24, #8
	b.ne	el1_arrive
	adr	x21, el1_arrive
	b	el2_to_el1

	/* Fail-closed EL3: NXU owns stack and vectors before any drop,
	 * so an EL3 exception cannot fetch the firmware ROM at 0x0.
	 * x21 is the EL1 address to enter. x0–x3 and x19 are kept.
	 */
el3_to_el1:
	adrp	x10, nxu_boot_stack_top
	add	x10, x10, :lo12:nxu_boot_stack_top
	mov	sp, x10
	adr	x10, nxu_vec_diag
	msr	vbar_el3, x10
	isb
	adrp	x13, nxu_icc_sre3_obs
	add	x13, x13, :lo12:nxu_icc_sre3_obs
	mov	x8, #1
	str	x8, [x13, #24]		/* stage: vectors up */
	mov	x9, #0x501
	msr	scr_el3, x9
	mov	x8, #2
	str	x8, [x13, #24]		/* stage: scr */
	mov	x9, #0xf		/* SRE|DFB|DIB|Enable: EL1 may use ICC regs */
	msr	S3_6_C12_C12_5, x9	/* ICC_SRE_EL3 */
	mov	x8, #3
	str	x8, [x13, #24]		/* stage: sre_el3 */
	msr	S3_4_C12_C9_5, x9	/* ICC_SRE_EL2 */
	mov	x8, #4
	str	x8, [x13, #24]		/* stage: sre_el2@el3 */
	msr	S3_0_C12_C12_5, x9	/* ICC_SRE_EL1 */
	mov	x8, #5
	str	x8, [x13, #24]		/* stage: sre_el1@el3 */
	mov	x9, #7
	msr	icc_igrpen1_el3, x9	/* EnableGrp1NS|EnableGrp1S; bit2 is RES0 */
	mov	x8, #6
	str	x8, [x13, #24]		/* stage: grppen1_el3 */
	isb
	mrs	x11, S3_6_C12_C12_5
	str	x11, [x13]
	mrs	x11, S3_4_C12_C9_5
	str	x11, [x13, #8]
	mrs	x11, S3_0_C12_C12_5
	str	x11, [x13, #16]
	mrs	x11, icc_igrpen1_el3
	str	x11, [x13, #56]		/* ICC_IGRPEN1_EL3 readback */
	mov	x8, #0x1111
	str	x8, [x13, #24]		/* stage: block complete */
	adr	x10, nxu_vec_full
	msr	vbar_el3, x10		/* restore fail-closed runtime vectors */
	/* HCR_EL2.RW: EL1 is AArch64. TGE stays clear so EL1 exists. */
	mov	x9, #0x80000000
	msr	hcr_el2, x9
	/* EL1PCTEN|EL1PCEN: EL1 owns CNTPCT and CNTP, not a trap to EL2. */
	mov	x9, #3
	msr	cnthctl_el2, x9
	adr	x10, nxu_vec_halt
	msr	vbar_el2, x10
	isb
	/* Group registers are Secure. Program the boot CPU's CNTP PPI and
	 * the IPI SGI as Non-secure Group 1 before the NS drop. EL1 cannot
	 * change that assignment when two security states exist.
	 */
	adrp	x14, nxu_board
	add	x14, x14, :lo12:nxu_board
	ldr	w4, [x14]
	cbz	w4, 60f
	ldr	x16, [x14, #0x98]
	ldr	x17, [x14, #0xa0]
	cbz	x16, 60f
	cbz	x17, 60f
	mrs	x12, mpidr_el1
	and	x9, x12, #0xffffff
	ubfx	x8, x12, #32, #8
	orr	x9, x9, x8, lsl #24
	mov	x4, x16
	mov	x5, xzr
40:
	ldr	x6, [x4, #8]
	lsr	x7, x6, #32
	cmp	x7, x9
	b.eq	41f
	tbnz	x6, #4, 60f
	add	x5, x5, #1
	lsr	x8, x17, #17
	cmp	x5, x8
	b.hi	60f
	add	x4, x4, #0x20000
	b	40b
41:
	ldr	w8, [x4, #0x14]
	bic	w8, w8, #2
	str	w8, [x4, #0x14]
	mov	w5, #0x2710
42:
	ldr	w8, [x4, #0x14]
	tbz	w8, #2, 43f
	subs	w5, w5, #1
	b.ne	42b
43:
	add	x4, x4, #0x10000
	mov	w7, #1
	lsl	w7, w7, #30
	orr	w7, w7, #1
	ldr	w8, [x4, #0x80]
	orr	w8, w8, w7
	str	w8, [x4, #0x80]
	ldr	w8, [x4, #0xd00]
	bic	w8, w8, w7
	str	w8, [x4, #0xd00]
	dsb	sy
60:
	/* CNTFRQ_EL0 is writable only at the highest EL. A zero read at EL1
	 * turns a later deadline into a compare of "now".
	 */
	mrs	x9, cntfrq_el0
	cbnz	x9, 61f
	ldr	w9, [x14, #0x104]
	cbz	w9, 61f
	msr	cntfrq_el0, x9
	isb
61:
	mov	x9, #0x3c5		/* EL1h, DAIF masked */
	msr	spsr_el3, x9
	msr	elr_el3, x21
	isb
	eret

	/* Same landing from a platform that reset at EL2. */
el2_to_el1:
	adrp	x10, nxu_boot_stack_top
	add	x10, x10, :lo12:nxu_boot_stack_top
	mov	sp, x10
	adr	x10, nxu_vec_halt
	msr	vbar_el2, x10
	mov	x9, #0x80000000
	msr	hcr_el2, x9
	mov	x9, #3
	msr	cnthctl_el2, x9
	mov	x9, #0xf
	msr	S3_4_C12_C9_5, x9	/* ICC_SRE_EL2: SRE and Enable */
	isb
	mov	x9, #0x3c5
	msr	spsr_el2, x9
	msr	elr_el2, x21
	isb
	eret

el1_arrive:
	mov	x19, x0
	mov	x20, x1
	mov	x21, x2
	mov	x22, x3
	mov	x23, sp
	mrs	x24, CurrentEL
	mrs	x25, mpidr_el1
	mrs	x28, daif

	mrs	x26, sctlr_el1
	mrs	x12, ttbr0_el1
	mrs	x13, tcr_el1
	mrs	x14, vbar_el1
	msr	tpidr_el1, xzr
	adr	x27, _start

	adrp	x8, __bss_start
	add	x8, x8, :lo12:__bss_start
	adrp	x9, __bss_end
	add	x9, x9, :lo12:__bss_end
	mov	x10, xzr
3:
	cmp	x8, x9
	b.hs	4f
	str	x10, [x8], #8
	b	3b
4:
	adrp	x11, nxu_boot_stack_top
	add	x11, x11, :lo12:nxu_boot_stack_top
	mov	sp, x11

	adrp	x8, nxu_intake
	add	x8, x8, :lo12:nxu_intake
	str	x19, [x8, #0]
	str	x20, [x8, #8]
	str	x21, [x8, #16]
	str	x22, [x8, #24]
	str	x27, [x8, #32]
	str	x23, [x8, #40]
	str	x24, [x8, #48]
	str	x25, [x8, #56]
	str	x26, [x8, #64]
	str	x28, [x8, #72]
	str	x11, [x8, #80]
	str	x12, [x8, #88]
	str	x13, [x8, #96]
	str	x14, [x8, #104]

	adr	x0, nxu_vec_halt
	msr	vbar_el1, x0
	isb

	bl	nxu_foundation
	bl	nxu_halt
7:
	wfi
	b	7b

	.section .text.vecdiag, "ax"
	.align  11
nxu_vec_diag:
	.rept   4			/* EL3 SP_EL0 block: unreachable */
	b	nxu_vec_diag
	.space	0x7c
	.endr
					/* EL3 SP_ELx Sync (0x200): record+skip+eret */
	adrp	x24, nxu_icc_sre3_obs
	add	x24, x24, :lo12:nxu_icc_sre3_obs
	mrs	x25, elr_el3
	mrs	x26, esr_el3
	str	x25, [x24, #40]		/* last trap ELR */
	str	x26, [x24, #48]		/* last trap ESR */
	ldr	x27, [x24, #32]		/* trap count */
	add	x27, x27, #1
	str	x27, [x24, #32]
	add	x25, x25, #4
	msr	elr_el3, x25
	eret
	.space	0x80-(12*4)
	.rept   11			/* rest: spin fail-closed */
	b	nxu_vec_diag
	.space	0x7c
	.endr

	.section .data
	.align 3
	.global nxu_icc_sre3_obs
nxu_icc_sre3_obs:
	.space 64

	.align  2
	.global nxu_secondary_entry
nxu_secondary_entry:
	msr	daifset, #0xf
	mov	x19, x0
	mrs	x1, CurrentEL
	cmp	x1, #12
	b.ne	8f
	adr	x21, sec_el1
	b	el3_to_el1
8:
	cmp	x1, #8
	b.ne	sec_el1
	adr	x21, sec_el1
	b	el2_to_el1
sec_el1:
	msr	tpidr_el1, x19
	adrp	x2, nxu_cpu_stack
	add	x2, x2, :lo12:nxu_cpu_stack
	mov	x3, #0x10000
	madd	x2, x19, x3, x2
	add	x2, x2, x3
	mov	sp, x2
	mov	x0, x19
	bl	nxu_secondary
10:
	wfi
	b	10b

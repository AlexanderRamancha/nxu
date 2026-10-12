/*
 * NXU MAP
 * Layer: silicon exception
 * Owns: halt VBAR and full VBAR; IRQ save/restore; fatal sync/fiq/serror
 * Consumes: none (branches to foundation C)
 * Produces: call nxu_exception / nxu_irq_c
 * Hardware: VBAR slots at 0x80
 * Forbidden: handler tables; continue after unexpected class
 * Invariant: vectors are NXU's before unmask
 */
	.section .text.vechalt, "ax"
	.align  11
	.global nxu_vec_halt
nxu_vec_halt:
	.rept   16
	b	nxu_vec_halt_entry
	.space  0x7c
	.endr

	.align  2
nxu_vec_halt_entry:
	msr	daifset, #0xf
1:
	wfi
	b	1b

	.section .text.vec, "ax"
	.align  11
	.global nxu_vec_full
nxu_vec_full:
	b	nxu_vec_sync
	.space  0x7c
	b	nxu_vec_irq
	.space  0x7c
	b	nxu_vec_fiq
	.space  0x7c
	b	nxu_vec_serror
	.space  0x7c
	b	nxu_vec_sync
	.space  0x7c
	b	nxu_vec_irq
	.space  0x7c
	b	nxu_vec_fiq
	.space  0x7c
	b	nxu_vec_serror
	.space  0x7c
	b	nxu_vec_lower_sync
	.space  0x7c
	b	nxu_vec_lower_irq
	.space  0x7c
	b	nxu_vec_fiq
	.space  0x7c
	b	nxu_vec_serror
	.space  0x7c
	b	nxu_vec_sync
	.space  0x7c
	b	nxu_vec_irq
	.space  0x7c
	b	nxu_vec_fiq
	.space  0x7c
	b	nxu_vec_serror
	.space  0x7c

	.align  2
nxu_vec_sync:
	stp	x0, x1, [sp, #-16]!
	mov	x0, #1
	b	nxu_vec_fatal

	.align  2
nxu_vec_fiq:
	stp	x0, x1, [sp, #-16]!
	mov	x0, #3
	b	nxu_vec_fatal

	.align  2
nxu_vec_serror:
	stp	x0, x1, [sp, #-16]!
	mov	x0, #4
	b	nxu_vec_fatal

	.align  2
nxu_vec_fatal:
	bl	nxu_exception
2:
	msr	daifset, #0xf
	wfi
	b	2b

	.align  2
nxu_vec_irq:
	sub	sp, sp, #256
	stp	x0, x1, [sp, #0]
	stp	x2, x3, [sp, #16]
	stp	x4, x5, [sp, #32]
	stp	x6, x7, [sp, #48]
	stp	x8, x9, [sp, #64]
	stp	x10, x11, [sp, #80]
	stp	x12, x13, [sp, #96]
	stp	x14, x15, [sp, #112]
	stp	x16, x17, [sp, #128]
	stp	x18, x19, [sp, #144]
	stp	x20, x21, [sp, #160]
	stp	x22, x23, [sp, #176]
	stp	x24, x25, [sp, #192]
	stp	x26, x27, [sp, #208]
	stp	x28, x29, [sp, #224]
	str	x30, [sp, #240]
	bl	nxu_irq_c
	ldr	x30, [sp, #240]
	ldp	x28, x29, [sp, #224]
	ldp	x26, x27, [sp, #208]
	ldp	x24, x25, [sp, #192]
	ldp	x22, x23, [sp, #176]
	ldp	x20, x21, [sp, #160]
	ldp	x18, x19, [sp, #144]
	ldp	x16, x17, [sp, #128]
	ldp	x14, x15, [sp, #112]
	ldp	x12, x13, [sp, #96]
	ldp	x10, x11, [sp, #80]
	ldp	x8, x9, [sp, #64]
	ldp	x6, x7, [sp, #48]
	ldp	x4, x5, [sp, #32]
	ldp	x2, x3, [sp, #16]
	ldp	x0, x1, [sp, #0]
	add	sp, sp, #256
	eret

	.align	2
nxu_vec_lower_sync:
	stp	x29, x30, [sp, #-16]!
	mov	x2, x0
	mrs	x0, esr_el1
	mrs	x1, elr_el1
	bl	nxu_room_exception
	ldp	x29, x30, [sp], #16
	cbnz	x0, nxu_room_leave
	mov	x4, #0x340
	msr	spsr_el1, x4
	eret

	.align	2
nxu_vec_lower_irq:
	stp	x29, x30, [sp, #-16]!
	bl	nxu_room_irq
	ldp	x29, x30, [sp], #16
nxu_room_leave:
	adr	x4, nxu_room_return
	msr	elr_el1, x4
	mov	x4, #0x3c5
	msr	spsr_el1, x4
	eret

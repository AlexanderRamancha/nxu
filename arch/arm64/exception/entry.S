.section .vectors, "ax"
.align 11
.global vectors

vectors:
    b    sync_current_sp0
    .org 0x080
    b    irq_current_sp0
    .org 0x100
    b    fiq_current_sp0
    .org 0x180
    b    serror_current_sp0

    .org 0x200
    b    sync_current_spx
    .org 0x280
    b    irq_current_spx
    .org 0x300
    b    fiq_current_spx
    .org 0x380
    b    serror_current_spx

    .org 0x400
    b    sync_lower_aarch64
    .org 0x480
    b    irq_lower_aarch64
    .org 0x500
    b    fiq_lower_aarch64
    .org 0x580
    b    serror_lower_aarch64

    .org 0x600
    b    sync_lower_aarch32
    .org 0x680
    b    irq_lower_aarch32
    .org 0x700
    b    fiq_lower_aarch32
    .org 0x780
    b    serror_lower_aarch32

.section .text
.align 4

sync_current_spx:
    stp     x29, x30, [sp, #-16]!
    stp     x0, x1,   [sp, #-16]!
    bl      handle_sync_exception
    ldp     x0, x1,   [sp], #16
    ldp     x29, x30, [sp], #16
    eret

sync_current_sp0:
irq_current_spx:
irq_current_sp0:
fiq_current_spx:
fiq_current_sp0:
serror_current_spx:
serror_current_sp0:
sync_lower_aarch64:
irq_lower_aarch64:
fiq_lower_aarch64:
serror_lower_aarch64:
sync_lower_aarch32:
irq_lower_aarch32:
fiq_lower_aarch32:
serror_lower_aarch32:
    wfi
    b       .

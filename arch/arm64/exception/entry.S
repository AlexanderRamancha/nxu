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

/*
 * Architecture map
 *
 * IRQ from EL1
 *     |
 *     v
 * vector
 *     |
 *     +-- save GPRs
 *     +-- save ELR_EL1 / SPSR_EL1
 *     |
 *     v
 * nxu_irq_handler()
 *     |
 *     +-- IRQ may be re-enabled
 *     +-- higher-priority IRQ may nest
 *     |
 *     v
 * restore saved exception context
 *     |
 *     v
 * eret
 *
 * Each exception frame owns its saved ELR/SPSR.
 * This makes nested IRQ return independent.
 */

.equ NXU_IRQ_FRAME_SIZE, 288

.equ NXU_X0,   0
.equ NXU_X2,   16
.equ NXU_X4,   32
.equ NXU_X6,   48
.equ NXU_X8,   64
.equ NXU_X10,  80
.equ NXU_X12,  96
.equ NXU_X14, 112
.equ NXU_X16, 128
.equ NXU_X18, 144
.equ NXU_X20, 160
.equ NXU_X22, 176
.equ NXU_X24, 192
.equ NXU_X26, 208
.equ NXU_X28, 224
.equ NXU_X30, 240
.equ NXU_ELR, 256
.equ NXU_SPSR, 264

sync_current_spx:
    stp x29, x30, [sp, #-16]!
    bl handle_sync_exception
    ldp x29, x30, [sp], #16
    eret

sync_current_sp0:
    b sync_current_spx

irq_current_spx:
    sub sp, sp, #NXU_IRQ_FRAME_SIZE

    stp x0,  x1,  [sp, #NXU_X0]
    stp x2,  x3,  [sp, #NXU_X2]
    stp x4,  x5,  [sp, #NXU_X4]
    stp x6,  x7,  [sp, #NXU_X6]
    stp x8,  x9,  [sp, #NXU_X8]
    stp x10, x11, [sp, #NXU_X10]
    stp x12, x13, [sp, #NXU_X12]
    stp x14, x15, [sp, #NXU_X14]
    stp x16, x17, [sp, #NXU_X16]
    stp x18, x19, [sp, #NXU_X18]
    stp x20, x21, [sp, #NXU_X20]
    stp x22, x23, [sp, #NXU_X22]
    stp x24, x25, [sp, #NXU_X24]
    stp x26, x27, [sp, #NXU_X26]
    stp x28, x29, [sp, #NXU_X28]
    str x30,      [sp, #NXU_X30]

    mrs x0, elr_el1
    mrs x1, spsr_el1
    stp x0, x1, [sp, #NXU_ELR]

    bl nxu_irq_handler

    ldp x0, x1, [sp, #NXU_ELR]
    msr elr_el1, x0
    msr spsr_el1, x1
    isb

    ldp x0,  x1,  [sp, #NXU_X0]
    ldp x2,  x3,  [sp, #NXU_X2]
    ldp x4,  x5,  [sp, #NXU_X4]
    ldp x6,  x7,  [sp, #NXU_X6]
    ldp x8,  x9,  [sp, #NXU_X8]
    ldp x10, x11, [sp, #NXU_X10]
    ldp x12, x13, [sp, #NXU_X12]
    ldp x14, x15, [sp, #NXU_X14]
    ldp x16, x17, [sp, #NXU_X16]
    ldp x18, x19, [sp, #NXU_X18]
    ldp x20, x21, [sp, #NXU_X20]
    ldp x22, x23, [sp, #NXU_X22]
    ldp x24, x25, [sp, #NXU_X24]
    ldp x26, x27, [sp, #NXU_X26]
    ldp x28, x29, [sp, #NXU_X28]
    ldr x30,      [sp, #NXU_X30]

    add sp, sp, #NXU_IRQ_FRAME_SIZE
    eret

irq_current_sp0:
    b irq_current_spx

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
    b .

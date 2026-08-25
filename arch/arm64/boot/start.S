.section .text.boot
.global _start
.global nxu_secondary_entry

.extern kernel_main
.extern nxu_secondary_cpu_main
.extern vectors

_start:
    msr     daifset, #0xf

    mrs     x0, mpidr_el1
    and     x0, x0, #0xff
    cbnz    x0, hang

    ldr     x1, =__stack_top
    mov     sp, x1

    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
    sub     x2, x2, x1
    cbz     x2, 1f

0:
    str     xzr, [x1], #8
    subs    x2, x2, #8
    b.hi    0b

1:
    ldr     x0, =vectors
    msr     vbar_el1, x0
    isb

    bl      kernel_main

hang:
    wfe
    b       hang

nxu_secondary_entry:
    mov     x19, x0

    ldr     x1, =__secondary_stack_cpu1_top
    mov     sp, x1

    ldr     x1, =vectors
    msr     vbar_el1, x1
    isb

    mov     x0, x19
    bl      nxu_secondary_cpu_main

secondary_idle:
    wfi
    b       secondary_idle
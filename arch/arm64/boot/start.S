.section .text.boot

.global _start
.global nxu_secondary_entry

.extern nxu_kernel_main
.extern nxu_secondary_cpu_main
.extern nxu_boot_info
.extern vectors

.extern __stack_top
.extern __bss_start
.extern __bss_end
.extern __secondary_stack_cpu1_top

_start:
    msr     daifset, #0xf

    mov x20, x0

    mrs     x0, mpidr_el1

    ubfx    x1, x0, #0, #8
    ubfx    x2, x0, #8, #8
    ubfx    x3, x0, #16, #8
    ubfx    x4, x0, #32, #8

    lsl     x4, x4, #24
    lsl     x3, x3, #16
    lsl     x2, x2, #8

    orr     x19, x4, x3
    orr     x19, x19, x2
    orr     x19, x19, x1

    ldr     x5, =__stack_top
    mov     sp, x5

    ldr     x5, =__bss_start
    ldr     x6, =__bss_end

    mov     x7, xzr

1:
    cmp     x5, x6
    b.hs    2f
    str     x7, [x5], #8
    b       1b

2:
    ldr     x5, =vectors
    msr     vbar_el1, x5
    isb

    adrp    x1, nxu_boot_info
    add     x1, x1, :lo12:nxu_boot_info

    str     x19, [x1, #0]
    str     x20, [x1, #8]
    str     wzr, [x1, #16]

    mov     x0, x1
    bl      nxu_kernel_main

3:
    wfi
    b       3b


nxu_secondary_entry:
    msr     daifset, #0xf

    mov     x19, x0

    ldr     x1, =__secondary_stack_cpu1_top
    mov     sp, x1

    ldr     x1, =vectors
    msr     vbar_el1, x1
    isb

    mov     x0, x19
    bl      nxu_secondary_cpu_main

4:
    wfi
    b       4b
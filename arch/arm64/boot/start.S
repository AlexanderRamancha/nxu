.section .text.boot
.global _start

_start:
    mrs     x0, CurrentEL
    cmp     x0, #0b0100
    b.ne    hang

    ldr     x0, =__stack_top
    mov     sp, x0

    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    sub     x1, x1, x0
    cbz     x1, 1f
0:
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.hi    0b
1:
    bl      kernel_main

hang:
    wfi
    b       hang

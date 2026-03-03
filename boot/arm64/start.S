// boot/arm64/start.S
// NXU kernel entry point (AArch64 bare-metal)

.section .text.boot
.global _start

_start:
    // Check EL1 (QEMU virt enters here)
    mrs     x0, CurrentEL
    cmp     x0, #0b0100                     // EL1
    b.ne    hang

    // Set up stack pointer
    ldr     x0, =__stack_top
    mov     sp, x0

    // Zero .bss section
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
    sub     x1, x1, x0
    cbz     x1, 1f

0:
    str     xzr, [x0], #8
    subs    x1, x1, #8
    b.hi    0b

1:
    // Call C entry point
    bl      kernel_main

hang:
    wfi
    b       hang

// BSS section (linker will place symbols here)
.section .bss
.align 8
__bss_start:
    .space  0x20000                         // 128 KiB BSS (adjust as needed)
__bss_end:

// Stack section (grows down from top)
.section .stack
.align 8
__stack_bottom:
    .space  0x20000                         // 128 KiB stack
__stack_top:
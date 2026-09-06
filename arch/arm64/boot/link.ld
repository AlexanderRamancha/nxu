/*
 * Architecture map
 *
 *   load address 0x40080000
 *         |
 *         +-- .text (boot + vectors + code)
 *         +-- .rodata
 *         +-- .data
 *         +-- .bss
 *         +-- .stack (primary + secondary CPU1)
 *
 * Stacks are fixed size and NOLOAD.
 * Entry point is _start.
 */

ENTRY(_start)

SECTIONS {
    . = 0x40080000;

    .text : ALIGN(4096) {
        *(.text.boot)
        *(.text .text.*)
        *(.vectors)
    } = 0

    .rodata : ALIGN(4096) {
        *(.rodata .rodata.*)
    }

    .data : ALIGN(4096) {
        *(.data .data.*)
    }

    .bss : ALIGN(4096) {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    .stack (NOLOAD) : ALIGN(16) {
    __stack_bottom = .;

    /*
     * Primary CPU stack.
     */

    . += 0x20000;
    __stack_top = .;

    /*
     * Secondary CPU stacks.
     *
     * Each CPU receives its own 128 KiB stack.
     */
     
    __secondary_stack_cpu1_bottom = .;
    . += 0x20000;
    __secondary_stack_cpu1_top = .;

    __secondary_stack_end = .;
}
    /DISCARD/ : { *(.comment) *(.eh_frame) }
}

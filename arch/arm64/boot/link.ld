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
        . += 0x20000;
        __stack_top = .;
    }

    /DISCARD/ : { *(.comment) *(.eh_frame) }
}

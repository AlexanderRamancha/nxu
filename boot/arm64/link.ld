ENTRY(_start)

SECTIONS {
    . = 0x40080000;                 /* Typical qemu -M virt load address */

    .text : ALIGN(4096) {
        *(.text.boot)
        *(.text .text.*)
    } =0

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

    /DISCARD/ : { *(.comment) *(.eh_frame) }
}
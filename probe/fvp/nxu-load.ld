/*
 * FVP DRAM link. Probe-only. Foundation image remains VA 0.
 * AEMvA RVBAR + -a need p_vaddr == 0x80000000.
 */
ENTRY(_start)
SECTIONS {
	. = 0x80000000;
	.text : ALIGN(16) {
		KEEP(*(.text.header))
		*(.text .text.*)
		. = ALIGN(4096);
		__text_end = .;
	}
	.rodata : ALIGN(4096) {
		*(.rodata .rodata.*)
		. = ALIGN(4096);
		__rodata_end = .;
	}
	.data : ALIGN(16) {
		*(.data .data.*)
		. = ALIGN(16);
		__data_end = .;
	}
	.bss : ALIGN(4096) {
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(16);
		nxu_boot_stack = .;
		. += 0x10000;
		nxu_boot_stack_top = .;
		. = ALIGN(4096);
		__bss_end = .;
	}
	_image_end = .;
	/DISCARD/ : {
		*(.comment)
		*(.eh_frame)
		*(.eh_frame_hdr)
		*(.note*)
		*(.ARM.attributes)
		*(.got)
		*(.got.*)
		*(.toc)
	}
}

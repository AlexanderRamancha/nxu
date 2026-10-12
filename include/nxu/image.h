/*
 * NXU MAP
 * Layer: image (type)
 * Owns: image footprint symbols
 * Forbidden: board load address
 * Invariant: PC-relative
 */
#ifndef NXU_IMAGE_H
#define NXU_IMAGE_H

#include <nxu/types.h>

struct nxu_image {
	nxu_u64 base;
	nxu_u64 size;
	nxu_u64 text_end;
	nxu_u64 rodata_end;
	nxu_u64 data_end;
};

extern struct nxu_image nxu_image;

nxu_u64 nxu_pa_of(const void *link);
void *nxu_runtime(const void *link);

extern char _start[];
extern char _image_end[];
extern char __text_end[];
extern char __rodata_end[];
extern char __data_end[];
extern char __bss_start[];
extern char __bss_end[];
extern char nxu_boot_stack[];
extern char nxu_boot_stack_top[];

#endif

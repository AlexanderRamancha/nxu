/*
 * NXU MAP
 * Layer: image
 * Owns: runtime image descriptor and PC-relative PA of linker symbols
 * Consumes: linker symbols, intake PC
 * Produces: nxu_image, nxu_pa_of
 * Hardware: none
 * Forbidden: board load address as architecture
 * Invariant: footprint is PC-relative
 */
#include <nxu/image.h>

struct nxu_image nxu_image;

nxu_u64
nxu_pa_of(const void *link)
{
	return (nxu_u64)(nxu_uptr)link;
}

void *
nxu_runtime(const void *link)
{
	return (void *)(nxu_uptr)nxu_pa_of(link);
}

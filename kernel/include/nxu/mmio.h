#ifndef NXU_MMIO_H
#define NXU_MMIO_H

#include <nxu/types.h>

nxu_u8  mmio_read8(nxu_uptr address);
nxu_u16 mmio_read16(nxu_uptr address);
nxu_u32 mmio_read32(nxu_uptr address);
nxu_u64 mmio_read64(nxu_uptr address);

void mmio_write8(nxu_uptr address, nxu_u8 value);
void mmio_write16(nxu_uptr address, nxu_u16 value);
void mmio_write32(nxu_uptr address, nxu_u32 value);
void mmio_write64(nxu_uptr address, nxu_u64 value);

#endif
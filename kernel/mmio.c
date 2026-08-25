#include <nxu/mmio.h>

nxu_u8 mmio_read8(nxu_uptr address)
{
    return *(volatile nxu_u8 *)address;
}

nxu_u16 mmio_read16(nxu_uptr address)
{
    return *(volatile nxu_u16 *)address;
}

nxu_u32 mmio_read32(nxu_uptr address)
{
    return *(volatile nxu_u32 *)address;
}

nxu_u64 mmio_read64(nxu_uptr address)
{
    nxu_u64 low;
    nxu_u64 high;
    nxu_u64 value;

    low = (nxu_u64)mmio_read32(address);
    high = (nxu_u64)mmio_read32(address + 4U);

    value = low | (high << 32);

    return value;
}
void mmio_write8(nxu_uptr address, nxu_u8 value)
{
    *(volatile nxu_u8 *)address = value;
}

void mmio_write16(nxu_uptr address, nxu_u16 value)
{
    *(volatile nxu_u16 *)address = value;
}

void mmio_write32(nxu_uptr address, nxu_u32 value)
{
    *(volatile nxu_u32 *)address = value;
}

void mmio_write64(nxu_uptr address, nxu_u64 value)
{
    *(volatile nxu_u64 *)address = value;
}
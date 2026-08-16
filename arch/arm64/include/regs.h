#ifndef ARCH_ARM64_REGS_H
#define ARCH_ARM64_REGS_H

typedef unsigned long u64;

static inline u64 read_esr_el1(void)
{
    u64 val;
    asm volatile("mrs %0, ESR_EL1" : "=r"(val));
    return val;
}

#endif

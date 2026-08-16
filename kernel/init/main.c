#include "uart/pl011.h"
#include "exceptions.h"

extern void *vectors;

void kernel_main(void)
{
    uart_init();

    asm volatile("msr VBAR_EL1, %0" : : "r"(&vectors) : "memory");
    asm volatile("dsb sy; isb" ::: "memory");

    uart_puts("NXU kernel initialized\r\n");
    uart_puts("Exception vectors installed successfully\r\n");
    uart_puts("System ready.\r\n");

    while (1) {
        asm volatile("wfi" ::: "memory");
    }
}

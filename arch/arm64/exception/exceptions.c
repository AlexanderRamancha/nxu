#include "uart/pl011.h"
#include "exceptions.h"

static const char* get_exception_class_name(unsigned long ec)
{
    switch (ec) {
        case 0x00: return "Unknown reason";
        case 0x01: return "WFx trap";
        case 0x15: return "SVC from lower EL";
        case 0x20: return "Instruction abort (lower EL)";
        case 0x21: return "Instruction abort (current EL)";
        case 0x24: return "Data abort (current EL)";
        case 0x25: return "Data abort (lower EL)";
        default:   return "Unknown exception class";
    }
}

void handle_sync_exception(void)
{
    unsigned long esr, far, elr, spsr;

    asm volatile("mrs %0, ESR_EL1"  : "=r"(esr));
    asm volatile("mrs %0, FAR_EL1"  : "=r"(far));
    asm volatile("mrs %0, ELR_EL1"  : "=r"(elr));
    asm volatile("mrs %0, SPSR_EL1" : "=r"(spsr));

    unsigned long ec = (esr >> 26) & 0x3F;

    uart_puts("\r\n*** SYNCHRONOUS EXCEPTION ***\r\n");
    uart_puts("Exception Class : 0x"); uart_puthex(ec);
    uart_puts(" ("); uart_puts(get_exception_class_name(ec)); uart_puts(")\r\n");
    uart_puts("FAR_EL1         : 0x"); uart_puthex(far);  uart_puts("\r\n");
    uart_puts("ELR_EL1         : 0x"); uart_puthex(elr);  uart_puts("\r\n");
    uart_puts("SPSR_EL1        : 0x"); uart_puthex(spsr); uart_puts("\r\n");
    uart_puts("\r\nKernel halted.\r\n");

    while (1) {
        asm volatile("wfi");
    }
}

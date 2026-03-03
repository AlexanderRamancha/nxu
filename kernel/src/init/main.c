#define UART_BASE   0x9000000ULL

#define UARTDR      (*(volatile unsigned int *)(UART_BASE + 0x00))
#define UARTFR      (*(volatile unsigned int *)(UART_BASE + 0x18))
#define UARTCR      (*(volatile unsigned int *)(UART_BASE + 0x30))

#define UARTFR_TXFF (1U << 5)

static void uart_putc(char c) {
    while (UARTFR & UARTFR_TXFF) {}
    UARTDR = c;
}

static void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

void kernel_main(void) {
    // Enable TX (minimal)
    UARTCR = (1U << 8);  // TXE = 1

    uart_puts("NXU ALIVE\r\n");

    // If no output → infinite nop loop (QEMU CPU usage ~100%)
    while (1) {
        asm volatile("nop; nop; nop; nop" ::: "memory");
    }
}
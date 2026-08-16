#include "pl011.h"
#include <stdint.h>

#define UART_BASE   0x9000000ULL

#define UARTDR      (*(volatile uint32_t *)(UART_BASE + 0x00))
#define UARTFR      (*(volatile uint32_t *)(UART_BASE + 0x18))
#define UARTCR      (*(volatile uint32_t *)(UART_BASE + 0x30))
#define UARTLCR_H   (*(volatile uint32_t *)(UART_BASE + 0x2C))

#define UARTFR_TXFF (1U << 5)
#define UARTFR_BUSY (1U << 3)
#define UARTCR_UARTEN   (1U << 0)
#define UARTCR_TXE      (1U << 8)
#define UARTLCR_H_WLEN_8 (3U << 5)
#define UARTLCR_H_FEN    (1U << 4)

void uart_init(void)
{
    UARTCR = 0;
    asm volatile("dmb sy" ::: "memory");
    UARTLCR_H = UARTLCR_H_WLEN_8 | UARTLCR_H_FEN;
    UARTCR = UARTCR_UARTEN | UARTCR_TXE;
    asm volatile("dmb sy" ::: "memory");
}

void uart_putc(char c)
{
    while (UARTFR & (UARTFR_TXFF | UARTFR_BUSY));
    UARTDR = (uint32_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

void uart_puthex(unsigned long value)
{
    const char *hex = "0123456789ABCDEF";
    for (int i = 60; i >= 0; i -= 4)
        uart_putc(hex[(value >> i) & 0xF]);
}

void uart_putdec(unsigned long value)
{
    if (value == 0) {
        uart_putc('0');
        return;
    }
    char buf[20];
    int i = 19;
    buf[i] = '\0';
    while (value) {
        buf[--i] = '0' + (value % 10);
        value /= 10;
    }
    uart_puts(&buf[i]);
}

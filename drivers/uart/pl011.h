#ifndef DRIVERS_UART_PL011_H
#define DRIVERS_UART_PL011_H

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_puthex(unsigned long value);
void uart_putdec(unsigned long value);

#endif

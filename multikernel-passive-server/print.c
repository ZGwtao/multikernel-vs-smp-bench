#include "print.h"

#include <microkit.h>

uintptr_t uart_base;

#define UART_REG(x) ((volatile uint32_t *)(uart_base + (x)))

#if defined(CONFIG_PLAT_ODROIDC4)
#define UART_WFIFO 0x0
#define UART_STATUS 0xC
#define UART_TX_FULL (1 << 21)

void putc(uint8_t ch) {
    while ((*UART_REG(UART_STATUS) & UART_TX_FULL));
    *UART_REG(UART_WFIFO) = ch;
}

#elif defined(CONFIG_PLAT_BCM2711)

#undef UART_REG
#define UART_REG(x) ((volatile uint32_t *)(uart_base + 0x40 + (x)))
/* rpi4b */
#define UART_BASE 0xfe215040
#define MU_IO 0x00
#define MU_LSR 0x14
#define MU_LSR_TXIDLE (1 << 6)

void putc(uint8_t ch)
{
    while (!(*UART_REG(MU_LSR) & MU_LSR_TXIDLE));
    *UART_REG(MU_IO) = (ch & 0xff);
}

#elif defined(CONFIG_PLAT_TQMA8XQP1GB)
// #define UART_BASE 0x5a070000
#define STAT 0x14
#define TRANSMIT 0x1c
#define STAT_TDRE (1 << 23)

void putc(uint8_t ch)
{
    while (!(*UART_REG(STAT) & STAT_TDRE)) { }
    *UART_REG(TRANSMIT) = ch;
}

#elif defined(CONFIG_PLAT_MAAXBOARD)
#define STAT 0x98
#define TRANSMIT 0x40
#define STAT_TDRE (1 << 14)

void putc(uint8_t ch)
{
    // ensure FIFO has space
    while (!(*UART_REG(STAT) & STAT_TDRE)) { }
    *UART_REG(TRANSMIT) = ch;
}
#else
#error "unsupported board"
#endif

void puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            putc('\r');
        }
        putc(*s);
        s++;
    }
}

void print(const char *s) {
    puts(microkit_name);
    puts(": ");
    puts(s);
}

char hexchar(unsigned int v) {
    return v < 10 ? '0' + v : ('a' - 10) + v;
}

void puthex32(uint32_t val) {
    char buffer[8 + 3];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[8 + 3 - 1] = 0;
    for (unsigned i = 8 + 1; i > 1; i--) {
        buffer[i] = hexchar(val & 0xf);
        val >>= 4;
    }
    puts(buffer);
}

void puthex64(uint64_t val) {
    char buffer[16 + 3];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[16 + 3 - 1] = 0;
    for (unsigned i = 16 + 1; i > 1; i--) {
        buffer[i] = hexchar(val & 0xf);
        val >>= 4;
    }
    puts(buffer);
}

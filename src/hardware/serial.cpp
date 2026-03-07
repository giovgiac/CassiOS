#include "hardware/serial.hpp"
#include "hardware/port.hpp"

using namespace cassio;
using namespace cassio::hardware;

static const u16 COM1 = 0x3F8;

void hardware::serial_init() {
    Port<u8> ier(static_cast<u16>(COM1 + 1));
    Port<u8> lcr(static_cast<u16>(COM1 + 3));
    Port<u8> fcr(static_cast<u16>(COM1 + 2));
    Port<u8> mcr(static_cast<u16>(COM1 + 4));
    Port<u8> dll(COM1);
    Port<u8> dlh(static_cast<u16>(COM1 + 1));

    ier.write(0x00);    // Disable interrupts
    lcr.write(0x80);    // Enable DLAB (set baud rate divisor)
    dll.write(0x03);    // 38400 baud (divisor = 3)
    dlh.write(0x00);
    lcr.write(0x03);    // 8 bits, no parity, one stop bit
    fcr.write(0xC7);    // Enable FIFO, clear, 14-byte threshold
    mcr.write(0x03);    // DTR + RTS
}

void hardware::serial_putchar(char ch) {
    Port<u8> lsr(static_cast<u16>(COM1 + 5));
    Port<u8> data(COM1);

    // Wait for transmit buffer to be empty
    while ((lsr.read() & 0x20) == 0);

    data.write(static_cast<u8>(ch));
}

void hardware::serial_puts(const char* str) {
    for (u32 i = 0; str[i] != '\0'; ++i) {
        hardware::serial_putchar(str[i]);
    }
}

void hardware::serial_put_dec(u32 value) {
    if (value == 0) {
        hardware::serial_putchar('0');
        return;
    }

    char buf[10];
    i32 len = 0;
    while (value > 0) {
        buf[len++] = '0' + (value % 10);
        value /= 10;
    }

    for (i32 i = len - 1; i >= 0; --i) {
        hardware::serial_putchar(buf[i]);
    }
}

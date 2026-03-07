#include "hardware/serial.hpp"

using namespace cassio;
using namespace cassio::hardware;

Serial Serial::instance;

Serial::Serial()
    : data(PortType::SerialCOM1Data)
    , interrupt_enable(PortType::SerialCOM1InterruptEnable)
    , fifo_control(PortType::SerialCOM1FIFOControl)
    , line_control(PortType::SerialCOM1LineControl)
    , modem_control(PortType::SerialCOM1ModemControl)
    , line_status(PortType::SerialCOM1LineStatus)
{
    interrupt_enable.write(0x00);   // Disable interrupts
    line_control.write(0x80);       // Enable DLAB (set baud rate divisor)
    data.write(0x03);               // 38400 baud (divisor = 3)
    interrupt_enable.write(0x00);   // High byte of divisor
    line_control.write(0x03);       // 8 bits, no parity, one stop bit
    fifo_control.write(0xC7);       // Enable FIFO, clear, 14-byte threshold
    modem_control.write(0x03);      // DTR + RTS
}

void Serial::putchar(char ch) {
    while ((line_status.read() & 0x20) == 0);
    data.write(static_cast<u8>(ch));
}

void Serial::puts(const char* str) {
    for (u32 i = 0; str[i] != '\0'; ++i) {
        putchar(str[i]);
    }
}

void Serial::put_dec(u32 value) {
    if (value == 0) {
        putchar('0');
        return;
    }

    char buf[10];
    i32 len = 0;
    while (value > 0) {
        buf[len++] = '0' + (value % 10);
        value /= 10;
    }

    for (i32 i = len - 1; i >= 0; --i) {
        putchar(buf[i]);
    }
}

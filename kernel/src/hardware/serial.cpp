#include "hardware/serial.hpp"

using namespace cassio;
using namespace std;
using namespace cassio::hardware;

Serial COM1::instance(
    PortType::SerialCOM1Data,
    PortType::SerialCOM1InterruptEnable,
    PortType::SerialCOM1FIFOControl,
    PortType::SerialCOM1LineControl,
    PortType::SerialCOM1ModemControl,
    PortType::SerialCOM1LineStatus
);

Serial::Serial(PortType data, PortType interrupt_enable, PortType fifo_control,
               PortType line_control, PortType modem_control, PortType line_status)
    : data(data)
    , interrupt_enable(interrupt_enable)
    , fifo_control(fifo_control)
    , line_control(line_control)
    , modem_control(modem_control)
    , line_status(line_status)
{
    this->interrupt_enable.write(0x00);   // Disable interrupts
    this->line_control.write(0x80);       // Enable DLAB (set baud rate divisor)
    this->data.write(0x03);               // 38400 baud (divisor = 3)
    this->interrupt_enable.write(0x00);   // High byte of divisor
    this->line_control.write(0x03);       // 8 bits, no parity, one stop bit
    this->fifo_control.write(0xC7);       // Enable FIFO, clear, 14-byte threshold
    this->modem_control.write(0x03);      // DTR + RTS
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


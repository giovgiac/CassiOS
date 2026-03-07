#ifndef HARDWARE_SERIAL_HPP_
#define HARDWARE_SERIAL_HPP_

#include <common/types.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

class Serial {
private:
    Port<u8> data;
    Port<u8> interrupt_enable;
    Port<u8> fifo_control;
    Port<u8> line_control;
    Port<u8> modem_control;
    Port<u8> line_status;

public:
    Serial(PortType data, PortType interrupt_enable, PortType fifo_control,
           PortType line_control, PortType modem_control, PortType line_status);
    ~Serial() = default;

    void putchar(char ch);
    void puts(const char* str);
    void put_dec(u32 value);

    Serial(const Serial&) = delete;
    Serial(Serial&&) = delete;
    Serial& operator=(const Serial&) = delete;
    Serial& operator=(Serial&&) = delete;
};

class COM1 final {
private:
    static Serial instance;

    COM1() = delete;

public:
    inline static Serial& getSerial() {
        return instance;
    }
};

} // hardware
} // cassio

#endif // HARDWARE_SERIAL_HPP_

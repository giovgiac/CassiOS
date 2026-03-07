#ifndef HARDWARE_SERIAL_HPP_
#define HARDWARE_SERIAL_HPP_

#include <common/types.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

class Serial final {
private:
    Port<u8> data;
    Port<u8> interrupt_enable;
    Port<u8> fifo_control;
    Port<u8> line_control;
    Port<u8> modem_control;
    Port<u8> line_status;

    static Serial instance;

    Serial();

public:
    ~Serial() = default;

    void putchar(char ch);
    void puts(const char* str);
    void put_dec(u32 value);

    inline static Serial& getSerial() {
        return instance;
    }

    Serial(const Serial&) = delete;
    Serial(Serial&&) = delete;
    Serial& operator=(const Serial&) = delete;
    Serial& operator=(Serial&&) = delete;
};

} // hardware
} // cassio

#endif // HARDWARE_SERIAL_HPP_

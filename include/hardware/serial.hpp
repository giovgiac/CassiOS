#ifndef HARDWARE_SERIAL_HPP_
#define HARDWARE_SERIAL_HPP_

#include <common/types.hpp>

namespace cassio {
namespace hardware {

void serial_init();
void serial_putchar(char ch);
void serial_puts(const char* str);
void serial_put_dec(u32 value);

} // hardware
} // cassio

#endif // HARDWARE_SERIAL_HPP_

/**
 * serial.hpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * The COM1 driver is Rust (kernel/src/hardware/serial.rs). COM1 forwards
 * to it until its callers are ported.
 *
 */

#ifndef HARDWARE_SERIAL_HPP_
#define HARDWARE_SERIAL_HPP_

#include <std/types.hpp>

extern "C" void com1_init();
extern "C" void com1_write(const char* buf, std::u32 len);

namespace cassio {
namespace hardware {

class COM1 final {
public:
    /** Programs the UART. Must run before the first write. */
    static void init() { com1_init(); }

    static void write(const char* buf, std::u32 len) { com1_write(buf, len); }

    COM1() = delete;
};

} // namespace hardware
} // namespace cassio

#endif // HARDWARE_SERIAL_HPP_

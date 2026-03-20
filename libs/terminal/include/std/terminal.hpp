/**
 * terminal.hpp -- Terminal service client
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Instance-based client for the terminal service. Drop-in
 * replacement for the old std::vga::Vga client. The constructor
 * resolves the service PID from the nameserver automatically,
 * blocking until the service is registered.
 *
 */

#ifndef STD_TERMINAL_HPP
#define STD_TERMINAL_HPP

#include <std/str.hpp>
#include <std/types.hpp>

namespace std {
namespace terminal {

class Terminal {
public:
    Terminal();

    void putchar(char c);
    void write(str::StringView text);
    void clear();
    void flush();
    void setCursor(u8 col, u8 row);
    void getCursor(u8& col, u8& row);

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

private:
    u32 pid;
};

} // namespace terminal
} // namespace std

#endif // STD_TERMINAL_HPP

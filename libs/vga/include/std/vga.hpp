/**
 * vga.hpp -- VGA terminal service client
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 * Instance-based client for the VGA terminal service. The constructor
 * resolves the service PID from the nameserver automatically, blocking
 * until the service is registered.
 *
 */

#ifndef STD_VGA_HPP
#define STD_VGA_HPP

#include <std/types.hpp>

namespace std {
namespace vga {

class Vga {
  public:
    /// Construct a VGA client. Blocks until the "vga" service is
    /// registered with the nameserver.
    Vga();

    /// Print a single character (fire-and-forget).
    void putchar(char c);

    /// Write a null-terminated string (fire-and-forget).
    void write(const char* str);

    /// Write a buffer of explicit length (fire-and-forget).
    void write(const char* buf, u32 len);

    /// Clear the screen (blocking).
    void clear();

    /// Set the cursor position (blocking).
    void setCursor(u8 col, u8 row);

    /// Get the current cursor position (blocking).
    void getCursor(u8& col, u8& row);

    Vga(const Vga&) = delete;
    Vga& operator=(const Vga&) = delete;

  private:
    u32 pid;
};

} // namespace vga
} // namespace std

#endif // STD_VGA_HPP

/**
 * terminal.hpp -- VGA text-mode terminal
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef VGA_TERMINAL_HPP_
#define VGA_TERMINAL_HPP_

#include <types.hpp>
#include <port.hpp>

namespace cassio {

constexpr u8 VGA_WIDTH  = 80;
constexpr u8 VGA_HEIGHT = 25;

/**
 * @brief VGA text-mode terminal.
 *
 * Writes directly to the VGA text buffer. Each entry is 2 bytes:
 * the high byte holds color attributes, the low byte holds the character.
 * Interprets control characters: newline, backspace, delete, and tab.
 *
 */
class VgaTerminal {
public:
    VgaTerminal(u16* buffer);

    void putchar(char ch);
    void clear();
    void setCursor(u8 col, u8 row);
    u8 getCursorX() const;
    u8 getCursorY() const;

private:
    u16* buffer;
    u8 x;
    u8 y;
    u16 color;

    hardware::Port<u8> crtc_index;
    hardware::Port<u8> crtc_data;

    void updateCursor();
    void scrollUp();
};

} // cassio

#endif // VGA_TERMINAL_HPP_

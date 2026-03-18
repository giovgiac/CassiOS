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

#include <std/types.hpp>
#include <port.hpp>

namespace cassio {

constexpr std::u8 VGA_WIDTH  = 80;
constexpr std::u8 VGA_HEIGHT = 25;

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
    VgaTerminal(std::u16* buffer);

    void putchar(char ch);
    void clear();
    void setCursor(std::u8 col, std::u8 row);
    std::u8 getCursorX() const;
    std::u8 getCursorY() const;

private:
    std::u16* buffer;
    std::u8 x;
    std::u8 y;
    std::u16 color;

    hardware::Port<std::u8> crtc_index;
    hardware::Port<std::u8> crtc_data;

    void updateCursor();
    void scrollUp();
};

} // cassio

#endif // VGA_TERMINAL_HPP_

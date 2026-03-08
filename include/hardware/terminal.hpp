/**
 * terminal.hpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef HARDWARE_TERMINAL_HPP_
#define HARDWARE_TERMINAL_HPP_

#include <common/types.hpp>
#include <hardware/port.hpp>

namespace cassio {
namespace hardware {

/**
 * @brief Base class for text output terminals.
 *
 */
class Terminal {
public:
    Terminal() = default;
    ~Terminal() = default;

    /**
     * @brief Writes a single character to the terminal.
     *
     */
    virtual void putchar(char ch) = 0;

    /**
     * @brief Writes a null-terminated string to the terminal.
     *
     */
    virtual void print(const char* str) = 0;

    /**
     * @brief Writes a 32-bit value as a hex string (e.g., "0x0000001A").
     *
     */
    virtual void print_hex(u32 value) = 0;

    /**
     * @brief Clears the terminal.
     *
     */
    virtual void clear() = 0;
};

constexpr u8 VGA_WIDTH  = 80;
constexpr u8 VGA_HEIGHT = 25;

/**
 * @brief VGA text-mode terminal singleton.
 *
 * Writes directly to the VGA text buffer at 0xB8000. Each entry is 2 bytes:
 * the high byte holds color attributes, the low byte holds the character.
 * Interprets control characters: newline, backspace, delete, and tab.
 *
 */
class VgaTerminal : public Terminal {
private:
    u16* buffer;
    u8 x;
    u8 y;
    u16 color;

    Port<u8> crtc_index;
    Port<u8> crtc_data;

    static VgaTerminal instance;

    VgaTerminal();

    void updateCursor();

public:
    ~VgaTerminal() = default;

    /**
     * @brief Returns the singleton VgaTerminal instance.
     *
     */
    static VgaTerminal& getTerminal();

    /**
     * @brief Returns the current cursor column.
     *
     */
    u8 getCursorX();

    /**
     * @brief Returns the current cursor row.
     *
     */
    u8 getCursorY();

    /**
     * @brief Moves the cursor to the given column and row.
     *
     */
    void setCursor(u8 col, u8 row);

    virtual void putchar(char ch) override;
    virtual void print(const char* str) override;
    virtual void print_hex(u32 value) override;
    virtual void clear() override;

    /** Deleted Methods */
    VgaTerminal(const VgaTerminal&) = delete;
    VgaTerminal(VgaTerminal&&) = delete;
    VgaTerminal& operator=(const VgaTerminal&) = delete;
    VgaTerminal& operator=(VgaTerminal&&) = delete;
};

} // hardware
} // cassio

#endif // HARDWARE_TERMINAL_HPP_

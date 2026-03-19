/**
 * keyboard.hpp -- PS/2 keyboard scancode translation and buffering
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef KBD_KEYBOARD_HPP_
#define KBD_KEYBOARD_HPP_

#include <std/types.hpp>
#include <std/kbd.hpp>

using std::kbd::KeyCode;

namespace cassio {

/**
 * @brief PS/2 scan set 1 scancodes as received from the keyboard hardware.
 *
 */
enum class ScanCode : std::u8 {
    Escape          = 0x01,
    One             = 0x02,
    Two             = 0x03,
    Three           = 0x04,
    Four            = 0x05,
    Five            = 0x06,
    Six             = 0x07,
    Seven           = 0x08,
    Eight           = 0x09,
    Nine            = 0x0A,
    Zero            = 0x0B,
    Minus           = 0x0C,
    Equals          = 0x0D,
    Backspace       = 0x0E,
    Tab             = 0x0F,
    Q               = 0x10,
    W               = 0x11,
    E               = 0x12,
    R               = 0x13,
    T               = 0x14,
    Y               = 0x15,
    U               = 0x16,
    I               = 0x17,
    O               = 0x18,
    P               = 0x19,
    LeftBracket     = 0x1A,
    RightBracket    = 0x1B,
    Enter           = 0x1C,
    LeftCtrl        = 0x1D,
    A               = 0x1E,
    S               = 0x1F,
    D               = 0x20,
    F               = 0x21,
    G               = 0x22,
    H               = 0x23,
    J               = 0x24,
    K               = 0x25,
    L               = 0x26,
    Semicolon       = 0x27,
    Quote           = 0x28,
    Backquote       = 0x29,
    LeftShift       = 0x2A,
    Backslash       = 0x2B,
    Z               = 0x2C,
    X               = 0x2D,
    C               = 0x2E,
    V               = 0x2F,
    B               = 0x30,
    N               = 0x31,
    M               = 0x32,
    Comma           = 0x33,
    Period          = 0x34,
    Slash           = 0x35,
    RightShift      = 0x36,
    LeftAlt         = 0x38,
    Space           = 0x39,
    CapsLock        = 0x3A,
    F1              = 0x3B,
    F2              = 0x3C,
    F3              = 0x3D,
    F4              = 0x3E,
    F5              = 0x3F,
    F6              = 0x40,
    F7              = 0x41,
    F8              = 0x42,
    F9              = 0x43,
    F10             = 0x44,
    NumLock         = 0x45,
    F11             = 0x57,
    F12             = 0x58
};

static constexpr std::u16 KEYBOARD_BUFFER_SIZE = 256;

/**
 * @brief PS/2 keyboard that translates scancodes into KeyCode values
 *        and buffers printable characters in a ring buffer.
 *
 */
class Keyboard {
public:
    Keyboard();

    void handleScancode(std::u8 raw);
    char readBuffer();
    std::u16 bufferCount() const;

    static KeyCode resolveShift(KeyCode key);

private:
    static const KeyCode scancode_table[0x59];

    bool shift_held;
    bool ctrl_held;
    bool alt_held;
    bool caps_lock_on;
    bool e0_prefix;

    char ring[KEYBOARD_BUFFER_SIZE];
    std::u16 ring_head;
    std::u16 ring_tail;

public:
    Keyboard(const Keyboard&) = delete;
    Keyboard(Keyboard&&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;
    Keyboard& operator=(Keyboard&&) = delete;
};

} // cassio

#endif // KBD_KEYBOARD_HPP_

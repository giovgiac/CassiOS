/**
 * keycode.hpp -- resolved key values shared between kernel and userspace
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef COMMON_KEYCODE_HPP_
#define COMMON_KEYCODE_HPP_

#include <types.hpp>

namespace cassio {

/**
 * @brief Resolved key values after scancode translation.
 *
 * ASCII-range values (0x00-0x7F) correspond to their ASCII characters.
 * Non-ASCII keys (function keys, arrows) use values above 0x7F.
 *
 */
enum class KeyCode : u8 {
    Backspace                                   = 0x08,
    Tab                                         = 0x09,
    Enter                                       = 0x0D,
    Escape                                      = 0x1B,
    Space                                       = 0x20,
    Exclamation                                 = 0x21,
    DoubleQuote                                 = 0x22,
    Hash                                        = 0x23,
    Dollar                                      = 0x24,
    Percent                                     = 0x25,
    Ampersand                                   = 0x26,
    Quote                                       = 0x27,
    LeftParenthesis                             = 0x28,
    RightParenthesis                            = 0x29,
    Asterisk                                    = 0x2A,
    Plus                                        = 0x2B,
    Comma                                       = 0x2C,
    Minus                                       = 0x2D,
    Period                                      = 0x2E,
    Slash                                       = 0x2F,
    Zero                                        = 0x30,
    One                                         = 0x31,
    Two                                         = 0x32,
    Three                                       = 0x33,
    Four                                        = 0x34,
    Five                                        = 0x35,
    Six                                         = 0x36,
    Seven                                       = 0x37,
    Eight                                       = 0x38,
    Nine                                        = 0x39,
    Colon                                       = 0x3A,
    Semicolon                                   = 0x3B,
    LessThan                                    = 0x3C,
    Equals                                      = 0x3D,
    GreaterThan                                 = 0x3E,
    Question                                    = 0x3F,
    At                                          = 0x40,
    A                                           = 0x41,
    B                                           = 0x42,
    C                                           = 0x43,
    D                                           = 0x44,
    E                                           = 0x45,
    F                                           = 0x46,
    G                                           = 0x47,
    H                                           = 0x48,
    I                                           = 0x49,
    J                                           = 0x4A,
    K                                           = 0x4B,
    L                                           = 0x4C,
    M                                           = 0x4D,
    N                                           = 0x4E,
    O                                           = 0x4F,
    P                                           = 0x50,
    Q                                           = 0x51,
    R                                           = 0x52,
    S                                           = 0x53,
    T                                           = 0x54,
    U                                           = 0x55,
    V                                           = 0x56,
    W                                           = 0x57,
    X                                           = 0x58,
    Y                                           = 0x59,
    Z                                           = 0x5A,
    LeftBracket                                 = 0x5B,
    BackSlash                                   = 0x5C,
    RightBracket                                = 0x5D,
    Caret                                       = 0x5E,
    Underscore                                  = 0x5F,
    Backquote                                   = 0x60,
    a                                           = 0x61,
    b                                           = 0x62,
    c                                           = 0x63,
    d                                           = 0x64,
    e                                           = 0x65,
    f                                           = 0x66,
    g                                           = 0x67,
    h                                           = 0x68,
    i                                           = 0x69,
    j                                           = 0x6A,
    k                                           = 0x6B,
    l                                           = 0x6C,
    m                                           = 0x6D,
    n                                           = 0x6E,
    o                                           = 0x6F,
    p                                           = 0x70,
    q                                           = 0x71,
    r                                           = 0x72,
    s                                           = 0x73,
    t                                           = 0x74,
    u                                           = 0x75,
    v                                           = 0x76,
    w                                           = 0x77,
    x                                           = 0x78,
    y                                           = 0x79,
    z                                           = 0x7A,
    LeftCurly                                   = 0x7B,
    Pipe                                        = 0x7C,
    RightCurly                                  = 0x7D,
    Tilde                                       = 0x7E,
    Delete                                      = 0x7F,

    // Non-ASCII keys.
    F1                                          = 0x80,
    F2                                          = 0x81,
    F3                                          = 0x82,
    F4                                          = 0x83,
    F5                                          = 0x84,
    F6                                          = 0x85,
    F7                                          = 0x86,
    F8                                          = 0x87,
    F9                                          = 0x88,
    F10                                         = 0x89,
    F11                                         = 0x8A,
    F12                                         = 0x8B,
    LeftArrow                                   = 0x8C,
    RightArrow                                  = 0x8D
};

} // cassio

#endif // COMMON_KEYCODE_HPP_

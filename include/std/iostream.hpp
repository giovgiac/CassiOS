/**
 * iostream.hpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#ifndef STD_IOSTREAM_HPP_
#define STD_IOSTREAM_HPP_

#include <common/types.hpp>

namespace std {

constexpr cassio::u8 TERMINAL_WIDTH     = 80;
constexpr cassio::u8 TERMINAL_HEIGHT    = 25;

/**
 * @brief
 * 
 * Each entry in the terminal is 2 bytes, with the first byte containing color information and the
 * second byte containing the character. Therefore, the function will maintain the color information
 * by ANDing with 0xFF00 and adding the new text information with an OR.
 * 
 */
class ostream {
public:
    ostream() = default;

public:
    ostream& operator<<(const char ch);
    ostream& operator<<(const char* str);
    ostream& operator<<(const cassio::u8 byte);
    ostream& operator<<(const cassio::u16 word);
    ostream& operator<<(const cassio::u32 dword);
    ostream& operator<<(const cassio::u64 qword);

    /** Deleted Methods */
    ostream(const ostream&) = delete;
    ostream(ostream&&) = delete;
    ostream& operator=(const ostream&) = delete;
    ostream& operator=(ostream&&) = delete;
};

// Global standard output stream.
static ostream cout;

// Newline character.
constexpr char endl = '\n';

}

#endif // STD_IOSTREAM_HPP_

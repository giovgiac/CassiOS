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
 * @brief Output stream that writes directly to the VGA text buffer at 0xB8000.
 *
 * Each VGA entry is 2 bytes: the high byte holds color attributes and the low byte
 * holds the character. Write operations preserve existing attributes by masking with
 * 0xFF00 and ORing in the new character.
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

    void clear();

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

/**
 * iostream.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "std/iostream.hpp"

using namespace std;

static ostream cout;

void clear() {
    static u16* tty = reinterpret_cast<u16*>(0xb8000);

    for (u8 y = 0; y < TERMINAL_HEIGHT; ++y) {
        for (u8 x = 0; x < TERMINAL_WIDTH; ++x) {
            // Copy High-Bits and Merge with New Low-Bits
            tty[TERMINAL_WIDTH * y + x] = (tty[TERMINAL_WIDTH * y + x] & 0xFF00) | ' ';
        }
    }
}

ostream& ostream::operator<<(const char ch) {
    static u16* tty = reinterpret_cast<u16*>(0xb8000);
    static u8 x = 0, y = 0;

    switch(ch) {
    case '\n':
        y += 1;
        x = 0;

        break;
    default:
        // Copy High-Bits and Merge with New Low-Bits
        tty[TERMINAL_WIDTH * y + x] = (tty[TERMINAL_WIDTH * y + x] & 0xFF00) | ch;
        x += 1;

        break;
    }

    if (x >= TERMINAL_WIDTH) {
        y += 1;
        x = 0;
    }

    if (y >= TERMINAL_HEIGHT) {
        clear();
        x = y = 0;
    }

    return *this;
}

ostream& ostream::operator<<(const char* str) {
    for (u32 i = 0; str[i] != '\0' ; ++i) {
        *this << str[i];
    }

    return *this;
}

ostream& ostream::operator<<(const u8 byte) {
    char* str = "0x00";
    const char* hex = "0123456789ABCDEF";

    str[2] = hex[(byte >> 4) & 0x0F];
    str[3] = hex[byte & 0x0F];

    *this << str;
    return *this;
}

ostream& ostream::operator<<(const u16 word) {
    char* str = "0x0000";
    const char* hex = "0123456789ABCDEF";

    str[2] = hex[(word >> 12) & 0x000F];
    str[3] = hex[(word >> 8) & 0x000F];
    str[4] = hex[(word >> 4) & 0x000F];
    str[5] = hex[word & 0x000F];

    *this << str;
    return *this;
}

ostream& ostream::operator<<(const u32 dword) {
    char* str = "0x00000000";
    const char* hex = "0123456789ABCDEF";

    str[2] = hex[(dword >> 28) & 0x0000000F];
    str[3] = hex[(dword >> 24) & 0x0000000F];
    str[4] = hex[(dword >> 20) & 0x0000000F];
    str[5] = hex[(dword >> 16) & 0x0000000F];
    str[6] = hex[(dword >> 12) & 0x0000000F];
    str[7] = hex[(dword >> 8) & 0x0000000F];
    str[8] = hex[(dword >> 4) & 0x0000000F];
    str[9] = hex[dword & 0x0000000F];

    *this << str;
    return *this;
}

ostream& ostream::operator<<(const u64 qword) {

    return *this;
}

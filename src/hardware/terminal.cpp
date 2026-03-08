/**
 * terminal.cpp
 *
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "hardware/terminal.hpp"

using namespace cassio;
using namespace cassio::hardware;

/** VgaTerminal Methods */

VgaTerminal VgaTerminal::instance;

VgaTerminal::VgaTerminal()
    : buffer(reinterpret_cast<u16*>(0xB8000)),
      x(0),
      y(0) {}

VgaTerminal& VgaTerminal::getTerminal() {
    return instance;
}

void VgaTerminal::putchar(char ch) {
    switch (ch) {
    case '\n':
        y += 1;
        x = 0;
        break;

    case '\b':
        if (x > 0) {
            x -= 1;
        } else if (y > 0) {
            y -= 1;
            x = VGA_WIDTH - 1;
        }
        buffer[VGA_WIDTH * y + x] = (buffer[VGA_WIDTH * y + x] & 0xFF00) | ' ';
        break;

    case 0x7F: // DEL
        buffer[VGA_WIDTH * y + x] = (buffer[VGA_WIDTH * y + x] & 0xFF00) | ' ';
        break;

    case '\t': {
        u8 next = (x + 8) & ~7;
        if (next >= VGA_WIDTH) {
            next = VGA_WIDTH;
        }
        while (x < next) {
            buffer[VGA_WIDTH * y + x] = (buffer[VGA_WIDTH * y + x] & 0xFF00) | ' ';
            x += 1;
        }
        break;
    }

    default:
        buffer[VGA_WIDTH * y + x] = (buffer[VGA_WIDTH * y + x] & 0xFF00) | ch;
        x += 1;
        break;
    }

    if (x >= VGA_WIDTH) {
        y += 1;
        x = 0;
    }

    if (y >= VGA_HEIGHT) {
        clear();
    }
}

void VgaTerminal::print(const char* str) {
    for (u32 i = 0; str[i] != '\0'; ++i) {
        putchar(str[i]);
    }
}

void VgaTerminal::print_hex(u32 value) {
    const char* hex = "0123456789ABCDEF";
    char str[11] = "0x00000000";

    str[2] = hex[(value >> 28) & 0x0F];
    str[3] = hex[(value >> 24) & 0x0F];
    str[4] = hex[(value >> 20) & 0x0F];
    str[5] = hex[(value >> 16) & 0x0F];
    str[6] = hex[(value >> 12) & 0x0F];
    str[7] = hex[(value >> 8) & 0x0F];
    str[8] = hex[(value >> 4) & 0x0F];
    str[9] = hex[value & 0x0F];

    print(str);
}

void VgaTerminal::clear() {
    for (u8 row = 0; row < VGA_HEIGHT; ++row) {
        for (u8 col = 0; col < VGA_WIDTH; ++col) {
            buffer[VGA_WIDTH * row + col] = (buffer[VGA_WIDTH * row + col] & 0xFF00) | ' ';
        }
    }

    x = 0;
    y = 0;
}

/**
 * terminal.cpp
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
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
      y(0),
      color(0x0700),
      crtc_index(PortType::VgaCrtcIndex),
      crtc_data(PortType::VgaCrtcData) {}

void VgaTerminal::updateCursor() {
    u16 pos = VGA_WIDTH * y + x;
    crtc_index.write(0x0F);
    crtc_data.write(static_cast<u8>(pos & 0xFF));
    crtc_index.write(0x0E);
    crtc_data.write(static_cast<u8>((pos >> 8) & 0xFF));
}

VgaTerminal& VgaTerminal::getTerminal() {
    return instance;
}

u8 VgaTerminal::getCursorX() {
    return x;
}

u8 VgaTerminal::getCursorY() {
    return y;
}

void VgaTerminal::setCursor(u8 col, u8 row) {
    x = col;
    y = row;
    updateCursor();
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
        buffer[VGA_WIDTH * y + x] = color | ' ';
        break;

    case 0x7F: // DEL
        buffer[VGA_WIDTH * y + x] = color | ' ';
        break;

    case '\t': {
        u8 next = (x + 8) & ~7;
        if (next >= VGA_WIDTH) {
            next = VGA_WIDTH;
        }
        while (x < next) {
            buffer[VGA_WIDTH * y + x] = color | ' ';
            x += 1;
        }
        break;
    }

    default:
        buffer[VGA_WIDTH * y + x] = color | ch;
        x += 1;
        break;
    }

    if (x >= VGA_WIDTH) {
        y += 1;
        x = 0;
    }

    if (y >= VGA_HEIGHT) {
        scrollUp();
    }

    updateCursor();
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

void VgaTerminal::print_dec(u32 value) {
    if (value == 0) {
        putchar('0');
        return;
    }

    char buf[10];
    i32 i = 0;
    while (value > 0) {
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (--i >= 0) {
        putchar(buf[i]);
    }
}

void VgaTerminal::scrollUp() {
    for (u8 row = 1; row < VGA_HEIGHT; ++row) {
        for (u8 col = 0; col < VGA_WIDTH; ++col) {
            buffer[VGA_WIDTH * (row - 1) + col] = buffer[VGA_WIDTH * row + col];
        }
    }

    for (u8 col = 0; col < VGA_WIDTH; ++col) {
        buffer[VGA_WIDTH * (VGA_HEIGHT - 1) + col] = color | ' ';
    }

    y = VGA_HEIGHT - 1;
}

void VgaTerminal::clear() {
    for (u8 row = 0; row < VGA_HEIGHT; ++row) {
        for (u8 col = 0; col < VGA_WIDTH; ++col) {
            buffer[VGA_WIDTH * row + col] = color | ' ';
        }
    }

    x = 0;
    y = 0;
    updateCursor();
}

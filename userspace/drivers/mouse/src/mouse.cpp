/**
 * mouse.cpp -- PS/2 mouse packet parser implementation
 *
 * Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include <mouse.hpp>

using namespace cassio;
using namespace std;

void Mouse::init() {
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    offset = 0;
    buttons = 0;
    dx = 0;
    dy = 0;
}

void Mouse::handleByte(u8 byte) {
    buffer[offset] = byte;
    offset = (offset + 1) % 3;

    // Full 3-byte packet assembled.
    if (offset == 0) {
        buttons = buffer[0] & 0x07;
        dx += static_cast<i8>(buffer[1]);
        dy += -static_cast<i8>(buffer[2]);
    }
}

void Mouse::readState(u8& btns, i32& outDx, i32& outDy) {
    btns = buttons;
    outDx = dx;
    outDy = dy;
    dx = 0;
    dy = 0;
}

u8 Mouse::getButtons() const {
    return buttons;
}

i32 Mouse::getDx() const {
    return dx;
}

i32 Mouse::getDy() const {
    return dy;
}

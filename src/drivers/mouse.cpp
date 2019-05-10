/**
 * mouse.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "drivers/mouse.hpp"

using namespace cassio::drivers;

MouseDriver::MouseDriver() 
    : Driver(0x2C), cmd(0x64), data(0x60) {
    offset = 0;
    button = 0;

    // Show cursor initial position.
    u16* tty = reinterpret_cast<u16*>(0xb8000);
    tty[80 * 12 + 40] = ((tty[80 * 12 + 40] & 0xF000) >> 4) |
                        ((tty[80 * 12 + 40] & 0x0F00) << 4) |
                        ((tty[80 * 12 + 40] & 0x00FF));

    // Tells PIC to start sending interrupts.
    cmd.write(0xA8);

    // Requests current state.
    cmd.write(0x20);

    // Set new state to status.
    u8 status = data.read() | 2;
    cmd.write(0x60);
    data.write(status);

    cmd.write(0xD4);
    data.write(0xF4);
    data.read();
}

u32 MouseDriver::handleInterrupt(u32 esp) {
    u8 status = cmd.read();
    
    // Check whether there is data to be read.
    if (!(status & 0x20)) {
        return esp;
    }

    // Read data on current offset.
    buffer[offset] = data.read();
    offset = (offset + 1) % 3;

    // A buffer has been completed and can be processed.
    // buffer[0] has button, buffer[1] has x, buffer[2] has -y.
    if (offset == 0) {
        static i16 x = 40, y = 12;
        static u16* tty = reinterpret_cast<u16*>(0xb8000);

        // Unshow cursor at old position by inverting color at position.
        tty[80 * y + x] = ((tty[80 * y + x] & 0xF000) >> 4) |
                          ((tty[80 * y + x] & 0x0F00) << 4) |
                          ((tty[80 * y + x] & 0x00FF));

        x += buffer[1] - (0x100 & (buffer[0] << (8 - 4)));  // Bit 4 of buffer[0] is X's sign.
        if (x < 00) x = 00;
        if (x > 79) x = 79;

        y -= buffer[2] - (0x100 & (buffer[0] << (8 - 5)));  // Bit 5 of buffer[0] is Y's sign.
        if (y < 00) y = 00;
        if (y > 24) y = 24;

        // Show cursor on terminal by inverting colors at position.
        tty[80 * y + x] = ((tty[80 * y + x] & 0xF000) >> 4) |
                          ((tty[80 * y + x] & 0x0F00) << 4) |
                          ((tty[80 * y + x] & 0x00FF));

        // Check if left, middle or right mouse button have been pressed.
        for (u8 i = 0; i < 3; ++i) {
            // Check if ith button was pressed.
            if ((buffer[0] & (0x01 << i)) != (button & (0x01 << i))) {
                // Invert color for button presses.
                tty[80 * y + x] = ((tty[80 * y + x] & 0xF000) >> 4) |
                                  ((tty[80 * y + x] & 0x0F00) << 4) |
                                  ((tty[80 * y + x] & 0x00FF));
            }
        }

        button = buffer[0];
    }

    return esp;
}

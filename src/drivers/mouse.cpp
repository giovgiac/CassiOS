/**
 * mouse.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "drivers/mouse.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;

/** MouseEventHandler Methods */

void MouseEventHandler::OnActivate() {}

void MouseEventHandler::OnMouseDown(u8 btn) {}

void MouseEventHandler::OnMouseUp(u8 btn) {}

void MouseEventHandler::OnMouseMove(i8 dx, i8 dy) {}

/** MouseDriver Methods */

MouseDriver::MouseDriver(MouseEventHandler* han) 
    : Driver(DriverType::MouseController), cmd(PortType::KeyboardControllerCommand), data(PortType::KeyboardControllerData), handler(han) {}

void MouseDriver::activate() {
    offset = 0;
    button = 0;

    // Activate mouse handler.
    if (handler) {
        handler->OnActivate();
    }

    // Tells PIC to start sending interrupts.
    cmd.write(MouseCommand::EnableMouse);

    // Requests current state.
    cmd.write(MouseCommand::ReadCommand);

    // Set new state to status.
    u8 status = data.read() | 2;
    cmd.write(MouseCommand::WriteCommand);
    data.write(status);

    cmd.write(MouseCommand::WriteMouse);
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

    // Check for a handler.
    if (!handler) {
        return esp;
    }

    // A buffer has been completed and can be processed.
    // buffer[0] has button, buffer[1] has x, buffer[2] has -y.
    if (offset == 0) {
        if (buffer[1] != 0 || buffer[2] != 0) {
            // Inform handler of mouse movement.
            handler->OnMouseMove(buffer[1], -buffer[2]);
        }

        // Check if left, middle or right mouse button have been pressed.
        for (u8 i = 0; i < 3; ++i) {
            // Check if ith button was pressed.
            if ((buffer[0] & (0x01 << i)) != (button & (0x01 << i))) {
                if (button & (0x01 << i)) {
                    handler->OnMouseUp(i + 1);
                }
                else {
                    handler->OnMouseDown(i + 1);
                }
            }
        }

        button = buffer[0];
    }

    return esp;
}

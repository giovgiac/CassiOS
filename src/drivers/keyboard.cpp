/**
 * keyboard.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "drivers/keyboard.hpp"

using namespace cassio::drivers;
using namespace cassio::hardware;

void outputs(const char* str);

KeyboardCommandByte KeyboardDriver::readCommandByte() {
    KeyboardCommandByte status;

    status.byte = data.read();
    return status;
}

KeyboardDriver::KeyboardDriver()
    : Driver(DriverType::KeyboardController), cmd(PortType::KeyboardControllerCommand), data(PortType::KeyboardControllerData) {

    // Cleanup keystrokes before OS starts.
    while (cmd.read() & 0x1) {
        data.read();
    }

    // Enable communication with the keyboard.
    cmd.write(KeyboardCommand::EnableKeyboard);

    // Requests current command byte.
    cmd.write(KeyboardCommand::ReadCommand);

    // Set new command byte to status.
    KeyboardCommandByte status = readCommandByte();
    status.keyboard_interrupt = true;
    status.disable_keyboard = false;

    cmd.write(KeyboardCommand::WriteCommand);
    data.write(status.byte);

    // ?
    // data.write(0xF4);
}

u32 KeyboardDriver::handleInterrupt(u32 esp) {
    // PIC requires that key be fetched from data port.
    u8 key = data.read();

    // Ignore key release interrupts.
    if (key < 0x80) {
        switch (key) {
        // Ignore interrupts about NumLock, State, etc.
        case 0x45: case 0xC5: case 0xFA: break;
        case 0x02:
            outputs("1");
            break;
        case 0x03:
            outputs("2");
            break;
        case 0x04:
            outputs("3");
            break;
        case 0x05:
            outputs("4");
            break;
        case 0x06:
            outputs("5");
            break;
        case 0x07:
            outputs("6");
            break;
        case 0x08:
            outputs("7");
            break;
        case 0x09:
            outputs("8");
            break;
        case 0x0A:
            outputs("9");
            break;
        case 0x0B:
            outputs("0");
            break;
        case 0x0C:
            outputs("-");
            break;
        case 0x10:
            outputs("q");
            break;
        case 0x11:
            outputs("w");
            break;
        case 0x12:
            outputs("e");
            break;
        case 0x13:
            outputs("r");
            break;
        case 0x14:
            outputs("t");
            break;
        case 0x15:
            outputs("y");
            break;
        case 0x16:
            outputs("u");
            break;
        case 0x17:
            outputs("i");
            break;
        case 0x18:
            outputs("o");
            break;
        case 0x19:
            outputs("p");
            break;
        case 0x1A:
            outputs("[");
            break;
        case 0x1B:
            outputs("]");
            break;
        case 0x1C:
            outputs("\n");
            break;
        case 0x1E:
            outputs("a");
            break;
        case 0x1F:
            outputs("s");
            break;
        case 0x20:
            outputs("d");
            break;
        case 0x21:
            outputs("f");
            break;
        case 0x22:
            outputs("g");
            break;
        case 0x23:
            outputs("h");
            break;
        case 0x24:
            outputs("j");
            break;
        case 0x25:
            outputs("k");
            break;
        case 0x26:
            outputs("l");
            break;
        case 0x27:
            outputs(";");
            break;
        case 0x28:
            outputs("'");
            break;
        case 0x2C:
            outputs("z");
            break;
        case 0x2D:
            outputs("x");
            break;
        case 0x2E:
            outputs("c");
            break;
        case 0x2F:
            outputs("v");
            break;
        case 0x30:
            outputs("b");
            break;
        case 0x31:
            outputs("n");
            break;
        case 0x32:
            outputs("m");
            break;
        case 0x33:
            outputs(",");
            break;
        case 0x34:
            outputs(".");
            break;
        case 0x35:
            outputs("/");
            break;
        case 0x39:
            outputs(" ");
            break;

        default:
            // Code for acquiring key values.
            /**
            char* foo = "KEYBOARD 0x00 ";
            const char* hex = "0123456789ABCDEF";

            foo[11] = hex[(key >> 4) & 0x0F];
            foo[12] = hex[key & 0x0F];
            
            outputs(foo);
            */
            break;
        }
    }

    return esp;
}

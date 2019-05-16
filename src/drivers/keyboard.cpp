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

KeyboardCommandByte KeyboardDriver::readCommandByte() {
    KeyboardCommandByte status;

    status.byte = data.read();
    return status;
}

void KeyboardEventHandler::OnKeyDown(KeyCode key) {
    if (key != KeyCode::Enter)
        std::cout << static_cast<char>(key);
    else
        std::cout << '\n';
}

void KeyboardEventHandler::OnKeyUp(KeyCode key) {

}

KeyboardDriver::KeyboardDriver(KeyboardEventHandler* han)
    : Driver(DriverType::KeyboardController), cmd(PortType::KeyboardControllerCommand), data(PortType::KeyboardControllerData), handler(han) {}

void KeyboardDriver::activate() {
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
            if (handler) {
                handler->OnKeyDown(KeyCode::One);
            }

            break;
        case 0x03:
            if (handler) {
                handler->OnKeyDown(KeyCode::Two);
            }

            break;
        case 0x04:
            if (handler) {
                handler->OnKeyDown(KeyCode::Three);
            }

            break;
        case 0x05:
            if (handler) {
                handler->OnKeyDown(KeyCode::Four);
            }

            break;
        case 0x06:
            if (handler) {
                handler->OnKeyDown(KeyCode::Five);
            }

            break;
        case 0x07:
            if (handler) {
                handler->OnKeyDown(KeyCode::Six);
            }

            break;
        case 0x08:
            if (handler) {
                handler->OnKeyDown(KeyCode::Seven);
            }

            break;
        case 0x09:
            if (handler) {
                handler->OnKeyDown(KeyCode::Eight);
            }

            break;
        case 0x0A:
            if (handler) {
                handler->OnKeyDown(KeyCode::Nine);
            }

            break;
        case 0x0B:
            if (handler) {
                handler->OnKeyDown(KeyCode::Zero);
            }

            break;
        case 0x0C:
            if (handler) {
                handler->OnKeyDown(KeyCode::Minus);
            }

            break;
        case 0x0D:
            // TODO: VERIFY
            if (handler) {
                handler->OnKeyDown(KeyCode::Equals);
            }
            
            break;
        case 0x10:
            if (handler) {
                handler->OnKeyDown(KeyCode::Q);
            }

            break;
        case 0x11:
            if (handler) {
                handler->OnKeyDown(KeyCode::W);
            }

            break;
        case 0x12:
            if (handler) {
                handler->OnKeyDown(KeyCode::E);
            }

            break;
        case 0x13:
            if (handler) {
                handler->OnKeyDown(KeyCode::R);
            }

            break;
        case 0x14:
            if (handler) {
                handler->OnKeyDown(KeyCode::T);
            }

            break;
        case 0x15:
            if (handler) {
                handler->OnKeyDown(KeyCode::Y);
            }

            break;
        case 0x16:
            if (handler) {
                handler->OnKeyDown(KeyCode::U);
            }

            break;
        case 0x17:
            if (handler) {
                handler->OnKeyDown(KeyCode::I);
            }

            break;
        case 0x18:
            if (handler) {
                handler->OnKeyDown(KeyCode::O);
            }

            break;
        case 0x19:
            if (handler) {
                handler->OnKeyDown(KeyCode::P);
            }

            break;
        case 0x1A:
            if (handler) {
                handler->OnKeyDown(KeyCode::LeftBracket);
            }

            break;
        case 0x1B:
            if (handler) {
                handler->OnKeyDown(KeyCode::RightBracket);
            }

            break;
        case 0x1C:
            if (handler) {
                handler->OnKeyDown(KeyCode::Enter);
            }

            break;
        case 0x1E:
            if (handler) {
                handler->OnKeyDown(KeyCode::A);
            }

            break;
        case 0x1F:
            if (handler) {
                handler->OnKeyDown(KeyCode::S);
            }

            break;
        case 0x20:
            if (handler) {
                handler->OnKeyDown(KeyCode::D);
            }

            break;
        case 0x21:
            if (handler) {
                handler->OnKeyDown(KeyCode::F);
            }

            break;
        case 0x22:
            if (handler) {
                handler->OnKeyDown(KeyCode::G);
            }

            break;
        case 0x23:
            if (handler) {
                handler->OnKeyDown(KeyCode::H);
            }

            break;
        case 0x24:
            if (handler) {
                handler->OnKeyDown(KeyCode::J);
            }

            break;
        case 0x25:
            if (handler) {
                handler->OnKeyDown(KeyCode::K);
            }

            break;
        case 0x26:
            if (handler) {
                handler->OnKeyDown(KeyCode::L);
            }

            break;
        case 0x27:
            if (handler) {
                handler->OnKeyDown(KeyCode::Semicolon);
            }

            break;
        case 0x28:
            if (handler) {
                handler->OnKeyDown(KeyCode::Quote);
            }

            break;
        case 0x2C:
            if (handler) {
                handler->OnKeyDown(KeyCode::Z);
            }

            break;
        case 0x2D:
            if (handler) {
                handler->OnKeyDown(KeyCode::X);
            }

            break;
        case 0x2E:
            if (handler) {
                handler->OnKeyDown(KeyCode::C);
            }

            break;
        case 0x2F:
            if (handler) {
                handler->OnKeyDown(KeyCode::V);
            }

            break;
        case 0x30:
            if (handler) {
                handler->OnKeyDown(KeyCode::B);
            }

            break;
        case 0x31:
            if (handler) {
                handler->OnKeyDown(KeyCode::N);
            }

            break;
        case 0x32:
            if (handler) {
                handler->OnKeyDown(KeyCode::M);
            }

            break;
        case 0x33:
            if (handler) {
                handler->OnKeyDown(KeyCode::Comma);
            }

            break;
        case 0x34:
            if (handler) {
                handler->OnKeyDown(KeyCode::Period);
            }

            break;
        case 0x35:
            if (handler) {
                handler->OnKeyDown(KeyCode::Slash);
            }

            break;
        case 0x39:
            if (handler) {
                handler->OnKeyDown(KeyCode::Space);
            }

            break;
        default:
            // Code for printing unhandled interrupts.
            std::cout << "Keyboard " << key << " ";
            break;
        }
    }

    return esp;
}

/**
 * keyboard.cpp
 * 
 * Copyright (c) 2019 Giovanni Giacomo. All Rights Reserved.
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file.
 *
 */

#include "drivers/keyboard.hpp"

using namespace cassio;
using namespace cassio::drivers;
using namespace cassio::hardware;

/** KeyboardEventHandler Methods */

void KeyboardEventHandler::OnKeyDown(KeyCode key) {}

void KeyboardEventHandler::OnKeyUp(KeyCode key) {}

/** KeyboardDriver Methods */

KeyboardCommandByte KeyboardDriver::readCommandByte() {
    KeyboardCommandByte status;

    // Requests current command byte.
    cmd.write(KeyboardCommand::ReadCommandByte);

    status.byte = data.read();
    return status;
}

KeyboardDriver::KeyboardDriver(KeyboardEventHandler* han)
    : Driver(DriverType::KeyboardController), cmd(PortType::KeyboardControllerCommand), data(PortType::KeyboardControllerData), handler(han) {}

void KeyboardDriver::activate() {
    // Cleanup keystrokes before OS starts.
    while (cmd.read() & 0x1) {
        data.read();
    }

    // Enable communication with the keyboard.
    cmd.write(KeyboardCommand::EnableKeyboardInterface);

    // Read and modify current command byte.
    KeyboardCommandByte status = readCommandByte();
    status.keyboard_interrupt = true;
    status.disable_keyboard = false;

    // Set modified command byte to be the new one.
    cmd.write(KeyboardCommand::WriteCommandByte);
    data.write(status.byte);

    // Enables keyboard.
    data.write(0xF4);
}

u32 KeyboardDriver::handleInterrupt(u32 esp) {
    // PIC requires that key be fetched from data port.
    u8 key = data.read();

    // Check for a handler.
    if (!handler) {
        return esp;
    }

    // Ignore key release interrupts.
    if (key < 0x80) {
        switch (key) {
        // Ignore interrupts about NumLock, State, etc.
        case 0x45: case 0xC5: case 0xFA: break;
        case 0x02:
            handler->OnKeyDown(KeyCode::One);
            break;
        case 0x03:
            handler->OnKeyDown(KeyCode::Two);
            break;
        case 0x04:
            handler->OnKeyDown(KeyCode::Three);
            break;
        case 0x05:
            handler->OnKeyDown(KeyCode::Four);
            break;
        case 0x06:
            handler->OnKeyDown(KeyCode::Five);
            break;
        case 0x07:
            handler->OnKeyDown(KeyCode::Six);
            break;
        case 0x08:
            handler->OnKeyDown(KeyCode::Seven);
            break;
        case 0x09:
            handler->OnKeyDown(KeyCode::Eight);
            break;
        case 0x0A:
            handler->OnKeyDown(KeyCode::Nine);
            break;
        case 0x0B:
            handler->OnKeyDown(KeyCode::Zero);
            break;
        case 0x0C:
            handler->OnKeyDown(KeyCode::Minus);
            break;
        case 0x0D:
            handler->OnKeyDown(KeyCode::Equals);            
            break;
        case 0x10:
            handler->OnKeyDown(KeyCode::Q);
            break;
        case 0x11:
            handler->OnKeyDown(KeyCode::W);
            break;
        case 0x12:
            handler->OnKeyDown(KeyCode::E);
            break;
        case 0x13:
            handler->OnKeyDown(KeyCode::R);
            break;
        case 0x14:
            handler->OnKeyDown(KeyCode::T);
            break;
        case 0x15:
            handler->OnKeyDown(KeyCode::Y);
            break;
        case 0x16:
            handler->OnKeyDown(KeyCode::U);
            break;
        case 0x17:
            handler->OnKeyDown(KeyCode::I);
            break;
        case 0x18:
            handler->OnKeyDown(KeyCode::O);
            break;
        case 0x19:
            handler->OnKeyDown(KeyCode::P);
            break;
        case 0x1A:
            handler->OnKeyDown(KeyCode::LeftBracket);
            break;
        case 0x1B:
            handler->OnKeyDown(KeyCode::RightBracket);
            break;
        case 0x1C:
            handler->OnKeyDown(KeyCode::Enter);
            break;
        case 0x1E:
            handler->OnKeyDown(KeyCode::A);
            break;
        case 0x1F:
            handler->OnKeyDown(KeyCode::S);
            break;
        case 0x20:
            handler->OnKeyDown(KeyCode::D);
            break;
        case 0x21:
            handler->OnKeyDown(KeyCode::F);
            break;
        case 0x22:
            handler->OnKeyDown(KeyCode::G);
            break;
        case 0x23:
            handler->OnKeyDown(KeyCode::H);
            break;
        case 0x24:
            handler->OnKeyDown(KeyCode::J);
            break;
        case 0x25:
            handler->OnKeyDown(KeyCode::K);
            break;
        case 0x26:
            handler->OnKeyDown(KeyCode::L);
            break;
        case 0x27:
            handler->OnKeyDown(KeyCode::Semicolon);
            break;
        case 0x28:
            handler->OnKeyDown(KeyCode::Quote);
            break;
        case 0x2C:
            handler->OnKeyDown(KeyCode::Z);
            break;
        case 0x2D:
            handler->OnKeyDown(KeyCode::X);
            break;
        case 0x2E:
            handler->OnKeyDown(KeyCode::C);
            break;
        case 0x2F:
            handler->OnKeyDown(KeyCode::V);
            break;
        case 0x30:
            handler->OnKeyDown(KeyCode::B);
            break;
        case 0x31:
            handler->OnKeyDown(KeyCode::N);
            break;
        case 0x32:
            handler->OnKeyDown(KeyCode::M);
            break;
        case 0x33:
            handler->OnKeyDown(KeyCode::Comma);
            break;
        case 0x34:
            handler->OnKeyDown(KeyCode::Period);
            break;
        case 0x35:
            handler->OnKeyDown(KeyCode::Slash);
            break;
        case 0x39:
            handler->OnKeyDown(KeyCode::Space);
            break;
        default:
            // Code for printing unhandled interrupts.
            std::cout << "Keyboard " << key << " ";
            break;
        }
    }

    return esp;
}

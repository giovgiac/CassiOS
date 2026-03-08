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

/** Scancode-to-KeyCode lookup table for PS/2 scan set 1.
 *  Index is the raw scancode byte. A zero entry means no mapping (modifier or unused).
 */
const KeyCode KeyboardDriver::scancode_table[0x59] = {
    /* 0x00 */  static_cast<KeyCode>(0),
    /* 0x01 */  KeyCode::Escape,
    /* 0x02 */  KeyCode::One,
    /* 0x03 */  KeyCode::Two,
    /* 0x04 */  KeyCode::Three,
    /* 0x05 */  KeyCode::Four,
    /* 0x06 */  KeyCode::Five,
    /* 0x07 */  KeyCode::Six,
    /* 0x08 */  KeyCode::Seven,
    /* 0x09 */  KeyCode::Eight,
    /* 0x0A */  KeyCode::Nine,
    /* 0x0B */  KeyCode::Zero,
    /* 0x0C */  KeyCode::Minus,
    /* 0x0D */  KeyCode::Equals,
    /* 0x0E */  KeyCode::Backspace,
    /* 0x0F */  KeyCode::Tab,
    /* 0x10 */  KeyCode::q,
    /* 0x11 */  KeyCode::w,
    /* 0x12 */  KeyCode::e,
    /* 0x13 */  KeyCode::r,
    /* 0x14 */  KeyCode::t,
    /* 0x15 */  KeyCode::y,
    /* 0x16 */  KeyCode::u,
    /* 0x17 */  KeyCode::i,
    /* 0x18 */  KeyCode::o,
    /* 0x19 */  KeyCode::p,
    /* 0x1A */  KeyCode::LeftBracket,
    /* 0x1B */  KeyCode::RightBracket,
    /* 0x1C */  KeyCode::Enter,
    /* 0x1D */  static_cast<KeyCode>(0),    // Left Ctrl (modifier)
    /* 0x1E */  KeyCode::a,
    /* 0x1F */  KeyCode::s,
    /* 0x20 */  KeyCode::d,
    /* 0x21 */  KeyCode::f,
    /* 0x22 */  KeyCode::g,
    /* 0x23 */  KeyCode::h,
    /* 0x24 */  KeyCode::j,
    /* 0x25 */  KeyCode::k,
    /* 0x26 */  KeyCode::l,
    /* 0x27 */  KeyCode::Semicolon,
    /* 0x28 */  KeyCode::Quote,
    /* 0x29 */  KeyCode::Backquote,
    /* 0x2A */  static_cast<KeyCode>(0),    // Left Shift (modifier)
    /* 0x2B */  KeyCode::BackSlash,
    /* 0x2C */  KeyCode::z,
    /* 0x2D */  KeyCode::x,
    /* 0x2E */  KeyCode::c,
    /* 0x2F */  KeyCode::v,
    /* 0x30 */  KeyCode::b,
    /* 0x31 */  KeyCode::n,
    /* 0x32 */  KeyCode::m,
    /* 0x33 */  KeyCode::Comma,
    /* 0x34 */  KeyCode::Period,
    /* 0x35 */  KeyCode::Slash,
    /* 0x36 */  static_cast<KeyCode>(0),    // Right Shift (modifier)
    /* 0x37 */  static_cast<KeyCode>(0),    // Keypad *
    /* 0x38 */  static_cast<KeyCode>(0),    // Left Alt (modifier)
    /* 0x39 */  KeyCode::Space,
    /* 0x3A */  static_cast<KeyCode>(0),    // Caps Lock (modifier)
    /* 0x3B */  KeyCode::F1,
    /* 0x3C */  KeyCode::F2,
    /* 0x3D */  KeyCode::F3,
    /* 0x3E */  KeyCode::F4,
    /* 0x3F */  KeyCode::F5,
    /* 0x40 */  KeyCode::F6,
    /* 0x41 */  KeyCode::F7,
    /* 0x42 */  KeyCode::F8,
    /* 0x43 */  KeyCode::F9,
    /* 0x44 */  KeyCode::F10,
    /* 0x45 */  static_cast<KeyCode>(0),    // Num Lock
    /* 0x46 */  static_cast<KeyCode>(0),    // Scroll Lock
    /* 0x47 */  static_cast<KeyCode>(0),    // Keypad 7 / Home
    /* 0x48 */  static_cast<KeyCode>(0),    // Keypad 8 / Up
    /* 0x49 */  static_cast<KeyCode>(0),    // Keypad 9 / PgUp
    /* 0x4A */  static_cast<KeyCode>(0),    // Keypad -
    /* 0x4B */  static_cast<KeyCode>(0),    // Keypad 4 / Left
    /* 0x4C */  static_cast<KeyCode>(0),    // Keypad 5
    /* 0x4D */  static_cast<KeyCode>(0),    // Keypad 6 / Right
    /* 0x4E */  static_cast<KeyCode>(0),    // Keypad +
    /* 0x4F */  static_cast<KeyCode>(0),    // Keypad 1 / End
    /* 0x50 */  static_cast<KeyCode>(0),    // Keypad 2 / Down
    /* 0x51 */  static_cast<KeyCode>(0),    // Keypad 3 / PgDn
    /* 0x52 */  static_cast<KeyCode>(0),    // Keypad 0 / Ins
    /* 0x53 */  static_cast<KeyCode>(0),    // Keypad . / Del
    /* 0x54 */  static_cast<KeyCode>(0),
    /* 0x55 */  static_cast<KeyCode>(0),
    /* 0x56 */  static_cast<KeyCode>(0),
    /* 0x57 */  KeyCode::F11,
    /* 0x58 */  KeyCode::F12
};

KeyCode KeyboardDriver::resolveShift(KeyCode key) {
    switch (key) {
    // Letters: lowercase -> uppercase.
    case KeyCode::a:            return KeyCode::A;
    case KeyCode::b:            return KeyCode::B;
    case KeyCode::c:            return KeyCode::C;
    case KeyCode::d:            return KeyCode::D;
    case KeyCode::e:            return KeyCode::E;
    case KeyCode::f:            return KeyCode::F;
    case KeyCode::g:            return KeyCode::G;
    case KeyCode::h:            return KeyCode::H;
    case KeyCode::i:            return KeyCode::I;
    case KeyCode::j:            return KeyCode::J;
    case KeyCode::k:            return KeyCode::K;
    case KeyCode::l:            return KeyCode::L;
    case KeyCode::m:            return KeyCode::M;
    case KeyCode::n:            return KeyCode::N;
    case KeyCode::o:            return KeyCode::O;
    case KeyCode::p:            return KeyCode::P;
    case KeyCode::q:            return KeyCode::Q;
    case KeyCode::r:            return KeyCode::R;
    case KeyCode::s:            return KeyCode::S;
    case KeyCode::t:            return KeyCode::T;
    case KeyCode::u:            return KeyCode::U;
    case KeyCode::v:            return KeyCode::V;
    case KeyCode::w:            return KeyCode::W;
    case KeyCode::x:            return KeyCode::X;
    case KeyCode::y:            return KeyCode::Y;
    case KeyCode::z:            return KeyCode::Z;

    // Number and symbol keys.
    case KeyCode::One:          return KeyCode::Exclamation;
    case KeyCode::Two:          return KeyCode::At;
    case KeyCode::Three:        return KeyCode::Hash;
    case KeyCode::Four:         return KeyCode::Dollar;
    case KeyCode::Five:         return KeyCode::Percent;
    case KeyCode::Six:          return KeyCode::Caret;
    case KeyCode::Seven:        return KeyCode::Ampersand;
    case KeyCode::Eight:        return KeyCode::Asterisk;
    case KeyCode::Nine:         return KeyCode::LeftParenthesis;
    case KeyCode::Zero:         return KeyCode::RightParenthesis;
    case KeyCode::Minus:        return KeyCode::Underscore;
    case KeyCode::Equals:       return KeyCode::Plus;
    case KeyCode::LeftBracket:  return KeyCode::LeftCurly;
    case KeyCode::RightBracket: return KeyCode::RightCurly;
    case KeyCode::BackSlash:    return KeyCode::Pipe;
    case KeyCode::Semicolon:    return KeyCode::Colon;
    case KeyCode::Quote:        return KeyCode::DoubleQuote;
    case KeyCode::Backquote:    return KeyCode::Tilde;
    case KeyCode::Comma:        return KeyCode::LessThan;
    case KeyCode::Period:       return KeyCode::GreaterThan;
    case KeyCode::Slash:        return KeyCode::Question;
    default:                    return key;
    }
}

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
    : Driver(DriverType::KeyboardController),
      cmd(PortType::KeyboardControllerCommand),
      data(PortType::KeyboardControllerData),
      handler(han),
      shift_held(false),
      ctrl_held(false),
      alt_held(false),
      caps_lock_on(false) {}

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

    // Enables keyboard and reads the ACK response so it does not remain
    // in the output buffer where mouse.activate() could consume it.
    data.write(0xF4);
    data.read();
}

u32 KeyboardDriver::handleInterrupt(u32 esp) {
    u8 raw = data.read();

    if (!handler) {
        return esp;
    }

    bool released = raw & 0x80;
    u8 scancode = raw & 0x7F;

    // Handle modifier key state changes.
    switch (static_cast<ScanCode>(scancode)) {
    case ScanCode::LeftShift:
    case ScanCode::RightShift:
        shift_held = !released;
        return esp;
    case ScanCode::LeftCtrl:
        ctrl_held = !released;
        return esp;
    case ScanCode::LeftAlt:
        alt_held = !released;
        return esp;
    case ScanCode::CapsLock:
        if (!released) {
            caps_lock_on = !caps_lock_on;
        }
        return esp;
    case ScanCode::NumLock:
        return esp;
    default:
        break;
    }

    // Only dispatch key-down events for non-modifier keys.
    if (released) {
        return esp;
    }

    // Look up the KeyCode from the scancode table.
    if (scancode < 0x59) {
        KeyCode key = scancode_table[scancode];
        if (static_cast<u8>(key) != 0) {
            bool is_letter = (key >= KeyCode::a && key <= KeyCode::z);

            // Letters: shift XOR caps_lock produces uppercase.
            // Symbols/numbers: shift alone produces the shifted variant.
            if (is_letter ? (shift_held != caps_lock_on) : shift_held) {
                key = resolveShift(key);
            }

            handler->OnKeyDown(key);
        }
    }

    return esp;
}

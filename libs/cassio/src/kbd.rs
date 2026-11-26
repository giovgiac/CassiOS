// kbd.rs -- keyboard key codes
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The keyboard service delivers one byte per key. Printable keys are
// their ASCII character; the constants below name the others, matching
// std::kbd::KeyCode in the C++ client.

pub const BACKSPACE: u8 = 0x08;
pub const TAB: u8 = 0x09;
pub const ENTER: u8 = 0x0D;
pub const ESCAPE: u8 = 0x1B;
pub const DELETE: u8 = 0x7F;
pub const F1: u8 = 0x80;
pub const F2: u8 = 0x81;
pub const F3: u8 = 0x82;
pub const F4: u8 = 0x83;
pub const F5: u8 = 0x84;
pub const F6: u8 = 0x85;
pub const F7: u8 = 0x86;
pub const F8: u8 = 0x87;
pub const F9: u8 = 0x88;
pub const F10: u8 = 0x89;
pub const F11: u8 = 0x8A;
pub const F12: u8 = 0x8B;
pub const LEFT_ARROW: u8 = 0x8C;
pub const RIGHT_ARROW: u8 = 0x8D;

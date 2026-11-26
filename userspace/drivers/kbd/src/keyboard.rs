// keyboard.rs -- scancode set 1 decoder, key buffer and waiting readers
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use alloc::collections::VecDeque;

use cassio::kbd;

// Scancodes with special handling.
const LEFT_SHIFT: u8 = 0x2A;
const RIGHT_SHIFT: u8 = 0x36;
const CAPS_LOCK: u8 = 0x3A;
const EXTENDED: u8 = 0xE0;
const RELEASED: u8 = 0x80;

// Extended (E0-prefixed) scancodes.
const EXT_LEFT: u8 = 0x4B;
const EXT_RIGHT: u8 = 0x4D;
const EXT_DELETE: u8 = 0x53;

/// Keys buffered before new ones are dropped.
const MAX_BUFFERED: usize = 255;

/// Unshifted key code per scancode; 0 for modifiers (Ctrl and Alt are not
/// used yet) and unmapped keys.
#[rustfmt::skip]
const SCANCODES: [u8; 0x59] = [
    // 0x00
    0, kbd::ESCAPE, b'1', b'2', b'3', b'4', b'5', b'6',
    b'7', b'8', b'9', b'0', b'-', b'=', kbd::BACKSPACE, kbd::TAB,
    // 0x10
    b'q', b'w', b'e', b'r', b't', b'y', b'u', b'i',
    b'o', b'p', b'[', b']', kbd::ENTER, 0, b'a', b's',
    // 0x20
    b'd', b'f', b'g', b'h', b'j', b'k', b'l', b';',
    b'\'', b'`', 0, b'\\', b'z', b'x', b'c', b'v',
    // 0x30
    b'b', b'n', b'm', b',', b'.', b'/', 0, 0,
    0, b' ', 0, kbd::F1, kbd::F2, kbd::F3, kbd::F4, kbd::F5,
    // 0x40: F6-F10, then keypad keys (unmapped)
    kbd::F6, kbd::F7, kbd::F8, kbd::F9, kbd::F10, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    // 0x50
    0, 0, 0, 0, 0, 0, 0, kbd::F11,
    kbd::F12,
];

/// Decodes scancodes into key codes, buffers them, and hands them to
/// readers in the order they asked.
pub struct Keyboard {
    left_shift: bool,
    right_shift: bool,
    caps_lock: bool,
    extended: bool,
    keys: VecDeque<u8>,
    readers: VecDeque<u32>,
}

impl Keyboard {
    pub const fn new() -> Self {
        Self {
            left_shift: false,
            right_shift: false,
            caps_lock: false,
            extended: false,
            keys: VecDeque::new(),
            readers: VecDeque::new(),
        }
    }

    /// Queue `pid` for the next key, then serve whoever can be served.
    /// `reply` sends a key to a reader and returns whether it arrived.
    pub fn request(&mut self, pid: u32, reply: impl FnMut(u32, u8) -> bool) {
        if self.readers.try_reserve(1).is_ok() {
            self.readers.push_back(pid);
        }
        self.serve(reply);
    }

    /// Give buffered keys to waiting readers, oldest first. A key stays
    /// buffered if its reader is gone (the reply fails).
    pub fn serve(&mut self, mut reply: impl FnMut(u32, u8) -> bool) {
        while let (Some(&pid), Some(&key)) = (self.readers.front(), self.keys.front()) {
            self.readers.pop_front();
            if reply(pid, key) {
                self.keys.pop_front();
            }
        }
    }

    /// Feed one byte from the PS/2 data port.
    pub fn handle_scancode(&mut self, raw: u8) {
        if raw == EXTENDED {
            self.extended = true;
            return;
        }

        let released = raw & RELEASED != 0;
        let scancode = raw & !RELEASED;

        if self.extended {
            self.extended = false;
            if !released {
                match scancode {
                    EXT_LEFT => self.push(kbd::LEFT_ARROW),
                    EXT_RIGHT => self.push(kbd::RIGHT_ARROW),
                    EXT_DELETE => self.push(kbd::DELETE),
                    _ => {}
                }
            }
            return;
        }

        match scancode {
            LEFT_SHIFT => self.left_shift = !released,
            RIGHT_SHIFT => self.right_shift = !released,
            CAPS_LOCK if !released => self.caps_lock = !self.caps_lock,
            _ if released => {}
            _ => {
                let key = SCANCODES.get(scancode as usize).copied().unwrap_or(0);
                if key != 0 {
                    let shift_held = self.left_shift || self.right_shift;
                    let shifted = if key.is_ascii_lowercase() {
                        shift_held != self.caps_lock
                    } else {
                        shift_held
                    };
                    self.push(if shifted { shift(key) } else { key });
                }
            }
        }
    }

    /// Buffer a key, dropping it if the buffer is full or out of memory.
    fn push(&mut self, key: u8) {
        if self.keys.len() < MAX_BUFFERED && self.keys.try_reserve(1).is_ok() {
            self.keys.push_back(key);
        }
    }
}

/// The key produced with shift held (US layout).
fn shift(key: u8) -> u8 {
    match key {
        b'a'..=b'z' => key.to_ascii_uppercase(),
        b'1' => b'!',
        b'2' => b'@',
        b'3' => b'#',
        b'4' => b'$',
        b'5' => b'%',
        b'6' => b'^',
        b'7' => b'&',
        b'8' => b'*',
        b'9' => b'(',
        b'0' => b')',
        b'-' => b'_',
        b'=' => b'+',
        b'[' => b'{',
        b']' => b'}',
        b'\\' => b'|',
        b';' => b':',
        b'\'' => b'"',
        b'`' => b'~',
        b',' => b'<',
        b'.' => b'>',
        b'/' => b'?',
        _ => key,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn feed(keyboard: &mut Keyboard, scancodes: &[u8]) {
        for &raw in scancodes {
            keyboard.handle_scancode(raw);
        }
    }

    fn keys(scancodes: &[u8]) -> Vec<u8> {
        let mut keyboard = Keyboard::new();
        feed(&mut keyboard, scancodes);
        keyboard.keys.into_iter().collect()
    }

    #[test]
    fn scancode_to_key() {
        assert_eq!(keys(&[0x1E]), b"a");
    }

    #[test]
    fn shift_letter_uppercase() {
        assert_eq!(keys(&[LEFT_SHIFT, 0x1E]), b"A");
    }

    #[test]
    fn shift_symbol() {
        assert_eq!(keys(&[RIGHT_SHIFT, 0x02]), b"!");
    }

    #[test]
    fn caps_lock_toggles_letters_only() {
        // Caps Lock on: letters upper, digits unchanged; Shift inverts
        // letters back to lower and shifts digits.
        assert_eq!(
            keys(&[CAPS_LOCK, 0x1E, 0x02, LEFT_SHIFT, 0x1E, 0x02]),
            b"A1a!"
        );
    }

    #[test]
    fn caps_lock_toggles_off() {
        assert_eq!(
            keys(&[CAPS_LOCK, CAPS_LOCK | RELEASED, CAPS_LOCK, 0x1E]),
            b"a"
        );
    }

    #[test]
    fn release_ignored() {
        assert_eq!(keys(&[0x1E, 0x1E | RELEASED]), b"a");
    }

    #[test]
    fn modifier_release_clears_state() {
        assert_eq!(keys(&[LEFT_SHIFT, LEFT_SHIFT | RELEASED, 0x1E]), b"a");
    }

    #[test]
    fn extended_keys() {
        assert_eq!(
            keys(&[
                EXTENDED, EXT_LEFT, EXTENDED, EXT_RIGHT, EXTENDED, EXT_DELETE
            ]),
            [kbd::LEFT_ARROW, kbd::RIGHT_ARROW, kbd::DELETE]
        );
    }

    #[test]
    fn extended_release_ignored() {
        assert_eq!(keys(&[EXTENDED, EXT_LEFT | RELEASED]), b"");
    }

    #[test]
    fn control_keys() {
        assert_eq!(
            keys(&[0x1C, 0x0E, 0x0F, 0x01]),
            [kbd::ENTER, kbd::BACKSPACE, kbd::TAB, kbd::ESCAPE]
        );
    }

    #[test]
    fn function_keys() {
        assert_eq!(
            keys(&[0x3B, 0x44, 0x57, 0x58]),
            [kbd::F1, kbd::F10, kbd::F11, kbd::F12]
        );
    }

    #[test]
    fn releasing_one_shift_keeps_the_other() {
        assert_eq!(
            keys(&[LEFT_SHIFT, RIGHT_SHIFT, RIGHT_SHIFT | RELEASED, 0x1E]),
            b"A"
        );
    }

    #[test]
    fn multiple_keys_in_order() {
        assert_eq!(keys(&[0x23, 0x17]), b"hi");
    }

    #[test]
    fn unmapped_and_out_of_range_ignored() {
        assert_eq!(keys(&[0x47, 0x59, 0x7F]), b"");
    }

    #[test]
    fn shift_resolves_all_symbols() {
        let pairs = [
            (b'1', b'!'),
            (b'2', b'@'),
            (b'3', b'#'),
            (b'4', b'$'),
            (b'5', b'%'),
            (b'6', b'^'),
            (b'7', b'&'),
            (b'8', b'*'),
            (b'9', b'('),
            (b'0', b')'),
            (b'-', b'_'),
            (b'=', b'+'),
            (b'[', b'{'),
            (b']', b'}'),
            (b'\\', b'|'),
            (b';', b':'),
            (b'\'', b'"'),
            (b'`', b'~'),
            (b',', b'<'),
            (b'.', b'>'),
            (b'/', b'?'),
        ];
        for (key, shifted) in pairs {
            assert_eq!(shift(key), shifted);
        }
    }

    #[test]
    fn full_buffer_drops_new_keys() {
        let mut scancodes = vec![0x1E; MAX_BUFFERED];
        scancodes.push(0x30);
        assert_eq!(keys(&scancodes), [b'a'; MAX_BUFFERED]);
    }

    #[test]
    fn reader_waits_for_a_key() {
        let mut keyboard = Keyboard::new();
        let mut sent = Vec::new();
        keyboard.request(7, |pid, key| {
            sent.push((pid, key));
            true
        });
        assert!(sent.is_empty());

        feed(&mut keyboard, &[0x1E]);
        keyboard.serve(|pid, key| {
            sent.push((pid, key));
            true
        });
        assert_eq!(sent, [(7, b'a')]);
        assert!(keyboard.keys.is_empty() && keyboard.readers.is_empty());
    }

    #[test]
    fn buffered_key_goes_to_reader_at_once() {
        let mut keyboard = Keyboard::new();
        feed(&mut keyboard, &[0x1E]);
        let mut sent = Vec::new();
        keyboard.request(7, |pid, key| {
            sent.push((pid, key));
            true
        });
        assert_eq!(sent, [(7, b'a')]);
    }

    #[test]
    fn readers_served_in_order() {
        let mut keyboard = Keyboard::new();
        let mut sent = Vec::new();
        keyboard.request(7, |_, _| true);
        keyboard.request(8, |_, _| true);
        feed(&mut keyboard, &[0x23, 0x17]);
        keyboard.serve(|pid, key| {
            sent.push((pid, key));
            true
        });
        assert_eq!(sent, [(7, b'h'), (8, b'i')]);
    }

    #[test]
    fn failed_reply_keeps_the_key() {
        let mut keyboard = Keyboard::new();
        keyboard.request(7, |_, _| true);
        keyboard.request(8, |_, _| true);
        feed(&mut keyboard, &[0x1E]);
        // Reader 7 has exited: its reply fails, so the key goes to 8.
        let mut sent = Vec::new();
        keyboard.serve(|pid, key| {
            sent.push((pid, key));
            pid != 7
        });
        assert_eq!(sent, [(7, b'a'), (8, b'a')]);
        assert!(keyboard.keys.is_empty() && keyboard.readers.is_empty());
    }
}

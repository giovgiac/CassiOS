// mouse.rs -- PS/2 mouse packet parser and state tracker
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

// First-byte flags of a PS/2 packet.
const BUTTONS: u8 = 0x07;
const ALWAYS_ONE: u8 = 0x08;
const X_SIGN: u8 = 0x10;
const Y_SIGN: u8 = 0x20;
const OVERFLOW: u8 = 0xC0;

/// Assembles 3-byte PS/2 packets and accumulates movement between reads.
pub struct Mouse {
    buffer: [u8; 3],
    offset: usize,
    buttons: u8,
    dx: i32,
    dy: i32,
}

impl Mouse {
    pub const fn new() -> Self {
        Self {
            buffer: [0; 3],
            offset: 0,
            buttons: 0,
            dx: 0,
            dy: 0,
        }
    }

    /// Feed one byte from the PS/2 data port. A complete packet updates
    /// the buttons and adds its movement to the accumulated deltas.
    pub fn handle_byte(&mut self, byte: u8) {
        // A packet's first byte always has bit 3 set. Skipping bytes until
        // one does resynchronises after a lost byte. Overflow packets are
        // skipped too: their movement is meaningless, and rejecting the
        // overflow bits stops movement bytes like 0xFF (-1) being taken
        // for a header.
        if self.offset == 0 && byte & (ALWAYS_ONE | OVERFLOW) != ALWAYS_ONE {
            return;
        }

        self.buffer[self.offset] = byte;
        self.offset = (self.offset + 1) % 3;

        if self.offset == 0 {
            let flags = self.buffer[0];
            self.buttons = flags & BUTTONS;
            self.dx += delta(self.buffer[1], flags & X_SIGN != 0);
            // PS/2 reports y upwards; screen y grows downwards.
            self.dy -= delta(self.buffer[2], flags & Y_SIGN != 0);
        }
    }

    /// Return (buttons, dx, dy) and reset the deltas.
    pub fn read_state(&mut self) -> (u8, i32, i32) {
        let state = (self.buttons, self.dx, self.dy);
        self.dx = 0;
        self.dy = 0;
        state
    }
}

/// A 9-bit two's complement movement value: `low` plus the sign bit from
/// the packet's first byte.
fn delta(low: u8, negative: bool) -> i32 {
    if negative {
        low as i32 - 0x100
    } else {
        low as i32
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn feed(mouse: &mut Mouse, packet: [u8; 3]) {
        for byte in packet {
            mouse.handle_byte(byte);
        }
    }

    #[test]
    fn initial_state() {
        let mouse = Mouse::new();
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (0, 0, 0));
    }

    #[test]
    fn single_packet_movement() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x08, 5, 3]);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (0, 5, -3));
    }

    #[test]
    fn negative_movement() {
        let mut mouse = Mouse::new();
        // Both sign bits set: dx = -5, raw dy = -2.
        feed(&mut mouse, [0x38, 0xFB, 0xFE]);
        assert_eq!((mouse.dx, mouse.dy), (-5, 2));
    }

    #[test]
    fn large_movement_uses_sign_bits() {
        let mut mouse = Mouse::new();
        // dx = +200 (sign clear), raw dy = -200 (sign set).
        feed(&mut mouse, [0x28, 0xC8, 0x38]);
        assert_eq!((mouse.dx, mouse.dy), (200, 200));
    }

    #[test]
    fn overflow_header_is_skipped() {
        let mut mouse = Mouse::new();
        mouse.handle_byte(0x49);
        feed(&mut mouse, [0x09, 10, 5]);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (1, 10, -5));
    }

    #[test]
    fn misaligned_negative_dx_is_not_a_header() {
        let mut mouse = Mouse::new();
        // Stream shifted by one while moving left: dx = 0xFF (-1) arrives
        // where a header is expected and must not be parsed as one.
        mouse.handle_byte(0xFF);
        mouse.handle_byte(0x00);
        feed(&mut mouse, [0x18, 0xFF, 0x00]);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (0, -1, 0));
    }

    #[test]
    fn buttons() {
        for button in [0x01, 0x02, 0x04] {
            let mut mouse = Mouse::new();
            feed(&mut mouse, [ALWAYS_ONE | button, 0, 0]);
            assert_eq!(mouse.buttons, button);
        }
    }

    #[test]
    fn resyncs_after_lost_byte() {
        let mut mouse = Mouse::new();
        // A stray movement byte without bit 3 is skipped, then a full
        // packet parses normally.
        mouse.handle_byte(0x05);
        feed(&mut mouse, [0x09, 10, 5]);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (1, 10, -5));
    }

    #[test]
    fn accumulate_movement() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x08, 10, 5]);
        feed(&mut mouse, [0x08, 3, 2]);
        assert_eq!((mouse.dx, mouse.dy), (13, -7));
    }

    #[test]
    fn read_state_resets_deltas() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x09, 10, 5]);
        assert_eq!(mouse.read_state(), (1, 10, -5));
        // Deltas reset; buttons persist.
        assert_eq!(mouse.read_state(), (1, 0, 0));
    }

    #[test]
    fn incomplete_packet_no_update() {
        let mut mouse = Mouse::new();
        mouse.handle_byte(0x09);
        mouse.handle_byte(10);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (0, 0, 0));
    }
}

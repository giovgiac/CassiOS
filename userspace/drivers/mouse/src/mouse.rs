// mouse.rs -- PS/2 mouse packet parser and state tracker
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

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
        self.buffer[self.offset] = byte;
        self.offset = (self.offset + 1) % 3;

        if self.offset == 0 {
            self.buttons = self.buffer[0] & 0x07;
            self.dx += self.buffer[1] as i8 as i32;
            // PS/2 reports y upwards; screen y grows downwards.
            self.dy -= self.buffer[2] as i8 as i32;
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
        feed(&mut mouse, [0x00, 5, 3]);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (0, 5, -3));
    }

    #[test]
    fn negative_movement() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x00, 0xFB, 0xFE]); // dx = -5, raw dy = -2
        assert_eq!((mouse.dx, mouse.dy), (-5, 2));
    }

    #[test]
    fn buttons() {
        for button in [0x01, 0x02, 0x04] {
            let mut mouse = Mouse::new();
            feed(&mut mouse, [button, 0, 0]);
            assert_eq!(mouse.buttons, button);
        }
    }

    #[test]
    fn button_byte_masks_flag_bits() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x09, 0, 0]); // bit 3 is always set by PS/2
        assert_eq!(mouse.buttons, 0x01);
    }

    #[test]
    fn accumulate_movement() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x00, 10, 5]);
        feed(&mut mouse, [0x00, 3, 2]);
        assert_eq!((mouse.dx, mouse.dy), (13, -7));
    }

    #[test]
    fn read_state_resets_deltas() {
        let mut mouse = Mouse::new();
        feed(&mut mouse, [0x01, 10, 5]);
        assert_eq!(mouse.read_state(), (1, 10, -5));
        // Deltas reset; buttons persist.
        assert_eq!(mouse.read_state(), (1, 0, 0));
    }

    #[test]
    fn incomplete_packet_no_update() {
        let mut mouse = Mouse::new();
        mouse.handle_byte(0x01);
        mouse.handle_byte(10);
        assert_eq!((mouse.buttons, mouse.dx, mouse.dy), (0, 0, 0));
    }
}

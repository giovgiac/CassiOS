// device.rs -- the PS/2 mouse on the shared controller
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::ps2::{self, Controller};

use crate::mouse::Mouse;

/// The mouse, reached through the auxiliary port of the PS/2 controller.
pub struct Device {
    ps2: Controller,
}

impl Device {
    pub const fn new() -> Self {
        Self {
            ps2: Controller::new(),
        }
    }

    /// Enable mouse data reporting. The keyboard driver has already enabled
    /// the auxiliary interface and its IRQ, so the mouse never reads the
    /// controller configuration while the keyboard is live. Returns false
    /// if the mouse does not respond.
    pub fn activate(&self) -> bool {
        self.try_activate().is_some()
    }

    /// Feed every pending mouse byte to `mouse`. Keyboard bytes are left
    /// for the keyboard driver.
    pub fn drain(&self, mouse: &mut Mouse) {
        const MOUSE_BYTE: u8 = ps2::OUTPUT_FULL | ps2::AUX_DATA;
        while self.ps2.status_is(MOUSE_BYTE, MOUSE_BYTE) {
            mouse.handle_byte(self.ps2.read());
        }
    }

    fn try_activate(&self) -> Option<()> {
        self.ps2.send_command(ps2::WRITE_AUX)?;
        self.ps2.write_data(ps2::ENABLE_DEVICE)?;
        let mouse_byte = ps2::OUTPUT_FULL | ps2::AUX_DATA;
        (self.ps2.read_data(mouse_byte, mouse_byte)? == ps2::ACK).then_some(())
    }
}

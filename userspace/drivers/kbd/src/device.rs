// device.rs -- the PS/2 keyboard on the shared controller
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::ps2::{self, Controller};

use crate::keyboard::Keyboard;

/// Status bits of a byte from the keyboard: output full, aux data clear.
const KBD_BYTE_MASK: u8 = ps2::OUTPUT_FULL | ps2::AUX_DATA;
const KBD_BYTE: u8 = ps2::OUTPUT_FULL;

/// The keyboard, reached through the first port of the PS/2 controller.
pub struct Device {
    ps2: Controller,
}

impl Device {
    pub const fn new() -> Self {
        Self {
            ps2: Controller::new(),
        }
    }

    /// Configure the controller for both devices (keyboard and auxiliary
    /// interfaces and their IRQs) and enable keyboard scanning. The mouse
    /// driver relies on this and only enables its own reporting. Returns
    /// false if the controller or the keyboard does not respond.
    pub fn activate(&self) -> bool {
        self.try_activate().is_some()
    }

    /// Feed every pending keyboard byte to `keyboard`. Mouse bytes are left
    /// for the mouse driver. The controller raises IRQ 1 only while a
    /// keyboard byte is at the head of its buffer, so keyboard bytes queued
    /// behind a mouse byte wait for the mouse driver to read it.
    pub fn drain(&self, keyboard: &mut Keyboard) {
        while self.ps2.status_is(KBD_BYTE_MASK, KBD_BYTE) {
            keyboard.handle_scancode(self.ps2.read());
        }
    }

    fn try_activate(&self) -> Option<()> {
        // Discard stale bytes so the replies below are ours.
        self.ps2.flush();
        self.ps2.send_command(ps2::ENABLE_KBD)?;
        self.ps2.send_command(ps2::ENABLE_AUX)?;
        self.ps2.update_config(|config| {
            (config | ps2::CONFIG_KBD_IRQ | ps2::CONFIG_AUX_IRQ) & !ps2::CONFIG_KBD_DISABLED
        })?;

        self.ps2.write_data(ps2::ENABLE_DEVICE)?;
        (self.ps2.read_data(KBD_BYTE_MASK, KBD_BYTE)? == ps2::ACK).then_some(())
    }
}

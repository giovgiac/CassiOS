// controller.rs -- PS/2 controller access for the mouse
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use cassio::io::{Port, PortType};

use crate::mouse::Mouse;

// Status register bits.
const OUTPUT_FULL: u8 = 0x01;
const INPUT_FULL: u8 = 0x02;
const AUX_DATA: u8 = 0x20;

// Controller commands.
const ENABLE_AUX: u8 = 0xA8;
const READ_CONFIG: u8 = 0x20;
const WRITE_CONFIG: u8 = 0x60;
const WRITE_AUX: u8 = 0xD4;

// Configuration byte bit that enables IRQ 12.
const CONFIG_AUX_IRQ: u8 = 0x02;

// Mouse command: enable data reporting, and the mouse's acknowledgement.
const ENABLE_REPORTING: u8 = 0xF4;
const ACK: u8 = 0xFA;

// Status polls before giving up on the controller (well over the
// milliseconds a real 8042 or mouse needs to respond).
const TIMEOUT: u32 = 100_000;

/// The PS/2 controller's command/status and data ports.
pub struct Controller {
    command: Port<u8>,
    data: Port<u8>,
}

impl Controller {
    pub const fn new() -> Self {
        Self {
            command: Port::new(PortType::KbdCommand),
            data: Port::new(PortType::KbdData),
        }
    }

    /// Enable the auxiliary device, its IRQ, and mouse data reporting.
    /// Returns false if the controller or the mouse does not respond.
    pub fn activate(&self) -> bool {
        self.try_activate().is_some()
    }

    /// Feed every pending mouse byte to `mouse`. Keyboard bytes are left
    /// for the keyboard driver.
    pub fn drain(&self, mouse: &mut Mouse) {
        while self.has(OUTPUT_FULL | AUX_DATA) {
            mouse.handle_byte(unsafe { self.data.read() });
        }
    }

    fn try_activate(&self) -> Option<()> {
        // Discard stale bytes so the replies below are ours. Bounded, since
        // a missing controller reads as 0xFF (every status bit set).
        for _ in 0..TIMEOUT {
            if !self.has(OUTPUT_FULL) {
                break;
            }
            unsafe { self.data.read() };
        }

        self.send_command(ENABLE_AUX)?;

        self.send_command(READ_CONFIG)?;
        let config = self.read_data(OUTPUT_FULL)? | CONFIG_AUX_IRQ;
        self.send_command(WRITE_CONFIG)?;
        self.write_data(config)?;

        self.send_command(WRITE_AUX)?;
        self.write_data(ENABLE_REPORTING)?;
        (self.read_data(OUTPUT_FULL | AUX_DATA)? == ACK).then_some(())
    }

    fn has(&self, bits: u8) -> bool {
        unsafe { self.command.read() & bits == bits }
    }

    /// Poll until `status & mask == expected`.
    fn wait(&self, mask: u8, expected: u8) -> Option<()> {
        (0..TIMEOUT)
            .any(|_| unsafe { self.command.read() } & mask == expected)
            .then_some(())
    }

    fn send_command(&self, command: u8) -> Option<()> {
        self.wait(INPUT_FULL, 0)?;
        unsafe { self.command.write(command) };
        Some(())
    }

    fn write_data(&self, value: u8) -> Option<()> {
        self.wait(INPUT_FULL, 0)?;
        unsafe { self.data.write(value) };
        Some(())
    }

    /// Read a data byte once the status has all of `bits` set.
    fn read_data(&self, bits: u8) -> Option<u8> {
        self.wait(bits, bits)?;
        Some(unsafe { self.data.read() })
    }
}

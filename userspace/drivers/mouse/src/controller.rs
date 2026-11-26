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

// Mouse command: enable data reporting.
const ENABLE_REPORTING: u8 = 0xF4;

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
    pub fn activate(&self) {
        self.send_command(ENABLE_AUX);

        self.send_command(READ_CONFIG);
        let config = self.read_data() | CONFIG_AUX_IRQ;
        self.send_command(WRITE_CONFIG);
        self.write_data(config);

        self.send_command(WRITE_AUX);
        self.write_data(ENABLE_REPORTING);
        self.read_data(); // ACK
    }

    /// Feed every pending mouse byte to `mouse`. Keyboard bytes are left
    /// for the keyboard driver.
    pub fn drain(&self, mouse: &mut Mouse) {
        while self.status() & (OUTPUT_FULL | AUX_DATA) == OUTPUT_FULL | AUX_DATA {
            mouse.handle_byte(unsafe { self.data.read() });
        }
    }

    fn status(&self) -> u8 {
        unsafe { self.command.read() }
    }

    fn send_command(&self, command: u8) {
        while self.status() & INPUT_FULL != 0 {}
        unsafe { self.command.write(command) };
    }

    fn write_data(&self, value: u8) {
        while self.status() & INPUT_FULL != 0 {}
        unsafe { self.data.write(value) };
    }

    fn read_data(&self) -> u8 {
        while self.status() & OUTPUT_FULL == 0 {}
        unsafe { self.data.read() }
    }
}

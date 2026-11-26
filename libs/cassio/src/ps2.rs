// ps2.rs -- PS/2 (8042) controller access
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The keyboard and mouse drivers share one controller: keyboard bytes
// arrive with AUX_DATA clear, mouse bytes with it set. Every wait is
// bounded so a missing or unresponsive device cannot hang a driver.

// Status register bits.
pub const OUTPUT_FULL: u8 = 0x01;
pub const INPUT_FULL: u8 = 0x02;
pub const AUX_DATA: u8 = 0x20;

// Controller commands.
pub const READ_CONFIG: u8 = 0x20;
pub const WRITE_CONFIG: u8 = 0x60;
pub const ENABLE_AUX: u8 = 0xA8;
pub const ENABLE_KBD: u8 = 0xAE;
pub const WRITE_AUX: u8 = 0xD4;

// Configuration byte bits.
pub const CONFIG_KBD_IRQ: u8 = 0x01;
pub const CONFIG_AUX_IRQ: u8 = 0x02;
pub const CONFIG_KBD_DISABLED: u8 = 0x10;

// Device command to enable scanning or reporting, and its acknowledgement.
pub const ENABLE_DEVICE: u8 = 0xF4;
pub const ACK: u8 = 0xFA;

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use super::*;
    use crate::io::{Port, PortType};

    // Status polls before giving up (well over the milliseconds a real
    // 8042 or device needs to respond).
    const TIMEOUT: u32 = 100_000;

    /// The controller's command/status and data ports.
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

        /// Whether the status register has `status & mask == expected`.
        pub fn status_is(&self, mask: u8, expected: u8) -> bool {
            unsafe { self.command.read() & mask == expected }
        }

        /// Read the data port without waiting. Use after `status_is` has
        /// confirmed a byte from the right device.
        pub fn read(&self) -> u8 {
            unsafe { self.data.read() }
        }

        /// Discard pending output bytes. Bounded, since a missing
        /// controller reads as 0xFF (every status bit set).
        pub fn flush(&self) {
            for _ in 0..TIMEOUT {
                if self.status_is(OUTPUT_FULL, 0) {
                    break;
                }
                self.read();
            }
        }

        pub fn send_command(&self, command: u8) -> Option<()> {
            self.wait(INPUT_FULL, 0)?;
            unsafe { self.command.write(command) };
            Some(())
        }

        pub fn write_data(&self, value: u8) -> Option<()> {
            self.wait(INPUT_FULL, 0)?;
            unsafe { self.data.write(value) };
            Some(())
        }

        /// Read a data byte once `status & mask == expected`.
        pub fn read_data(&self, mask: u8, expected: u8) -> Option<u8> {
            self.wait(mask, expected)?;
            Some(self.read())
        }

        /// Read the configuration byte, apply `update`, and write it back.
        pub fn update_config(&self, update: impl FnOnce(u8) -> u8) -> Option<()> {
            self.send_command(READ_CONFIG)?;
            let config = self.read_data(OUTPUT_FULL, OUTPUT_FULL)?;
            self.send_command(WRITE_CONFIG)?;
            self.write_data(update(config))
        }

        fn wait(&self, mask: u8, expected: u8) -> Option<()> {
            (0..TIMEOUT)
                .any(|_| self.status_is(mask, expected))
                .then_some(())
        }
    }
}

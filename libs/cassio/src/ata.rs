// ata.rs -- ATA service client
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use crate::ipc::{self, Message, message_type};
use crate::{ns, os};

pub const SECTOR_SIZE: usize = 512;

/// A connection to the ATA disk service.
pub struct Ata {
    pid: u32,
}

impl Ata {
    /// Connect to the ATA service, retrying the lookup a bounded number of
    /// times (500 tries, sleeping 10 ms between them) while it starts.
    /// Without it, every sector request fails.
    pub fn connect() -> Self {
        const TRIES: u32 = 500;
        const WAIT_MS: u32 = 10;
        for _ in 0..TRIES {
            let pid = ns::lookup(b"ata");
            if pid != 0 {
                return Self { pid };
            }
            os::sleep(WAIT_MS);
        }
        os::write(2, b"ata: service not found\n");
        Self { pid: 0 }
    }

    /// Read sector `lba` into `buf`.
    pub fn read_sector(&self, lba: u32, buf: &mut [u8; SECTOR_SIZE]) -> bool {
        let mut msg = Message {
            kind: message_type::ATA_READ,
            arg1: lba,
            ..Message::default()
        };
        ipc::send_into(self.pid, &mut msg, buf) >= 0 && msg.arg1 == 0
    }

    /// Write `buf` to sector `lba`.
    pub fn write_sector(&self, lba: u32, buf: &[u8; SECTOR_SIZE]) -> bool {
        let mut msg = Message {
            kind: message_type::ATA_WRITE,
            arg1: lba,
            ..Message::default()
        };
        ipc::send(self.pid, &mut msg, buf) >= 0 && msg.arg1 == 0
    }
}

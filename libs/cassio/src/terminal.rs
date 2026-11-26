// terminal.rs -- terminal service client
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Output is sent with notify (no round trip); only the cursor requests
// wait for the reply. Nothing reaches the screen until `flush`.

use crate::ipc::{self, Message, message_type};
use crate::ns;

/// Largest TerminalWrite the service accepts, in bytes.
pub const WRITE_MAX: usize = 256;

/// A connection to the terminal service.
pub struct Terminal {
    pid: u32,
}

impl Terminal {
    /// Connect to the terminal service, retrying the lookup a bounded
    /// number of times while it starts. Without it, output goes nowhere.
    pub fn connect() -> Self {
        Self {
            pid: ns::connect(b"terminal"),
        }
    }

    pub fn putchar(&self, ch: u8) {
        self.notify(message_type::TERMINAL_PUTCHAR, ch as u32, &[]);
    }

    pub fn write(&self, text: &[u8]) {
        for chunk in text.chunks(WRITE_MAX) {
            self.notify(message_type::TERMINAL_WRITE, chunk.len() as u32, chunk);
        }
    }

    pub fn clear(&self) {
        self.notify(message_type::TERMINAL_CLEAR, 0, &[]);
    }

    pub fn flush(&self) {
        self.notify(message_type::TERMINAL_FLUSH, 0, &[]);
    }

    /// Move the cursor; waits until the terminal has handled the output
    /// sent before it.
    pub fn set_cursor(&self, col: u32, row: u32) {
        let mut msg = Message {
            kind: message_type::TERMINAL_SET_CURSOR,
            arg1: col,
            arg2: row,
            ..Message::default()
        };
        ipc::send(self.pid, &mut msg, &[]);
    }

    /// The cursor position as (column, row), after the output sent before.
    pub fn cursor(&self) -> (u32, u32) {
        let mut msg = Message {
            kind: message_type::TERMINAL_GET_CURSOR,
            ..Message::default()
        };
        if ipc::send(self.pid, &mut msg, &[]) < 0 {
            return (0, 0);
        }
        (msg.arg1, msg.arg2)
    }

    fn notify(&self, kind: u32, arg1: u32, data: &[u8]) {
        let mut msg = Message {
            kind,
            arg1,
            ..Message::default()
        };
        ipc::notify(self.pid, &mut msg, data);
    }
}

// display.rs -- display service client
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Drawing requests are fire-and-forget notifies (the service never
// answers them); only get-info waits for a reply. Nothing reaches the
// screen until `flush`.

use crate::gfx::Color;
use crate::ipc::{self, Message, message_type};
use crate::{ns, os};

/// A connection to the display service.
pub struct Display {
    pid: u32,
}

impl Display {
    /// Connect to the display service, retrying the lookup a bounded
    /// number of times (500 tries, sleeping 10 ms between them) while it
    /// starts. Without it, requests go nowhere and `size` is 0x0, so
    /// callers keep working.
    pub fn connect() -> Self {
        const TRIES: u32 = 500;
        const WAIT_MS: u32 = 10;
        for _ in 0..TRIES {
            let pid = ns::lookup(b"display");
            if pid != 0 {
                return Self { pid };
            }
            os::sleep(WAIT_MS);
        }
        os::write(2, b"display: service not found\n");
        Self { pid: 0 }
    }

    /// Width and height in pixels.
    pub fn size(&self) -> (u32, u32) {
        let mut msg = Message {
            kind: message_type::DISPLAY_GET_INFO,
            ..Message::default()
        };
        if ipc::send(self.pid, &mut msg, &[]) < 0 {
            return (0, 0);
        }
        (msg.arg1, msg.arg2)
    }

    pub fn fill_rect(&self, x: u32, y: u32, w: u32, h: u32, color: Color) {
        self.notify(message_type::DISPLAY_FILL_RECT, [x, y, w, h, color]);
    }

    pub fn draw_char(&self, x: u32, y: u32, ch: u8, fg: Color, bg: Color) {
        self.notify(message_type::DISPLAY_DRAW_CHAR, [x, y, ch as u32, fg, bg]);
    }

    /// Scroll the screen content up by `rows` pixels.
    pub fn scroll(&self, rows: u32, color: Color) {
        self.notify(message_type::DISPLAY_SCROLL, [rows, color, 0, 0, 0]);
    }

    /// Copy everything drawn so far to the screen.
    pub fn flush(&self) {
        self.notify(message_type::DISPLAY_FLUSH, [0; 5]);
    }

    fn notify(&self, kind: u32, [arg1, arg2, arg3, arg4, arg5]: [u32; 5]) {
        let mut msg = Message {
            kind,
            arg1,
            arg2,
            arg3,
            arg4,
            arg5,
        };
        ipc::notify(self.pid, &mut msg, &[]);
    }
}

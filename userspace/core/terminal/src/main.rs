// main.rs -- terminal service entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Text console on top of the display service. Clients write characters
// (usually by notify) and control the cursor; every send is answered with
// the cursor position. The screen only updates on TerminalFlush.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

mod terminal;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::display::Display;
    use cassio::ipc::{self, Message, message_type};
    use cassio::ns;

    use terminal::Terminal;

    // Largest TerminalWrite, in bytes.
    const WRITE_MAX: usize = 256;

    // Registered first, as clients look the terminal up once and early;
    // their requests queue until the loop below starts receiving.
    ns::register_name(b"terminal");

    let display = Display::connect();
    let (width, height) = display.size();
    let mut terminal = Terminal::new(display, width, height);
    for &ch in b"Welcome to the Cassio Operating System!\n" {
        terminal.putchar(ch);
    }
    terminal.present();

    loop {
        // Zeroed per request: receive does not report how many bytes
        // arrived, so a short write must not reuse stale text.
        let mut text = [0u8; WRITE_MAX];
        let mut msg = Message::default();
        let sender = ipc::receive(&mut msg, &mut text);

        match msg.kind {
            message_type::TERMINAL_PUTCHAR => terminal.putchar(msg.arg1 as u8),
            message_type::TERMINAL_WRITE => {
                let len = (msg.arg1 as usize).min(WRITE_MAX);
                text[..len].iter().for_each(|&ch| terminal.putchar(ch));
            }
            message_type::TERMINAL_CLEAR => terminal.clear(),
            message_type::TERMINAL_SET_CURSOR => {
                terminal.set_cursor(msg.arg1 & 0xFF, msg.arg2 & 0xFF)
            }
            message_type::TERMINAL_FLUSH => terminal.present(),
            _ => {}
        }

        if sender > 0 {
            let (col, row) = terminal.cursor();
            let mut reply = Message {
                arg1: col,
                arg2: row,
                ..Message::default()
            };
            ipc::reply(sender as u32, &mut reply, &[]);
        }
    }
}

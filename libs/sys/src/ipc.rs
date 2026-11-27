// ipc.rs -- IPC message layout and message types
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

/// A 24-byte IPC message: type plus five arguments.
#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct Message {
    pub kind: u32,
    pub arg1: u32,
    pub arg2: u32,
    pub arg3: u32,
    pub arg4: u32,
    pub arg5: u32,
}

pub mod message_type {
    // Kernel.
    pub const IRQ_NOTIFY: u32 = 1;

    // Nameserver.
    pub const NS_REGISTER: u32 = 2;
    pub const NS_LOOKUP: u32 = 3;
    pub const NS_LIST_ALL: u32 = 4;

    // Keyboard.
    pub const KBD_READ: u32 = 5;

    // VFS.
    pub const VFS_MKDIR: u32 = 6;
    pub const VFS_REMOVE: u32 = 7;
    pub const VFS_OPEN: u32 = 8;
    pub const VFS_READ: u32 = 9;
    pub const VFS_WRITE: u32 = 10;
    pub const VFS_LIST: u32 = 11;
    pub const VFS_STAT: u32 = 12;
    pub const VFS_CLOSE: u32 = 13;

    // Mouse.
    pub const MOUSE_READ: u32 = 14;

    // ATA.
    pub const ATA_READ: u32 = 15;
    pub const ATA_WRITE: u32 = 16;

    // Display. Fill rect, draw rect, draw char, scroll and flush must be
    // sent with notify: the display never replies to them. Blit and get
    // info are sends and are answered with the display's dimensions.
    pub const DISPLAY_FILL_RECT: u32 = 17;
    pub const DISPLAY_DRAW_RECT: u32 = 18;
    pub const DISPLAY_BLIT: u32 = 19;
    pub const DISPLAY_SCROLL: u32 = 20;
    pub const DISPLAY_FLUSH: u32 = 21;
    pub const DISPLAY_GET_INFO: u32 = 22;
    pub const DISPLAY_DRAW_CHAR: u32 = 23;

    // Terminal. TerminalWrite carries at most 256 bytes of text; clients
    // split longer text. Every send is answered with the cursor position.
    pub const TERMINAL_PUTCHAR: u32 = 24;
    pub const TERMINAL_WRITE: u32 = 25;
    pub const TERMINAL_CLEAR: u32 = 26;
    pub const TERMINAL_SET_CURSOR: u32 = 27;
    pub const TERMINAL_GET_CURSOR: u32 = 28;
    pub const TERMINAL_FLUSH: u32 = 29;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn message_is_24_bytes() {
        assert_eq!(core::mem::size_of::<Message>(), 24);
    }
}

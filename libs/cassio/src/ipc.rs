// ipc.rs -- inter-process communication
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Message layout, message type constants and the send/receive/reply/notify
// syscalls. Data buffers travel in esi, which Rust's asm! cannot name on
// x86-32, so the wrappers swap it in and out by hand. The kernel restores
// every register except eax on return.

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

    // Keyboard.
    pub const KBD_READ: u32 = 4;

    // VFS.
    pub const VFS_MKDIR: u32 = 5;
    pub const VFS_REMOVE: u32 = 6;
    pub const VFS_OPEN: u32 = 7;
    pub const VFS_READ: u32 = 8;
    pub const VFS_WRITE: u32 = 9;
    pub const VFS_LIST: u32 = 10;
    pub const VFS_STAT: u32 = 11;

    // Mouse.
    pub const MOUSE_READ: u32 = 12;

    // ATA.
    pub const ATA_READ: u32 = 13;
    pub const ATA_WRITE: u32 = 14;

    // Nameserver (extended).
    pub const NS_LIST_ALL: u32 = 15;

    // Display.
    pub const DISPLAY_FILL_RECT: u32 = 16;
    pub const DISPLAY_DRAW_RECT: u32 = 17;
    pub const DISPLAY_BLIT: u32 = 18;
    pub const DISPLAY_SCROLL: u32 = 19;
    pub const DISPLAY_FLUSH: u32 = 20;
    pub const DISPLAY_GET_INFO: u32 = 21;
    pub const DISPLAY_DRAW_CHAR: u32 = 22;

    // Terminal.
    pub const TERMINAL_PUTCHAR: u32 = 23;
    pub const TERMINAL_WRITE: u32 = 24;
    pub const TERMINAL_CLEAR: u32 = 25;
    pub const TERMINAL_SET_CURSOR: u32 = 26;
    pub const TERMINAL_GET_CURSOR: u32 = 27;
    pub const TERMINAL_FLUSH: u32 = 28;
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::Message;
    use crate::os::syscall;

    /// Send `msg` to `pid` and block until it replies. The reply
    /// overwrites `msg`. Returns negative on error.
    pub fn send(pid: u32, msg: &mut Message, data: &[u8]) -> i32 {
        transfer(syscall::SEND, pid, msg, data.as_ptr(), data.len())
    }

    /// Reply to a blocked sender, unblocking it.
    pub fn reply(pid: u32, msg: &mut Message, data: &[u8]) -> i32 {
        transfer(syscall::REPLY, pid, msg, data.as_ptr(), data.len())
    }

    /// Enqueue `msg` to `pid` without blocking.
    pub fn notify(pid: u32, msg: &mut Message, data: &[u8]) -> i32 {
        transfer(syscall::NOTIFY, pid, msg, data.as_ptr(), data.len())
    }

    /// Block until a message arrives. Bulk data, if any, lands in `data`.
    /// Returns the sender PID, or negative on error.
    pub fn receive(msg: &mut Message, data: &mut [u8]) -> i32 {
        let ret: i32;
        unsafe {
            asm!(
                "xchg {d}, esi",
                "int 0x80",
                "xchg {d}, esi",
                d = inout(reg) data.as_mut_ptr() => _,
                inlateout("eax") syscall::RECEIVE => ret,
                in("ebx") msg as *mut Message,
                in("edi") data.len(),
            )
        };
        ret
    }

    fn transfer(call: u32, pid: u32, msg: &mut Message, data: *const u8, len: usize) -> i32 {
        let ret: i32;
        unsafe {
            asm!(
                "xchg {d}, esi",
                "int 0x80",
                "xchg {d}, esi",
                d = inout(reg) data => _,
                inlateout("eax") call => ret,
                in("ebx") pid,
                in("ecx") msg as *mut Message,
                in("edi") len,
            )
        };
        ret
    }
}

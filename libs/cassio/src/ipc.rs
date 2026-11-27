// ipc.rs -- inter-process communication
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The send/receive/reply/notify syscalls; the message layout and types
// come from the sys crate. Data buffers travel in esi, which Rust's asm!
// cannot name on x86-32, so the wrappers swap it in and out by hand. The
// kernel restores every register except eax on return.

pub use sys::ipc::{Message, message_type};

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

    /// Like `send`, but the reply's data lands in `buf`, which also carries
    /// the request's data.
    pub fn send_into(pid: u32, msg: &mut Message, buf: &mut [u8]) -> i32 {
        transfer(syscall::SEND, pid, msg, buf.as_mut_ptr(), buf.len())
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

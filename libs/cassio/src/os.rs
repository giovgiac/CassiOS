// os.rs -- syscall numbers and userspace wrappers
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

pub mod syscall {
    pub const SEND: u32 = 0;
    pub const RECEIVE: u32 = 1;
    pub const REPLY: u32 = 2;
    pub const IRQ_REGISTER: u32 = 3;
    pub const WRITE: u32 = 4;
    pub const SLEEP: u32 = 5;
    pub const UPTIME: u32 = 6;
    pub const REBOOT: u32 = 7;
    pub const SHUTDOWN: u32 = 8;
    pub const EXIT: u32 = 9;
    pub const MAP_DEVICE: u32 = 10;
    pub const NOTIFY: u32 = 11;
    pub const MEM_INFO: u32 = 12;
    pub const SBRK: u32 = 13;
    pub const PROC_LIST: u32 = 14;
    pub const EXEC: u32 = 15;
    pub const WAIT_PID: u32 = 16;
    pub const FRAMEBUFFER_INFO: u32 = 17;
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::syscall;

    /// Write bytes to a file descriptor (fd 2 = serial). Returns the
    /// number of bytes written, or negative on error.
    pub fn write(fd: u32, buf: &[u8]) -> i32 {
        let ret: i32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::WRITE => ret,
                in("ebx") fd,
                in("ecx") buf.as_ptr(),
                in("edx") buf.len(),
            )
        };
        ret
    }

    /// Terminate the calling process.
    pub fn exit(code: u32) -> ! {
        unsafe {
            asm!("int 0x80", in("eax") syscall::EXIT, in("ebx") code);
        }
        loop {}
    }

    /// Receive IRQ `irq` as IrqNotify messages. Returns 0 on success,
    /// negative if the IRQ is already registered.
    pub fn irq_register(irq: u32) -> i32 {
        let ret: i32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::IRQ_REGISTER => ret,
                in("ebx") irq,
            )
        };
        ret
    }

    /// Grow the heap by `increment` bytes. Returns the old break, or null
    /// on failure.
    pub fn sbrk(increment: u32) -> *mut u8 {
        let ret: u32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::SBRK => ret,
                in("ebx") increment,
            )
        };
        ret as *mut u8
    }
}

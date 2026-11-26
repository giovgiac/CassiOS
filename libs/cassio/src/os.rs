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

/// VESA framebuffer details reported by the boot loader.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct FramebufferInfo {
    /// Physical address of the framebuffer.
    pub address: u32,
    pub width: u32,
    pub height: u32,
    /// Bytes per scanline.
    pub pitch: u32,
    pub bpp: u32,
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::{FramebufferInfo, syscall};

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

    /// Suspend the calling process for at least `ms` milliseconds.
    pub fn sleep(ms: u32) {
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::SLEEP => _,
                in("ebx") ms,
            )
        };
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

    /// Map `pages` physical pages starting at `physical` to `virt` in the
    /// calling process. Returns 0 on success.
    pub fn map_device(physical: u32, virt: u32, pages: u32) -> i32 {
        let ret: i32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::MAP_DEVICE => ret,
                in("ebx") physical,
                in("ecx") virt,
                in("edx") pages,
            )
        };
        ret
    }

    /// The boot framebuffer, all zero if there is none.
    pub fn framebuffer_info() -> FramebufferInfo {
        let (address, width, height, pitch, bpp): (u32, u32, u32, u32, u32);
        unsafe {
            // The kernel returns bpp in esi, which asm! cannot name; swap it
            // through a scratch register.
            asm!(
                "xchg {bpp}, esi",
                "int 0x80",
                "xchg {bpp}, esi",
                bpp = inout(reg) 0u32 => bpp,
                inlateout("eax") syscall::FRAMEBUFFER_INFO => address,
                lateout("ebx") width,
                lateout("ecx") height,
                lateout("edx") pitch,
            )
        };
        FramebufferInfo {
            address,
            width,
            height,
            pitch,
            bpp,
        }
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

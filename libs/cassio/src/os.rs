// os.rs -- userspace syscall wrappers
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

pub use sys::os::{FramebufferInfo, ProcEntry, TICK_FREQUENCY, syscall};

/// Physical memory in 4 KiB frames, as reported by `mem_info`.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct MemInfo {
    pub total: u32,
    pub used: u32,
    pub free: u32,
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::{FramebufferInfo, MemInfo, ProcEntry, syscall};

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

    /// Timer ticks since boot (TICK_FREQUENCY per second).
    pub fn uptime() -> u32 {
        let ret: u32;
        unsafe { asm!("int 0x80", inlateout("eax") syscall::UPTIME => ret) };
        ret
    }

    pub fn reboot() {
        unsafe { asm!("int 0x80", inlateout("eax") syscall::REBOOT => _) };
    }

    pub fn shutdown() {
        unsafe { asm!("int 0x80", inlateout("eax") syscall::SHUTDOWN => _) };
    }

    pub fn mem_info() -> MemInfo {
        let (total, used, free): (u32, u32, u32);
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::MEM_INFO => total,
                lateout("ebx") used,
                lateout("ecx") free,
            )
        };
        MemInfo { total, used, free }
    }

    /// Fill `out` with the running processes; returns how many it holds.
    pub fn proc_list(out: &mut [ProcEntry]) -> usize {
        let ret: u32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::PROC_LIST => ret,
                in("ebx") out.as_mut_ptr(),
                in("ecx") out.len(),
            )
        };
        (ret as usize).min(out.len())
    }

    /// Start a process from the ELF image `elf`. Returns its PID, or 0 if
    /// it cannot be loaded.
    pub fn exec(elf: &[u8]) -> u32 {
        let ret: u32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::EXEC => ret,
                in("ebx") elf.as_ptr(),
                in("ecx") elf.len(),
            )
        };
        ret
    }

    /// Block until process `pid` exits.
    pub fn waitpid(pid: u32) -> i32 {
        let ret: i32;
        unsafe {
            asm!(
                "int 0x80",
                inlateout("eax") syscall::WAIT_PID => ret,
                in("ebx") pid,
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

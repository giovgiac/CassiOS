// os.rs -- syscall numbers and the structures syscalls return
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

/// Timer ticks per second, as counted by `uptime`.
pub const TICK_FREQUENCY: u32 = 1000;

/// A process as reported by `proc_list`.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ProcEntry {
    pub pid: u32,
    /// 1 Ready, 2 Running, 3 SendBlocked, 4 ReceiveBlocked, 5 WaitBlocked.
    pub state: u32,
    /// Heap size in bytes (0 without a heap).
    pub heap_size: u32,
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn proc_entry_layout_matches_the_kernel() {
        assert_eq!(core::mem::size_of::<ProcEntry>(), 12);
    }
}

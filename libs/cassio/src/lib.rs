// lib.rs -- CassiOS userspace standard library
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Syscall wrappers, IPC, nameserver protocol, global allocator and panic
// handler for Rust userspace programs. Everything that executes int 0x80
// is only compiled for the i586 target, so the pure logic can be tested
// on the host with `cargo test`.

#![no_std]

extern crate alloc;
// memcpy and the other memory functions every program links.
extern crate mem as _;

#[cfg(target_arch = "x86")]
pub mod ata;
#[cfg(target_arch = "x86")]
pub mod display;
pub mod gfx;
pub use sys::io;
pub mod ipc;
pub mod kbd;
pub mod ns;
pub mod os;
pub mod ps2;
#[cfg(target_arch = "x86")]
pub mod terminal;
#[cfg(target_arch = "x86")]
pub mod vfs;

#[cfg(target_arch = "x86")]
mod rt {
    use core::fmt::{self, Write};
    use core::panic::PanicInfo;

    use crate::os;

    struct Serial;

    impl Write for Serial {
        fn write_str(&mut self, s: &str) -> fmt::Result {
            os::write(2, s.as_bytes());
            Ok(())
        }
    }

    #[panic_handler]
    fn panic(info: &PanicInfo) -> ! {
        let _ = writeln!(Serial, "PANIC: {}", info);
        os::exit(1)
    }

    #[global_allocator]
    static HEAP: sys::heap::Heap =
        sys::heap::Heap::with_sbrk(|increment| os::sbrk(increment as u32));

    // The prebuilt alloc crate is compiled with unwinding and references
    // the personality routine and, from its formatting code, the unwinder.
    // With panic = "abort" neither is ever called.
    #[unsafe(no_mangle)]
    extern "C" fn rust_eh_personality() {}

    #[unsafe(no_mangle)]
    extern "C" fn _Unwind_Resume() -> ! {
        os::exit(1)
    }
}

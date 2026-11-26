// lib.rs -- CassiOS userspace standard library
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Syscall wrappers, IPC, nameserver protocol, heap allocator and panic
// handler for Rust userspace programs. Everything that executes int 0x80
// is only compiled for the i586 target, so the pure logic can be tested
// on the host with `cargo test`.

#![no_std]

extern crate alloc;

#[cfg(target_arch = "x86")]
pub mod display;
pub mod gfx;
pub mod heap;
pub mod io;
pub mod ipc;
pub mod kbd;
pub mod ns;
pub mod os;
pub mod ps2;

#[cfg(target_arch = "x86")]
mod rt {
    use core::ffi::c_void;
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
    static HEAP: crate::heap::Heap = crate::heap::Heap::new();

    // libstd_mem provides memcmp but not bcmp, which LLVM emits for
    // slice equality.
    unsafe extern "C" {
        fn memcmp(a: *const c_void, b: *const c_void, n: usize) -> i32;
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn bcmp(a: *const c_void, b: *const c_void, n: usize) -> i32 {
        unsafe { memcmp(a, b, n) }
    }

    // The prebuilt alloc crate is compiled with unwinding and references
    // the personality routine. With panic = "abort" it is never called.
    #[unsafe(no_mangle)]
    extern "C" fn rust_eh_personality() {}
}

// lib.rs -- CassiOS kernel, Rust part
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The kernel is being ported from C++ one subsystem at a time. This crate
// is a staticlib linked into the C++ kernel: C++ calls ported subsystems
// through extern "C" functions, and C++ classes forward to them until
// their callers are ported. Hardware access is only compiled for the i586
// target, so pure logic runs as host tests.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

#[macro_use]
mod ktest;
#[cfg(target_arch = "x86")]
mod sync;

// kernel/src/core; `core` itself names Rust's core library.
#[path = "core"]
mod system {
    pub mod elf;
    pub mod gdt;
}

mod hardware {
    #[cfg(target_arch = "x86")]
    pub mod exception;
    pub mod interrupt;
    pub mod irq;
    pub mod pit;
    #[cfg(target_arch = "x86")]
    pub mod serial;
}

mod memory {
    #[cfg(target_arch = "x86")]
    pub mod heap;
    pub mod multiboot;
    pub mod paging;
    pub mod physical;

    /// Where the direct map of physical memory starts.
    pub const KERNEL_VBASE: u32 = 0xC000_0000;
    /// Physical memory the direct map reaches: the rest of the address
    /// space after KERNEL_VBASE.
    pub const DIRECT_MAP_END: u64 = 0x1_0000_0000 - KERNEL_VBASE as u64;

    /// The direct-map address of physical address `phys`.
    #[cfg(target_arch = "x86")]
    pub fn phys_to_virt(phys: u32) -> *mut u8 {
        (phys + KERNEL_VBASE) as *mut u8
    }
}

/// Run the Rust kernel tests, adding to the counts. Called by the test
/// kernel before the C++ tests, whose runner reports the totals.
#[cfg(all(target_arch = "x86", not(test)))]
#[unsafe(no_mangle)]
extern "C" fn kernel_run_tests(passed: &mut u32, failed: &mut u32) {
    let mut out = &hardware::serial::COM1;
    let groups = [
        hardware::serial::ktests::TESTS,
        system::gdt::ktests::TESTS,
        hardware::interrupt::ktests::TESTS,
        hardware::irq::ktests::TESTS,
        hardware::pit::ktests::TESTS,
        memory::physical::ktests::TESTS,
        memory::heap::ktests::TESTS,
        memory::paging::ktests::TESTS,
        system::elf::ktests::TESTS,
    ];
    ktest::run(&groups, &mut out, passed, failed);
}

#[cfg(all(target_arch = "x86", not(test)))]
mod rt {
    use core::arch::asm;
    use core::ffi::c_void;
    use core::fmt::Write;
    use core::panic::PanicInfo;

    use sys::io::{Port, PortType};

    use crate::hardware::serial::COM1;

    // The prebuilt alloc crate is compiled with unwinding and references
    // the personality routine and, from its formatting code, the unwinder.
    // With panic = "abort" neither is ever called.
    #[unsafe(no_mangle)]
    extern "C" fn rust_eh_personality() {}

    #[unsafe(no_mangle)]
    extern "C" fn _Unwind_Resume() -> ! {
        panic!("unwinding is not supported")
    }

    // libstd_mem provides memcmp but not bcmp, which LLVM emits for
    // slice equality.
    unsafe extern "C" {
        fn memcmp(a: *const c_void, b: *const c_void, n: usize) -> i32;
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn bcmp(a: *const c_void, b: *const c_void, n: usize) -> i32 {
        unsafe { memcmp(a, b, n) }
    }

    #[panic_handler]
    fn panic(info: &PanicInfo) -> ! {
        let _ = writeln!(&COM1, "KERNEL PANIC: {info}");
        // Fails a test run; without QEMU's debug-exit device the write
        // does nothing.
        unsafe { Port::<u8>::new(PortType::QemuDebugExit).write(0x01) };
        loop {
            unsafe { asm!("cli", "hlt") };
        }
    }
}

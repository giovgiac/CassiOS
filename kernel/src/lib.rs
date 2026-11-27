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

#[macro_use]
mod ktest;

mod hardware {
    #[cfg(target_arch = "x86")]
    pub mod serial;
}

/// Run the Rust kernel tests, adding to the counts. Called by the test
/// kernel before the C++ tests, whose runner reports the totals.
#[cfg(all(target_arch = "x86", not(test)))]
#[unsafe(no_mangle)]
extern "C" fn kernel_run_tests(passed: &mut u32, failed: &mut u32) {
    let mut out = &hardware::serial::COM1;
    ktest::run(&[hardware::serial::ktests::TESTS], &mut out, passed, failed);
}

#[cfg(all(target_arch = "x86", not(test)))]
mod rt {
    use core::arch::asm;
    use core::fmt::Write;
    use core::panic::PanicInfo;

    use sys::io::{Port, PortType};

    use crate::hardware::serial::COM1;

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

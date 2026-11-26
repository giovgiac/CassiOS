// main.rs -- userspace test runner
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Booted after the services by `make test-userspace`. Runs the black-box
// tests of every service over IPC, reports to serial, and exits through
// the QEMU debug-exit device: 0 if every test passed, 1 otherwise.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

#[macro_use]
mod harness;

// The black-box tests need the services, so only the target builds them.
#[cfg(target_arch = "x86")]
mod ata;
#[cfg(target_arch = "x86")]
mod display;
#[cfg(target_arch = "x86")]
mod kbd;
#[cfg(target_arch = "x86")]
mod mouse;
#[cfg(target_arch = "x86")]
mod ns;
#[cfg(target_arch = "x86")]
mod system;
#[cfg(target_arch = "x86")]
mod terminal;
#[cfg(target_arch = "x86")]
mod vfs;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::os;

    let failed = harness::run(
        &[
            system::TESTS,
            ns::TESTS,
            kbd::TESTS,
            mouse::TESTS,
            ata::TESTS,
            display::TESTS,
            terminal::TESTS,
            vfs::TESTS,
        ],
        &mut |text| {
            os::write(2, text.as_bytes());
        },
    );
    os::exit((failed > 0) as u32)
}

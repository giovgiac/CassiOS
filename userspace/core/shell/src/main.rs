// main.rs -- shell entry point
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Reads keys from the keyboard service, edits the command line on the
// terminal, and runs commands against the VFS and the kernel.

#![cfg_attr(not(test), no_std)]

extern crate alloc;

mod args;
mod commands;
mod line;
mod path;
#[cfg(target_arch = "x86")]
mod shell;

#[cfg(not(test))]
#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    use cassio::kbd::Kbd;
    use cassio::{ns, os};

    use shell::Shell;

    ns::register_name(b"shell");

    let kbd = Kbd::connect();
    let mut shell = Shell::connect();
    shell.prompt();

    loop {
        match kbd.read() {
            Some(0) => {}
            Some(key) => shell.key(key),
            // No keyboard: wait instead of spinning on failed requests.
            None => os::sleep(100),
        }
    }
}

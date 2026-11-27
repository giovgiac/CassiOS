// exception.rs -- CPU exception handler
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Divide error, invalid opcode, general protection and page faults are
// logged to COM1, fail a QEMU test run through the debug-exit port, and
// halt. The entry stubs are in stub.s.

use core::arch::asm;
use core::fmt::Write;

use sys::io::{Port, PortType};

use crate::hardware::interrupt::set_gate;
use crate::hardware::serial::COM1;

unsafe extern "C" {
    fn exception_stub_0x00();
    fn exception_stub_0x06();
    fn exception_stub_0x0D();
    fn exception_stub_0x0E();
}

/// Point the exception vectors at their stubs.
pub fn install() {
    set_gate(0x00, exception_stub_0x00, 0);
    set_gate(0x06, exception_stub_0x06, 0);
    set_gate(0x0D, exception_stub_0x0D, 0);
    set_gate(0x0E, exception_stub_0x0E, 0);
}

/// Called by the stubs with the vector, the error code (0 for vectors
/// without one) and the saved frame.
#[unsafe(no_mangle)]
extern "C" fn handleException(vector: u32, error_code: u32, _esp: u32) -> u32 {
    let _ = writeln!(&COM1, "EXCEPTION: vector={vector} error_code={error_code}");
    // Fails a QEMU test run instead of hanging it; without the debug-exit
    // device the write does nothing.
    unsafe { Port::<u8>::new(PortType::QemuDebugExit).write(0x01) };
    loop {
        unsafe { asm!("cli", "hlt") };
    }
}

// system.rs -- userspace heap and syscall tests
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The heap tests go through the global allocator (cassio's free list on
// sbrk). Zero-sized allocations never reach it in Rust, so the C++ test
// for alloc(0) has no counterpart.

use alloc::boxed::Box;
use alloc::vec::Vec;

use cassio::os;

use crate::harness::Outcome;

tests!(
    heap_allocations_are_distinct,
    heap_reuses_freed_blocks,
    heap_write_and_read,
    syscall_sleep_waits_for_the_timer,
    syscall_write_serial_returns_length,
    syscall_kernel_buffers_rejected,
    syscall_wrapping_buffers_rejected,
    syscall_unmapped_buffers_rejected,
);

/// The kernel's own image, which no program may read or write through a
/// syscall.
const KERNEL_ADDRESS: u32 = 0xC010_0000;

/// A raw syscall, for buffers the safe wrappers cannot express. esi
/// cannot be an asm! operand on x86-32, so it is swapped in by hand. Every
/// argument register counts as clobbered, since some syscalls (MemInfo,
/// FramebufferInfo) return values in them.
fn raw_syscall(call: u32, ebx: u32, ecx: u32, edx: u32, esi: u32, edi: u32) -> u32 {
    let ret: u32;
    // SAFETY: the kernel validates every argument; the call either fails
    // or acts on the given registers.
    unsafe {
        core::arch::asm!(
            "xchg {s}, esi",
            "int 0x80",
            "xchg {s}, esi",
            s = inout(reg) esi => _,
            inlateout("eax") call => ret,
            inlateout("ebx") ebx => _,
            inlateout("ecx") ecx => _,
            inlateout("edx") edx => _,
            inlateout("edi") edi => _,
        )
    };
    ret
}

fn heap_allocations_are_distinct() -> Outcome {
    let a = Box::new([0u8; 32]);
    let b = Box::new([0u8; 32]);
    check!(a.as_ptr() != b.as_ptr());
    Ok(())
}

fn heap_reuses_freed_blocks() -> Outcome {
    let a = Box::new([0u8; 64]);
    let first = a.as_ptr() as usize;
    drop(a);
    let b = Box::new([0u8; 64]);
    check_eq!(b.as_ptr() as usize, first);
    Ok(())
}

fn heap_write_and_read() -> Outcome {
    let values: Vec<u32> = (0..10).map(|i| i * 7).collect();
    for (i, &v) in values.iter().enumerate() {
        check_eq!(v, i as u32 * 7);
    }
    Ok(())
}

fn syscall_sleep_waits_for_the_timer() -> Outcome {
    // Sleep waits for timer ticks in the kernel, so it must enable
    // interrupts while it waits.
    let before = os::uptime();
    os::sleep(20);
    check!(os::uptime() - before >= 20);
    Ok(())
}

fn syscall_write_serial_returns_length() -> Outcome {
    check_eq!(os::write(2, b""), 0);
    check_eq!(os::write(2, b"test"), 4);
    Ok(())
}

fn syscall_kernel_buffers_rejected() -> Outcome {
    // Exec is checked too, but a kernel image is no ELF, so it would fail
    // either way and has no test here.
    use os::syscall::{NOTIFY, PROC_LIST, WRITE};
    let k = KERNEL_ADDRESS;
    check_eq!(raw_syscall(WRITE, 2, k, 16, 0, 0) as i32, -1);
    // A notify to the nameserver whose message lies in the kernel.
    check_eq!(raw_syscall(NOTIFY, 1, k, 0, 0, 0) as i32, -1);
    check_eq!(raw_syscall(NOTIFY, 1, 0x1000, 0, k, 16) as i32, -1);
    check_eq!(raw_syscall(PROC_LIST, k, 4, 0, 0, 0), 0);
    Ok(())
}

fn syscall_wrapping_buffers_rejected() -> Outcome {
    use os::syscall::{PROC_LIST, WRITE};
    check_eq!(raw_syscall(WRITE, 2, 0x1000, 0xFFFF_F000, 0, 0) as i32, -1);
    // An entry count whose byte size overflows.
    check_eq!(raw_syscall(PROC_LIST, 0x1000, 0x2000_0000, 0, 0, 0), 0);
    Ok(())
}

fn syscall_unmapped_buffers_rejected() -> Outcome {
    use os::syscall::{NOTIFY, PROC_LIST, WRITE};
    // Below the program (linked at 0x400000) nothing is mapped.
    let unmapped = 0x1000;
    check_eq!(raw_syscall(WRITE, 2, unmapped, 4, 0, 0) as i32, -1);
    check_eq!(raw_syscall(NOTIFY, 1, unmapped, 0, 0, 0) as i32, -1);
    check_eq!(raw_syscall(PROC_LIST, unmapped, 4, 0, 0, 0), 0);
    // Two bytes before the program's first page and two inside it.
    check_eq!(raw_syscall(WRITE, 2, 0x003F_FFFE, 4, 0, 0) as i32, -1);
    Ok(())
}

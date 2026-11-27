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
    syscall_map_device_over_mapped_pages_rejected,
    faulting_program_ends_alone,
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

fn syscall_map_device_over_mapped_pages_rejected() -> Outcome {
    use os::syscall::MAP_DEVICE;
    // The program's own first page (linked at 0x400000) is in use.
    check_eq!(
        raw_syscall(MAP_DEVICE, 0xB8000, 0x0040_0000, 1, 0, 0) as i32,
        -1
    );
    Ok(())
}

/// A minimal i386 executable: one segment at 0x00400000 whose code is
/// `code`.
fn executable(code: &[u8]) -> Vec<u8> {
    const HEADER: usize = 52;
    const PROGRAM_HEADER: usize = 32;
    let base: u32 = 0x0040_0000;
    let offset = (HEADER + PROGRAM_HEADER) as u32;
    let mut image = alloc::vec![0u8; HEADER + PROGRAM_HEADER];
    image[..6].copy_from_slice(b"\x7FELF\x01\x01");
    let mut put = |at: usize, bytes: &[u8]| image[at..at + bytes.len()].copy_from_slice(bytes);
    put(16, &2u16.to_le_bytes()); // ET_EXEC
    put(18, &3u16.to_le_bytes()); // EM_386
    put(24, &(base + offset).to_le_bytes()); // entry: the code
    put(28, &(HEADER as u32).to_le_bytes()); // e_phoff
    put(42, &(PROGRAM_HEADER as u16).to_le_bytes());
    put(44, &1u16.to_le_bytes()); // e_phnum
    let size = offset + code.len() as u32;
    // PT_LOAD of the whole file at `base`, so the code sits at base + offset.
    for (i, field) in [1, 0, base, base, size, size].iter().enumerate() {
        put(HEADER + i * 4, &field.to_le_bytes());
    }
    image.extend_from_slice(code);
    image
}

fn faulting_program_ends_alone() -> Outcome {
    // ud2: an invalid opcode in user mode.
    let program = executable(&[0x0F, 0x0B]);
    let pid = os::exec(&program);
    check!(pid != 0);
    // The kernel ends the child (waking us) instead of halting.
    check_eq!(os::waitpid(pid), 0);
    let mut entries = [os::ProcEntry::default(); 32];
    let n = os::proc_list(&mut entries);
    check!(entries[..n].iter().all(|e| e.pid != pid));
    Ok(())
}

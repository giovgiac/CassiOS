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
);

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

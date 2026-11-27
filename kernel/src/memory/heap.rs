// heap.rs -- kernel heap
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// A fixed region of contiguous frames, reached through the direct map,
// managed by the shared free-list allocator. It is the crate's global
// allocator, and C++ operator new and delete forward to kernel_alloc and
// kernel_free.

use core::ptr::null_mut;

use sys::heap::Heap;

use super::phys_to_virt;
use super::physical::{FRAME_SIZE, PHYSICAL};
use crate::sync::Global;

/// Size of the heap region: 1 MiB.
pub const HEAP_FRAMES: usize = 256;

/// The part of the region the heap has not grown into yet.
struct Region {
    next: usize,
    end: usize,
}

static REGION: Global<Region> = Global::new(Region { next: 0, end: 0 });

/// The heap's break: hands out the region front to back.
fn grow(increment: usize) -> *mut u8 {
    REGION.with(|region| {
        if region.end - region.next < increment {
            return null_mut();
        }
        let old = region.next;
        region.next += increment;
        old as *mut u8
    })
}

#[global_allocator]
pub static HEAP: Heap = Heap::with_sbrk(grow);

/// Reserve the heap region. Allocations fail until this runs, and keep
/// failing if no run of free frames is large enough.
pub fn init() {
    let Some(base) = PHYSICAL.with(|p| p.alloc_contiguous(HEAP_FRAMES)) else {
        return;
    };
    let start = phys_to_virt(base) as usize;
    REGION.with(|region| {
        region.next = start;
        region.end = start + HEAP_FRAMES * FRAME_SIZE as usize;
    });
}

#[unsafe(no_mangle)]
extern "C" fn heap_init() {
    init();
}

/// `size` bytes for C++ operator new, or null.
#[unsafe(no_mangle)]
extern "C" fn kernel_alloc(size: usize) -> *mut u8 {
    unsafe { HEAP.allocate(size) }
}

/// # Safety
///
/// `ptr` must come from kernel_alloc, or be null.
#[unsafe(no_mangle)]
unsafe extern "C" fn kernel_free(ptr: *mut u8) {
    unsafe { HEAP.free(ptr) };
}

#[cfg(not(test))]
pub mod ktests {
    use alloc::boxed::Box;
    use alloc::vec;

    use sys::heap::HEADER;

    use super::*;
    use crate::ktest::Outcome;
    use crate::memory::KERNEL_VBASE;

    tests!(
        heap_allocations_are_distinct_and_reused,
        heap_allocate_zero_returns_null,
        heap_splits_and_coalesces_blocks,
        heap_ignores_double_and_foreign_frees,
        heap_large_allocation,
        heap_lies_in_one_run_of_ram,
        heap_backs_box_and_vec,
    );

    fn allocate(size: usize) -> *mut u8 {
        unsafe { HEAP.allocate(size) }
    }

    fn free(ptr: *mut u8) {
        unsafe { HEAP.free(ptr) };
    }

    fn heap_allocations_are_distinct_and_reused() -> Outcome {
        let a = allocate(64);
        let b = allocate(64);
        free(b);
        free(a);
        check!(!a.is_null() && !b.is_null());
        check!(a != b);
        check!(a as u32 >= KERNEL_VBASE);
        let c = allocate(64);
        free(c);
        check_eq!(c, a);
        Ok(())
    }

    fn heap_allocate_zero_returns_null() -> Outcome {
        check!(allocate(0).is_null());
        Ok(())
    }

    fn heap_splits_and_coalesces_blocks() -> Outcome {
        let a = allocate(32);
        let b = allocate(32);
        // The second block directly follows the first.
        let adjacent = b as usize == a as usize + 32 + HEADER;
        free(a);
        free(b);
        // Freed neighbours merge into one block that holds both.
        let c = allocate(64 + HEADER);
        free(c);
        check!(adjacent);
        check_eq!(c, a);
        Ok(())
    }

    fn heap_ignores_double_and_foreign_frees() -> Outcome {
        let a = allocate(64);
        let b = allocate(64);
        free(a);
        free(a);
        let mut local = 0u8;
        free(&mut local);
        free(null_mut());
        // a's block is free once, b's is still in use.
        let c = allocate(64);
        let d = allocate(64);
        free(c);
        free(d);
        free(b);
        check_eq!(c, a);
        check!(d != b);
        Ok(())
    }

    fn heap_large_allocation() -> Outcome {
        let ptr = allocate(4096);
        free(ptr);
        check!(!ptr.is_null());
        Ok(())
    }

    fn heap_lies_in_one_run_of_ram() -> Outcome {
        // More than the RAM below 640 KiB: a heap made of scattered
        // frames would run into VGA memory and the kernel here.
        let size = 768 * 1024;
        let ptr = allocate(size);
        check!(!ptr.is_null());
        let bytes = unsafe { core::slice::from_raw_parts_mut(ptr, size) };
        for (i, byte) in bytes.iter_mut().enumerate() {
            *byte = i as u8;
        }
        let intact = bytes.iter().enumerate().all(|(i, &b)| b == i as u8);
        free(ptr);
        check!(intact);
        Ok(())
    }

    fn heap_backs_box_and_vec() -> Outcome {
        let boxed = Box::new(42u32);
        check_eq!(*boxed, 42);
        let list = vec![7u16; 100];
        check_eq!(list.iter().map(|&x| x as u32).sum::<u32>(), 700);
        Ok(())
    }
}

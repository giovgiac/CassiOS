// heap.rs -- userspace heap allocator
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// First-fit free-list allocator with block splitting and coalescing,
// ported from std::alloc::HeapAllocator. `Heap` wraps it as the global
// allocator and grows it via sbrk, like libstd_heap.

use core::alloc::{GlobalAlloc, Layout};
use core::cell::UnsafeCell;
use core::mem::{align_of, size_of};
use core::ptr::null_mut;

#[repr(C)]
struct BlockHeader {
    size: usize,
    free: bool,
    next: *mut BlockHeader,
}

const HEADER: usize = size_of::<BlockHeader>();

/// Alignment every block payload has without extra work.
const MIN_ALIGN: usize = align_of::<BlockHeader>();

const MIN_BLOCK_SIZE: usize = HEADER + MIN_ALIGN;

/// Free-list allocator over a contiguous region the caller owns.
pub struct FreeList {
    head: *mut BlockHeader,
}

impl FreeList {
    pub const fn empty() -> Self {
        Self { head: null_mut() }
    }

    /// Start managing `size` bytes at `base`, which must be aligned to
    /// MIN_ALIGN.
    pub unsafe fn init(&mut self, base: *mut u8, size: usize) {
        if size <= HEADER {
            return;
        }
        let head = base as *mut BlockHeader;
        unsafe {
            head.write(BlockHeader {
                size: size - HEADER,
                free: true,
                next: null_mut(),
            })
        };
        self.head = head;
    }

    pub fn is_initialized(&self) -> bool {
        !self.head.is_null()
    }

    /// Allocate `size` bytes aligned to MIN_ALIGN. Returns null if no free
    /// block is large enough.
    pub unsafe fn allocate(&mut self, size: usize) -> *mut u8 {
        if size == 0 {
            return null_mut();
        }
        let size = (size + MIN_ALIGN - 1) & !(MIN_ALIGN - 1);

        let mut current = self.head;
        while let Some(block) = unsafe { current.as_mut() } {
            if block.free && block.size >= size {
                let remainder = block.size - size;
                if remainder >= MIN_BLOCK_SIZE {
                    let split =
                        unsafe { (current as *mut u8).add(HEADER + size) } as *mut BlockHeader;
                    unsafe {
                        split.write(BlockHeader {
                            size: remainder - HEADER,
                            free: true,
                            next: block.next,
                        })
                    };
                    block.size = size;
                    block.next = split;
                }
                block.free = false;
                return unsafe { (current as *mut u8).add(HEADER) };
            }
            current = block.next;
        }
        null_mut()
    }

    /// Free a pointer returned by `allocate` and merge adjacent free blocks.
    pub unsafe fn free(&mut self, ptr: *mut u8) {
        if ptr.is_null() {
            return;
        }
        let block = unsafe { &mut *(ptr.sub(HEADER) as *mut BlockHeader) };
        block.free = true;

        let mut current = self.head;
        while let Some(block) = unsafe { current.as_mut() } {
            match unsafe { block.next.as_ref() } {
                Some(next) if block.free && next.free => {
                    block.size += HEADER + next.size;
                    block.next = next.next;
                }
                _ => current = block.next,
            }
        }
    }

    /// Extend the region by `additional` bytes directly after its end.
    pub unsafe fn extend(&mut self, additional: usize) {
        if additional <= HEADER || self.head.is_null() {
            return;
        }
        let mut last = unsafe { &mut *self.head };
        while let Some(next) = unsafe { last.next.as_mut() } {
            last = next;
        }

        if last.free {
            last.size += additional;
        } else {
            let end = unsafe { (last as *mut BlockHeader as *mut u8).add(HEADER + last.size) };
            let block = end as *mut BlockHeader;
            unsafe {
                block.write(BlockHeader {
                    size: additional - HEADER,
                    free: true,
                    next: null_mut(),
                })
            };
            last.next = block;
        }
    }
}

/// Process-wide allocator that grows a FreeList via sbrk.
pub struct Heap {
    list: UnsafeCell<FreeList>,
    sbrk: fn(usize) -> *mut u8,
}

// Userspace processes are single-threaded.
unsafe impl Sync for Heap {}

const PAGE: usize = 4096;

impl Heap {
    #[cfg(target_arch = "x86")]
    pub const fn new() -> Self {
        Self::with_sbrk(|increment| crate::os::sbrk(increment as u32))
    }

    /// Build a heap over a custom break function (returns the old break,
    /// or null on failure).
    pub const fn with_sbrk(sbrk: fn(usize) -> *mut u8) -> Self {
        Self {
            list: UnsafeCell::new(FreeList::empty()),
            sbrk,
        }
    }

    unsafe fn allocate(&self, size: usize) -> *mut u8 {
        let list = unsafe { &mut *self.list.get() };
        if !list.is_initialized() {
            let base = (self.sbrk)(PAGE);
            if base.is_null() {
                return null_mut();
            }
            unsafe { list.init(base, PAGE) };
        }

        let ptr = unsafe { list.allocate(size) };
        if !ptr.is_null() {
            return ptr;
        }

        // Grow the heap and retry.
        let needed = (size + HEADER + PAGE + PAGE - 1) & !(PAGE - 1);
        if (self.sbrk)(needed).is_null() {
            return null_mut();
        }
        unsafe {
            list.extend(needed);
            list.allocate(size)
        }
    }
}

unsafe impl GlobalAlloc for Heap {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        if layout.align() <= MIN_ALIGN {
            return unsafe { self.allocate(layout.size()) };
        }
        // Over-allocate and keep the original pointer just below the
        // aligned one.
        let raw = unsafe { self.allocate(layout.size() + layout.align()) };
        if raw.is_null() {
            return raw;
        }
        let aligned = ((raw as usize + layout.align()) & !(layout.align() - 1)) as *mut u8;
        unsafe { (aligned as *mut *mut u8).sub(1).write(raw) };
        aligned
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        let raw = if layout.align() <= MIN_ALIGN {
            ptr
        } else {
            unsafe { (ptr as *mut *mut u8).sub(1).read() }
        };
        unsafe { (*self.list.get()).free(raw) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[repr(align(16))]
    struct Region([u8; 1024]);

    fn list(region: &mut Region) -> FreeList {
        let mut list = FreeList::empty();
        unsafe { list.init(region.0.as_mut_ptr(), 512) };
        list
    }

    #[test]
    fn allocate_returns_aligned_distinct_blocks() {
        let mut region = Region([0; 1024]);
        let mut list = list(&mut region);
        let a = unsafe { list.allocate(5) };
        let b = unsafe { list.allocate(5) };
        assert!(!a.is_null() && !b.is_null());
        assert_eq!(a as usize % MIN_ALIGN, 0);
        assert!(b as usize >= a as usize + 5);
    }

    #[test]
    fn allocate_zero_returns_null() {
        let mut region = Region([0; 1024]);
        let mut list = list(&mut region);
        assert!(unsafe { list.allocate(0) }.is_null());
    }

    #[test]
    fn allocate_fails_when_exhausted() {
        let mut region = Region([0; 1024]);
        let mut list = list(&mut region);
        assert!(unsafe { list.allocate(512) }.is_null());
    }

    #[test]
    fn free_reuses_and_coalesces() {
        let mut region = Region([0; 1024]);
        let mut list = list(&mut region);
        let a = unsafe { list.allocate(100) };
        let b = unsafe { list.allocate(100) };
        unsafe {
            list.free(a);
            list.free(b);
        }
        // Both blocks merged back into one that fits almost the whole region.
        let c = unsafe { list.allocate(400) };
        assert_eq!(c, a);
    }

    #[test]
    fn extend_grows_region() {
        let mut region = Region([0; 1024]);
        let mut list = list(&mut region);
        let a = unsafe { list.allocate(512 - HEADER) };
        assert!(!a.is_null());
        assert!(unsafe { list.allocate(100) }.is_null());
        unsafe { list.extend(512) };
        assert!(!unsafe { list.allocate(100) }.is_null());
    }

    // A fake break over a static region, for the Heap tests only.
    #[repr(align(4096))]
    struct Arena([u8; 4 * PAGE]);
    static mut ARENA: Arena = Arena([0; 4 * PAGE]);
    static BREAK: core::sync::atomic::AtomicUsize = core::sync::atomic::AtomicUsize::new(0);

    fn fake_sbrk(increment: usize) -> *mut u8 {
        use core::sync::atomic::Ordering;
        let old = BREAK.fetch_add(increment, Ordering::Relaxed);
        if old + increment > 4 * PAGE {
            return null_mut();
        }
        unsafe { (&raw mut ARENA.0).cast::<u8>().add(old) }
    }

    #[test]
    fn heap_grows_and_honours_alignment() {
        let heap = Heap::with_sbrk(fake_sbrk);
        unsafe {
            // Larger than the initial page, so the heap must grow.
            let big = heap.alloc(Layout::from_size_align(6000, 4).unwrap());
            assert!(!big.is_null());
            big.write_bytes(0xAA, 6000);

            let layout = Layout::from_size_align(100, 64).unwrap();
            let aligned = heap.alloc(layout);
            assert!(!aligned.is_null());
            assert_eq!(aligned as usize % 64, 0);
            aligned.write_bytes(0xBB, 100);
            heap.dealloc(aligned, layout);

            // Exhausting the fake break fails cleanly.
            assert!(
                heap.alloc(Layout::from_size_align(5 * PAGE, 4).unwrap())
                    .is_null()
            );
        }
    }
}

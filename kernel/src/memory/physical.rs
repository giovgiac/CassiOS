// physical.rs -- physical frame allocator
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// One bit per 4 KiB frame of the 32-bit physical address space, set when
// the frame is in use. Only available RAM between 1 MiB and the end of the
// kernel's direct map is ever handed out: below 1 MiB lie BIOS data, the
// boot loader's structures and device memory, and the kernel reaches
// frames through the direct map. The C++ PhysicalMemoryManager
// (kernel/include/memory/physical.hpp) forwards here until its callers
// are ported.

use super::DIRECT_MAP_END;
use super::multiboot::{MMAP_AVAILABLE, Region};

pub const FRAME_SIZE: u32 = 4096;

/// Frames below this address are never handed out.
pub const LOW_MEMORY_END: u64 = 0x10_0000;

const FRAMES: usize = 1 << 20;
const BITMAP_BYTES: usize = FRAMES / 8;

pub struct FrameBitmap {
    used: [u8; BITMAP_BYTES],
    /// Frames of usable RAM, as found by `reset`.
    total: u32,
}

impl FrameBitmap {
    /// An empty bitmap, unusable until `reset`. All zeros, so a static
    /// one lives in .bss.
    pub const fn new() -> Self {
        Self {
            used: [0; BITMAP_BYTES],
            total: 0,
        }
    }

    /// Mark every frame used, then free the usable RAM in `regions`.
    pub fn reset(&mut self, regions: impl Iterator<Item = Region>) {
        self.used.fill(0xFF);
        for region in regions.filter(|r| r.kind == MMAP_AVAILABLE) {
            let start = region.base.max(LOW_MEMORY_END);
            let end = region
                .base
                .saturating_add(region.length)
                .min(DIRECT_MAP_END);
            // Only frames wholly inside the region.
            let first = start.div_ceil(FRAME_SIZE as u64);
            let last = end / FRAME_SIZE as u64;
            for frame in first..last {
                self.set(frame as usize, false);
            }
        }
        self.total = self.free_frames();
    }

    /// Mark every frame that overlaps [start, end) used.
    pub fn reserve(&mut self, start: u64, end: u64) {
        let first = start / FRAME_SIZE as u64;
        let last = end.div_ceil(FRAME_SIZE as u64).min(FRAMES as u64);
        for frame in first..last {
            self.set(frame as usize, true);
        }
    }

    /// The address of a free frame, now marked used.
    pub fn alloc(&mut self) -> Option<u32> {
        let byte = self.used.iter().position(|&b| b != 0xFF)?;
        let frame = byte * 8 + self.used[byte].trailing_ones() as usize;
        self.set(frame, true);
        Some(frame as u32 * FRAME_SIZE)
    }

    /// The address of the first run of `count` free frames, now marked
    /// used.
    pub fn alloc_contiguous(&mut self, count: usize) -> Option<u32> {
        let mut run = 0;
        for frame in 0..FRAMES {
            run = if self.is_frame_used(frame) {
                0
            } else {
                run + 1
            };
            if run == count {
                let first = frame + 1 - count;
                for f in first..=frame {
                    self.set(f, true);
                }
                return Some(first as u32 * FRAME_SIZE);
            }
        }
        None
    }

    /// Return the frame at `address` to the free pool.
    pub fn free(&mut self, address: u32) {
        self.set((address / FRAME_SIZE) as usize, false);
    }

    pub fn is_used(&self, address: u32) -> bool {
        self.is_frame_used((address / FRAME_SIZE) as usize)
    }

    pub fn total_frames(&self) -> u32 {
        self.total
    }

    pub fn free_frames(&self) -> u32 {
        self.used.iter().map(|b| b.count_zeros()).sum()
    }

    pub fn used_frames(&self) -> u32 {
        self.total - self.free_frames()
    }

    fn is_frame_used(&self, frame: usize) -> bool {
        self.used[frame / 8] & (1 << (frame % 8)) != 0
    }

    fn set(&mut self, frame: usize, used: bool) {
        if used {
            self.used[frame / 8] |= 1 << (frame % 8);
        } else {
            self.used[frame / 8] &= !(1 << (frame % 8));
        }
    }
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::mem::size_of;

    use super::FrameBitmap;
    use crate::memory::multiboot::{FLAG_MMAP, FLAG_MODS, Info, Module, regions};
    use crate::memory::{KERNEL_VBASE, phys_to_virt};
    use crate::sync::Global;

    pub static PHYSICAL: Global<FrameBitmap> = Global::new(FrameBitmap::new());

    unsafe extern "C" {
        static _kernel_start: u8;
        static _kernel_end: u8;
    }

    /// Build the bitmap from the boot loader's memory map, then reserve
    /// the kernel image and everything the boot loader handed over.
    ///
    /// # Safety
    ///
    /// `info` must be the boot loader's information, with its addresses
    /// reachable through the direct map.
    pub unsafe fn init(info: &Info) {
        if info.flags & FLAG_MMAP == 0 {
            return;
        }
        let map = unsafe {
            core::slice::from_raw_parts(phys_to_virt(info.mmap_addr), info.mmap_length as usize)
        };
        PHYSICAL.with(|bitmap| {
            bitmap.reset(regions(map));

            let physical = |symbol: *const u8| (symbol as u32 - KERNEL_VBASE) as u64;
            let span = |start: u32, length: usize| (start as u64, start as u64 + length as u64);
            // The kernel image includes the bitmap itself (in .bss).
            let kernel = unsafe { (physical(&_kernel_start), physical(&_kernel_end)) };
            let info_address = info as *const Info as u32 - KERNEL_VBASE;
            for (start, end) in [
                kernel,
                span(info_address, size_of::<Info>()),
                span(info.mmap_addr, info.mmap_length as usize),
            ] {
                bitmap.reserve(start, end);
            }

            if info.flags & FLAG_MODS != 0 {
                let count = info.mods_count as usize;
                let (start, end) = span(info.mods_addr, count * size_of::<Module>());
                bitmap.reserve(start, end);
                let modules = unsafe {
                    core::slice::from_raw_parts(
                        phys_to_virt(info.mods_addr) as *const Module,
                        count,
                    )
                };
                for module in modules {
                    bitmap.reserve(module.start as u64, module.end as u64);
                }
            }
        });
    }

    #[unsafe(no_mangle)]
    unsafe extern "C" fn physical_init(info: *const Info) {
        unsafe { init(&*info) };
    }

    /// A free frame's physical address, or 0 when memory is exhausted
    /// (frame 0 is never handed out).
    #[unsafe(no_mangle)]
    extern "C" fn physical_alloc_frame() -> u32 {
        PHYSICAL.with(|b| b.alloc()).unwrap_or(0)
    }

    #[unsafe(no_mangle)]
    extern "C" fn physical_free_frame(address: u32) {
        PHYSICAL.with(|b| b.free(address));
    }

    #[unsafe(no_mangle)]
    extern "C" fn physical_is_frame_used(address: u32) -> bool {
        PHYSICAL.with(|b| b.is_used(address))
    }

    #[unsafe(no_mangle)]
    extern "C" fn physical_total_frames() -> u32 {
        PHYSICAL.with(|b| b.total_frames())
    }

    #[unsafe(no_mangle)]
    extern "C" fn physical_free_frames() -> u32 {
        PHYSICAL.with(|b| b.free_frames())
    }

    #[unsafe(no_mangle)]
    extern "C" fn physical_used_frames() -> u32 {
        PHYSICAL.with(|b| b.used_frames())
    }

    #[cfg(not(test))]
    pub mod ktests {
        use super::PHYSICAL;
        use crate::ktest::Outcome;
        use crate::memory::physical::{FRAME_SIZE, LOW_MEMORY_END};

        tests!(
            physical_has_free_frames_after_init,
            physical_alloc_marks_an_aligned_frame_used,
            physical_free_allows_realloc,
            physical_alloc_returns_different_frames,
            physical_low_memory_and_kernel_are_used,
            physical_free_count_tracks_alloc,
        );

        fn alloc() -> Result<u32, crate::ktest::Failure> {
            let frame = PHYSICAL.with(|p| p.alloc());
            check!(frame.is_some());
            Ok(frame.unwrap())
        }

        fn physical_has_free_frames_after_init() -> Outcome {
            check!(PHYSICAL.with(|p| p.total_frames()) > 0);
            Ok(())
        }

        fn physical_alloc_marks_an_aligned_frame_used() -> Outcome {
            let frame = alloc()?;
            let used = PHYSICAL.with(|p| p.is_used(frame));
            PHYSICAL.with(|p| p.free(frame));
            check_eq!(frame % FRAME_SIZE, 0);
            check!(frame as u64 >= LOW_MEMORY_END);
            check!(used);
            check!(!PHYSICAL.with(|p| p.is_used(frame)));
            Ok(())
        }

        fn physical_free_allows_realloc() -> Outcome {
            let first = alloc()?;
            PHYSICAL.with(|p| p.free(first));
            let second = alloc()?;
            PHYSICAL.with(|p| p.free(second));
            check_eq!(first, second);
            Ok(())
        }

        fn physical_alloc_returns_different_frames() -> Outcome {
            let first = alloc()?;
            let second = alloc()?;
            PHYSICAL.with(|p| {
                p.free(second);
                p.free(first);
            });
            check!(first != second);
            Ok(())
        }

        fn physical_low_memory_and_kernel_are_used() -> Outcome {
            for address in [0, 0x1000, 0x9_E000, 0x10_0000] {
                check!(PHYSICAL.with(|p| p.is_used(address)));
            }
            Ok(())
        }

        fn physical_free_count_tracks_alloc() -> Outcome {
            let before = PHYSICAL.with(|p| p.free_frames());
            let frame = alloc()?;
            let during = PHYSICAL.with(|p| p.free_frames());
            PHYSICAL.with(|p| p.free(frame));
            check_eq!(during, before - 1);
            check_eq!(PHYSICAL.with(|p| p.free_frames()), before);
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const MIB: u64 = 0x10_0000;

    fn ram(base: u64, length: u64) -> Region {
        Region {
            base,
            length,
            kind: MMAP_AVAILABLE,
        }
    }

    fn bitmap(regions: &[Region]) -> Box<FrameBitmap> {
        let mut bitmap = Box::new(FrameBitmap::new());
        bitmap.reset(regions.iter().copied());
        bitmap
    }

    #[test]
    fn only_available_ram_above_1_mib_is_free() {
        let reserved = Region {
            base: 2 * MIB,
            length: MIB,
            kind: 2,
        };
        let b = bitmap(&[ram(0, 0x9FC00), ram(MIB, MIB), reserved]);
        assert!(b.is_used(0x1000));
        assert!(b.is_used(0x9E000));
        assert!(!b.is_used(MIB as u32));
        assert!(b.is_used(2 * MIB as u32));
        assert_eq!(b.total_frames(), 256);
        assert_eq!(b.free_frames(), 256);
    }

    #[test]
    fn partial_frames_at_region_edges_stay_used() {
        let b = bitmap(&[ram(MIB + 0x800, 0x2000)]);
        // Only the frame wholly inside [MIB + 0x800, MIB + 0x2800).
        assert!(b.is_used(MIB as u32));
        assert!(!b.is_used(MIB as u32 + 0x1000));
        assert!(b.is_used(MIB as u32 + 0x2000));
        assert_eq!(b.total_frames(), 1);
    }

    #[test]
    fn ram_beyond_the_direct_map_is_not_used() {
        let b = bitmap(&[
            ram(DIRECT_MAP_END - 0x1000, 0x2000),
            ram(0x1_0000_0000, MIB),
        ]);
        assert!(!b.is_used((DIRECT_MAP_END - 0x1000) as u32));
        assert!(b.is_used(DIRECT_MAP_END as u32));
        assert_eq!(b.total_frames(), 1);
    }

    #[test]
    fn reserve_covers_partially_overlapped_frames() {
        let mut b = bitmap(&[ram(MIB, MIB)]);
        b.reserve(MIB + 0x800, MIB + 0x1001);
        assert!(b.is_used(MIB as u32));
        assert!(b.is_used(MIB as u32 + 0x1000));
        assert!(!b.is_used(MIB as u32 + 0x2000));
        assert_eq!(b.used_frames(), 2);
    }

    #[test]
    fn alloc_hands_out_the_lowest_free_frame_once() {
        let mut b = bitmap(&[ram(MIB, 0x3000)]);
        assert_eq!(b.alloc(), Some(MIB as u32));
        assert_eq!(b.alloc(), Some(MIB as u32 + 0x1000));
        b.free(MIB as u32);
        assert_eq!(b.alloc(), Some(MIB as u32));
        assert_eq!(b.alloc(), Some(MIB as u32 + 0x2000));
        assert_eq!(b.alloc(), None);
    }

    #[test]
    fn alloc_contiguous_skips_runs_that_are_too_short() {
        let mut b = bitmap(&[ram(MIB, 0x2000), ram(MIB + 0x3000, 0x4000)]);
        assert_eq!(b.alloc_contiguous(3), Some(MIB as u32 + 0x3000));
        assert!(b.is_used(MIB as u32 + 0x5000));
        assert!(!b.is_used(MIB as u32 + 0x6000));
        assert_eq!(b.alloc_contiguous(2), Some(MIB as u32));
        assert_eq!(b.alloc_contiguous(2), None);
    }
}

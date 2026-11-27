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
// frames through the direct map.

use core::mem::size_of;

use super::DIRECT_MAP_END;
use super::multiboot::{FLAG_MMAP, FLAG_MODS, Info, MMAP_AVAILABLE, Module, Region};

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

    /// Return every frame overlapping [start, end) to the free pool, under
    /// the same rule as `free`.
    #[cfg(any(test, not(feature = "ktest")))]
    pub fn release(&mut self, start: u64, end: u64) {
        let first = start / FRAME_SIZE as u64;
        let last = end.div_ceil(FRAME_SIZE as u64);
        for frame in first..last {
            self.free((frame * FRAME_SIZE as u64) as u32);
        }
    }

    /// Return the frame at `address` to the free pool. Frames outside
    /// the range ever handed out (device memory a process mapped, for
    /// example) are ignored.
    pub fn free(&mut self, address: u32) {
        if (LOW_MEMORY_END..DIRECT_MAP_END).contains(&(address as u64)) {
            self.set((address / FRAME_SIZE) as usize, false);
        }
    }

    #[cfg(any(test, feature = "ktest"))]
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

/// What the boot loader handed over, as physical [start, end) ranges: the
/// information block at `info_address`, the memory map, the module list
/// and each module.
pub fn handed_over<'a>(
    info: &'a Info,
    info_address: u32,
    modules: &'a [Module],
) -> impl Iterator<Item = (u64, u64)> + 'a {
    let span = |start: u32, length: usize| (start as u64, start as u64 + length as u64);
    let mut ranges = [None; 3];
    ranges[0] = Some(span(info_address, size_of::<Info>()));
    if info.flags & FLAG_MMAP != 0 {
        ranges[1] = Some(span(info.mmap_addr, info.mmap_length as usize));
    }
    if info.flags & FLAG_MODS != 0 {
        ranges[2] = Some(span(info.mods_addr, modules.len() * size_of::<Module>()));
    }
    let images = modules.iter().map(|m| (m.start as u64, m.end as u64));
    ranges.into_iter().flatten().chain(images)
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use super::{FrameBitmap, handed_over};
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
    /// Without a memory map every frame stays used.
    ///
    /// # Safety
    ///
    /// `info` must be the boot loader's information, with its addresses
    /// reachable through the direct map.
    pub unsafe fn init(info: &Info) {
        let map: &[u8] = if info.flags & FLAG_MMAP != 0 {
            unsafe {
                core::slice::from_raw_parts(phys_to_virt(info.mmap_addr), info.mmap_length as usize)
            }
        } else {
            &[]
        };
        let modules: &[Module] = if info.flags & FLAG_MODS != 0 {
            unsafe {
                core::slice::from_raw_parts(
                    phys_to_virt(info.mods_addr) as *const Module,
                    info.mods_count as usize,
                )
            }
        } else {
            &[]
        };
        let physical = |symbol: *const u8| (symbol as u32 - KERNEL_VBASE) as u64;
        // The kernel image includes the bitmap itself (in .bss).
        let kernel = unsafe { (physical(&_kernel_start), physical(&_kernel_end)) };
        let info_address = info as *const Info as u32 - KERNEL_VBASE;

        PHYSICAL.with(|bitmap| {
            bitmap.reset(regions(map));
            for (start, end) in handed_over(info, info_address, modules).chain([kernel]) {
                bitmap.reserve(start, end);
            }
        });
    }

    /// Free the module images once their processes are loaded (the ELF
    /// loader copies each into the process's own frames).
    ///
    /// # Safety
    ///
    /// As for `init`; nothing may read the modules afterwards.
    #[cfg(not(feature = "ktest"))]
    pub unsafe fn release_modules(info: &Info) {
        if info.flags & FLAG_MODS == 0 {
            return;
        }
        let modules = unsafe {
            core::slice::from_raw_parts(
                phys_to_virt(info.mods_addr) as *const Module,
                info.mods_count as usize,
            )
        };
        PHYSICAL.with(|bitmap| {
            for module in modules {
                bitmap.release(module.start as u64, module.end as u64);
            }
        });
    }

    #[cfg(feature = "ktest")]
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
    fn without_a_memory_map_every_frame_is_used() {
        let mut b = bitmap(&[]);
        assert_eq!(b.total_frames(), 0);
        assert_eq!(b.alloc(), None);
    }

    #[test]
    fn frees_outside_the_handed_out_range_are_ignored() {
        let mut b = bitmap(&[ram(MIB, MIB)]);
        let used = b.used_frames();
        b.free(0x1000);
        b.free(0xFD00_0000);
        assert!(b.is_used(0x1000));
        assert!(b.is_used(0xFD00_0000));
        assert_eq!(b.used_frames(), used);
    }

    #[test]
    fn release_frees_every_overlapped_frame_it_may() {
        let mut b = bitmap(&[ram(MIB, MIB)]);
        b.reserve(MIB, MIB + 0x3000);
        b.release(MIB + 0x800, MIB + 0x2001);
        assert!(!b.is_used(MIB as u32));
        assert!(!b.is_used(MIB as u32 + 0x2000));
        // Low memory stays used.
        b.release(0, 0x2000);
        assert!(b.is_used(0x1000));
    }

    #[test]
    fn handed_over_covers_the_boot_loader_data() {
        // SAFETY: Info is plain integers.
        let mut info: Info = unsafe { core::mem::zeroed() };
        info.flags = FLAG_MMAP | FLAG_MODS;
        info.mmap_addr = 0x9000;
        info.mmap_length = 144;
        info.mods_addr = 0x3_2C00;
        let modules = [
            Module {
                start: 0x32_D000,
                end: 0x33_1598,
                string: 0,
                reserved: 0,
            },
            Module {
                start: 0x33_2000,
                end: 0x33_7A28,
                string: 0,
                reserved: 0,
            },
        ];
        let ranges: Vec<_> = handed_over(&info, 0x9500, &modules).collect();
        assert_eq!(
            ranges,
            [
                (0x9500, 0x9500 + size_of::<Info>() as u64),
                (0x9000, 0x9090),
                (0x3_2C00, 0x3_2C20),
                (0x32_D000, 0x33_1598),
                (0x33_2000, 0x33_7A28),
            ]
        );
        info.flags = 0;
        assert_eq!(handed_over(&info, 0x9500, &[]).count(), 1);
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

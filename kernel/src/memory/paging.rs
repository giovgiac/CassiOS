// paging.rs -- page directories and tables
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// The kernel directory maps physical memory at KERNEL_VBASE (the direct
// map). Each process gets a directory whose kernel half (entries
// 768-1023) is copied from the kernel's, so kernel mappings are shared.
// Tables and directories are reached through the direct map.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

pub const PAGE_PRESENT: u32 = 0x01;
pub const PAGE_READWRITE: u32 = 0x02;
pub const PAGE_USER: u32 = 0x04;
/// Marks a device mapping (an available bit): its frame belongs to the
/// device, not the allocator, so destroying the address space keeps it.
pub const PAGE_DEVICE: u32 = 0x200;

/// Entries in a directory or table.
const ENTRIES: usize = 1024;
/// First directory entry of the kernel half.
const KERNEL_ENTRY: usize = 768;
const ADDRESS_MASK: u32 = 0xFFFF_F000;

fn directory_index(virt: u32) -> usize {
    (virt >> 22) as usize
}

fn table_index(virt: u32) -> usize {
    ((virt >> 12) & 0x3FF) as usize
}

/// The first address of each page that [start, start + len) touches.
fn pages(start: u32, len: u32) -> impl Iterator<Item = u32> {
    let first = start & ADDRESS_MASK;
    let count = if len == 0 {
        0
    } else {
        (((start as u64 + len as u64 - 1) & ADDRESS_MASK as u64) - first as u64) / 4096 + 1
    };
    (0..count).map(move |i| first + i as u32 * 4096)
}

#[cfg(target_arch = "x86")]
pub use imp::*;

#[cfg(target_arch = "x86")]
mod imp {
    use core::arch::asm;

    use super::*;
    use crate::memory::multiboot::{FLAG_MMAP, Info, MMAP_AVAILABLE, regions};
    use crate::memory::physical::{FRAME_SIZE, PHYSICAL};
    use crate::memory::{DIRECT_MAP_END, KERNEL_VBASE, phys_to_virt};
    use crate::sync::Global;

    type Table = [u32; ENTRIES];

    /// What loader.s maps before paging is set up.
    const BOOT_MAPPING_END: u64 = 0x40_0000;

    #[repr(C, align(4096))]
    struct Directory(Table);

    static KERNEL_DIRECTORY: Global<Directory> = Global::new(Directory([0; ENTRIES]));

    /// The kernel table for the first 4 MiB of the direct map. Static, so
    /// mapping them needs no frame from the allocator while only the boot
    /// mapping is reachable.
    static BOOT_TABLE: Global<Directory> = Global::new(Directory([0; ENTRIES]));

    /// The table or directory at physical address `entry & ADDRESS_MASK`.
    fn table(entry: u32) -> &'static mut Table {
        // SAFETY: tables and directories are whole frames in the direct
        // map, used one at a time (see Global).
        unsafe { &mut *(phys_to_virt(entry & ADDRESS_MASK) as *mut Table) }
    }

    /// The kernel directory's physical address, for CR3.
    pub fn kernel_directory() -> u32 {
        KERNEL_DIRECTORY.with(|d| d.0.as_ptr() as u32) - KERNEL_VBASE
    }

    /// A free frame, zeroed; its physical address.
    pub fn zeroed_frame() -> Option<u32> {
        let frame = PHYSICAL.with(|p| p.alloc())?;
        table(frame).fill(0);
        Some(frame)
    }

    /// Direct-map the available RAM below DIRECT_MAP_END and the VGA text
    /// buffer, then switch to the kernel directory.
    ///
    /// Until the switch only the boot mapping (the first 4 MiB) is
    /// reachable, so the first 4 MiB are mapped through the static
    /// BOOT_TABLE and the directory loaded first. Frames for later tables
    /// then lie in memory already mapped.
    ///
    /// # Safety
    ///
    /// `info` must be the boot loader's information; the physical
    /// allocator must be initialized.
    pub unsafe fn init(info: &Info) {
        if info.flags & FLAG_MMAP == 0 {
            return;
        }
        let map = unsafe {
            core::slice::from_raw_parts(phys_to_virt(info.mmap_addr), info.mmap_length as usize)
        };
        let flags = PAGE_PRESENT | PAGE_READWRITE;
        let map_ram = |from: u64, to: u64| {
            for region in regions(map).filter(|r| r.kind == MMAP_AVAILABLE) {
                let start = (region.base & ADDRESS_MASK as u64).max(from);
                let end = region.base.saturating_add(region.length).min(to);
                for page in (start..end).step_by(FRAME_SIZE as usize) {
                    map_page(phys_to_virt(page as u32) as u32, page as u32, flags);
                }
            }
        };

        let boot_table = BOOT_TABLE.with(|t| t.0.as_ptr() as u32) - KERNEL_VBASE;
        KERNEL_DIRECTORY.with(|d| {
            d.0[directory_index(KERNEL_VBASE)] = boot_table | PAGE_PRESENT | PAGE_READWRITE;
        });
        map_ram(0, BOOT_MAPPING_END);
        for page in (0xB8000..0xC0000).step_by(FRAME_SIZE as usize) {
            map_page(phys_to_virt(page) as u32, page, flags);
        }
        let directory = KERNEL_DIRECTORY.with(|d| d.0.as_ptr() as u32) - KERNEL_VBASE;
        unsafe { asm!("mov cr3, {}", in(reg) directory) };

        map_ram(BOOT_MAPPING_END, DIRECT_MAP_END);
    }

    /// Map `virt` to `phys` in the kernel directory, adding a table if
    /// needed. Does nothing if no frame is left for the table.
    pub fn map_page(virt: u32, phys: u32, flags: u32) {
        let new_table = KERNEL_DIRECTORY.with(|directory| {
            let entry = &mut directory.0[directory_index(virt)];
            let new_table = *entry & PAGE_PRESENT == 0;
            if new_table {
                let Some(frame) = zeroed_frame() else {
                    return None;
                };
                *entry = frame | PAGE_PRESENT | PAGE_READWRITE;
            }
            table(*entry)[table_index(virt)] = (phys & ADDRESS_MASK) | (flags & 0xFFF);
            Some(new_table)
        });
        // A stale TLB entry from a former table could linger.
        if new_table == Some(true) {
            flush_tlb(virt);
        }
    }

    pub fn flush_tlb(virt: u32) {
        unsafe { asm!("invlpg [{}]", in(reg) virt, options(nostack)) };
    }

    /// A new directory sharing the kernel half; its physical address, or
    /// None when memory is exhausted.
    pub fn create_address_space() -> Option<u32> {
        let frame = zeroed_frame()?;
        KERNEL_DIRECTORY.with(|kernel| {
            table(frame)[KERNEL_ENTRY..].copy_from_slice(&kernel.0[KERNEL_ENTRY..]);
        });
        Some(frame)
    }

    /// Map `virt` to `phys` in the directory at `directory`, adding a user
    /// table if needed.
    pub fn map_user_page(directory: u32, virt: u32, phys: u32, flags: u32) {
        let entry = &mut table(directory)[directory_index(virt)];
        if *entry & PAGE_PRESENT == 0 {
            let Some(frame) = zeroed_frame() else {
                return;
            };
            *entry = frame | PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
        }
        table(*entry)[table_index(virt)] = (phys & ADDRESS_MASK) | (flags & 0xFFF);
    }

    /// Free a directory, its user tables and every frame they map, except
    /// device mappings.
    pub fn destroy_address_space(directory: u32) {
        for &entry in &table(directory)[..KERNEL_ENTRY] {
            if entry & PAGE_PRESENT == 0 {
                continue;
            }
            for &page in table(entry).iter() {
                if page & (PAGE_PRESENT | PAGE_DEVICE) == PAGE_PRESENT {
                    PHYSICAL.with(|p| p.free(page & ADDRESS_MASK));
                }
            }
            PHYSICAL.with(|p| p.free(entry & ADDRESS_MASK));
        }
        PHYSICAL.with(|p| p.free(directory));
    }

    /// Whether every page of [start, start + len) is present and
    /// user-accessible in the directory at `directory`.
    pub fn user_range_mapped(directory: u32, start: u32, len: u32) -> bool {
        let user = PAGE_PRESENT | PAGE_USER;
        pages(start, len).all(|page| {
            let entry = table(directory)[directory_index(page)];
            entry & user == user && table(entry)[table_index(page)] & user == user
        })
    }

    /// The physical address of the page `virt` lies in, if it is present
    /// in the directory at `directory`.
    pub fn translate(directory: u32, virt: u32) -> Option<u32> {
        let entry = table(directory)[directory_index(virt)];
        if entry & PAGE_PRESENT == 0 {
            return None;
        }
        let page = table(entry)[table_index(virt)];
        (page & PAGE_PRESENT != 0).then_some(page & ADDRESS_MASK)
    }

    /// Whether no page of [start, start + len) is present in the directory
    /// at `directory`.
    pub fn range_unmapped(directory: u32, start: u32, len: u32) -> bool {
        pages(start, len).all(|page| {
            let entry = table(directory)[directory_index(page)];
            entry & PAGE_PRESENT == 0 || table(entry)[table_index(page)] & PAGE_PRESENT == 0
        })
    }

    #[cfg(feature = "ktest")]
    pub mod ktests {
        use alloc::boxed::Box;

        use super::*;
        use crate::ktest::Outcome;

        tests!(
            paging_kernel_runs_in_the_higher_half,
            paging_vga_memory_is_mapped,
            paging_cr3_holds_the_kernel_directory,
            paging_map_page_maps_a_frame,
            paging_address_space_shares_the_kernel_half,
            paging_map_user_page_creates_a_user_mapping,
            paging_user_range_mapped_checks_every_page,
            paging_destroy_keeps_device_frames,
            paging_range_unmapped_finds_any_present_page,
            paging_first_4_mib_use_the_boot_table,
        );

        unsafe extern "C" {
            static _kernel_start: u8;
            static _kernel_end: u8;
        }

        fn cr3() -> u32 {
            let value: u32;
            unsafe { asm!("mov {}, cr3", out(reg) value, options(nomem, nostack)) };
            value
        }

        fn new_frame() -> Result<u32, crate::ktest::Failure> {
            let frame = PHYSICAL.with(|p| p.alloc());
            check!(frame.is_some());
            Ok(frame.unwrap())
        }

        fn paging_kernel_runs_in_the_higher_half() -> Outcome {
            let (start, end) = unsafe { (&_kernel_start as *const u8, &_kernel_end as *const u8) };
            let stack = 0u32;
            let heap = Box::new(0u32);
            check!(start as u32 >= KERNEL_VBASE && end > start);
            check!(&stack as *const u32 as u32 >= KERNEL_VBASE);
            check!(&*heap as *const u32 as u32 >= KERNEL_VBASE);
            Ok(())
        }

        fn paging_vga_memory_is_mapped() -> Outcome {
            let vga = phys_to_virt(0xB8000) as *mut u16;
            let value = unsafe {
                let original = vga.read_volatile();
                vga.write_volatile(0x0741);
                let value = vga.read_volatile();
                vga.write_volatile(original);
                value
            };
            check_eq!(value, 0x0741);
            Ok(())
        }

        fn paging_cr3_holds_the_kernel_directory() -> Outcome {
            let kernel = KERNEL_DIRECTORY.with(|d| d.0.as_ptr() as u32);
            check_eq!(cr3(), kernel - KERNEL_VBASE);
            Ok(())
        }

        fn paging_map_page_maps_a_frame() -> Outcome {
            let frame = new_frame()?;
            let virt = phys_to_virt(frame) as u32;
            map_page(virt, frame, PAGE_PRESENT | PAGE_READWRITE);
            flush_tlb(virt);
            let value = unsafe {
                (virt as *mut u32).write_volatile(0x1234_5678);
                (virt as *const u32).read_volatile()
            };
            PHYSICAL.with(|p| p.free(frame));
            check_eq!(value, 0x1234_5678);
            Ok(())
        }

        fn paging_address_space_shares_the_kernel_half() -> Outcome {
            let directory = create_address_space();
            check!(directory.is_some());
            let directory = directory.unwrap();
            let entries = *table(directory);
            let kernel = KERNEL_DIRECTORY.with(|d| d.0);
            destroy_address_space(directory);
            check!((directory as u64) < DIRECT_MAP_END);
            check!(entries[..KERNEL_ENTRY].iter().all(|&e| e == 0));
            check!(entries[KERNEL_ENTRY..] == kernel[KERNEL_ENTRY..]);
            Ok(())
        }

        fn paging_map_user_page_creates_a_user_mapping() -> Outcome {
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            let frame = new_frame()?;
            let virt = 0x0040_0000;
            map_user_page(
                directory,
                virt,
                frame,
                PAGE_PRESENT | PAGE_READWRITE | PAGE_USER,
            );
            let entry = table(directory)[directory_index(virt)];
            let page = table(entry)[table_index(virt)];
            let before = PHYSICAL.with(|p| p.free_frames());
            // Frees the frame, the table and the directory.
            destroy_address_space(directory);
            let after = PHYSICAL.with(|p| p.free_frames());
            check_eq!(entry & (PAGE_PRESENT | PAGE_USER), PAGE_PRESENT | PAGE_USER);
            check_eq!(page & ADDRESS_MASK, frame);
            check_eq!(after, before + 3);
            Ok(())
        }

        fn paging_destroy_keeps_device_frames() -> Outcome {
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            // A frame the kernel uses (its first code page), mapped as a
            // device, as mapDevice would.
            let kernel_frame = 0x10_0000;
            let flags = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER | PAGE_DEVICE;
            map_user_page(directory, 0x1000_0000, kernel_frame, flags);
            destroy_address_space(directory);
            check!(PHYSICAL.with(|p| p.is_used(kernel_frame)));
            Ok(())
        }

        fn paging_range_unmapped_finds_any_present_page() -> Outcome {
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            let frame = new_frame()?;
            map_user_page(directory, 0x0040_1000, frame, PAGE_PRESENT | PAGE_USER);
            let unmapped = |start, len| range_unmapped(directory, start, len);
            let results = [
                unmapped(0x0040_0000, 0x1000),
                unmapped(0x0080_0000, 0x10_0000),
                !unmapped(0x0040_0000, 0x1001),
                !unmapped(0x0040_1FFF, 1),
            ];
            destroy_address_space(directory);
            check_eq!(results, [true; 4]);
            Ok(())
        }

        fn paging_first_4_mib_use_the_boot_table() -> Outcome {
            let entry = KERNEL_DIRECTORY.with(|d| d.0[directory_index(KERNEL_VBASE)]);
            let boot_table = BOOT_TABLE.with(|t| t.0.as_ptr() as u32) - KERNEL_VBASE;
            check_eq!(entry & ADDRESS_MASK, boot_table);
            Ok(())
        }

        fn paging_user_range_mapped_checks_every_page() -> Outcome {
            let directory = create_address_space().unwrap_or(0);
            check!(directory != 0);
            let frame = new_frame()?;
            let user = PAGE_PRESENT | PAGE_READWRITE | PAGE_USER;
            map_user_page(directory, 0x0040_0000, frame, user);
            // Present but kernel-only.
            let kernel_only = new_frame()?;
            map_user_page(directory, 0x0040_1000, kernel_only, PAGE_PRESENT);
            let mapped = |start, len| user_range_mapped(directory, start, len);
            let results = [
                mapped(0x0040_0000, 0x1000),
                mapped(0x0040_0FFF, 1),
                mapped(0x0040_0000, 0),
                !mapped(0x0040_0000, 0x1001),
                !mapped(0x003F_FFFF, 2),
                !mapped(0x0080_0000, 1),
                !mapped(KERNEL_VBASE + 0x10_0000, 1),
            ];
            destroy_address_space(directory);
            check_eq!(results, [true; 7]);
            Ok(())
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn indices_split_the_address() {
        assert_eq!(directory_index(0xC010_2345), 768);
        assert_eq!(table_index(0xC010_2345), 0x102);
        assert_eq!(directory_index(0x0040_0000), 1);
        assert_eq!(table_index(0x0040_0000), 0);
    }

    #[test]
    fn pages_covers_every_touched_page() {
        assert_eq!(pages(0x1000, 0).count(), 0);
        assert_eq!(pages(0x1FFF, 1).collect::<Vec<_>>(), [0x1000]);
        assert_eq!(pages(0x1FFF, 2).collect::<Vec<_>>(), [0x1000, 0x2000]);
        assert_eq!(pages(0x1000, 0x2000).collect::<Vec<_>>(), [0x1000, 0x2000]);
        assert_eq!(
            pages(0xFFFF_F000, 0x1000).collect::<Vec<_>>(),
            [0xFFFF_F000]
        );
    }
}

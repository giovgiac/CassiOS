// multiboot.rs -- Multiboot information from the boot loader
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Layouts follow the Multiboot specification. Every address in them is
// physical.

// Most of it is only used by the i586 build.
#![cfg_attr(not(target_arch = "x86"), allow(dead_code))]

use core::mem::size_of;

pub const FLAG_MODS: u32 = 1 << 3;
pub const FLAG_MMAP: u32 = 1 << 6;
pub const FLAG_FRAMEBUFFER: u32 = 1 << 12;
pub const MMAP_AVAILABLE: u32 = 1;

#[repr(C, packed)]
pub struct Info {
    pub flags: u32,
    pub mem_lower: u32,
    pub mem_upper: u32,
    pub boot_device: u32,
    pub cmdline: u32,
    pub mods_count: u32,
    pub mods_addr: u32,
    pub syms: [u32; 4],
    pub mmap_length: u32,
    pub mmap_addr: u32,
    pub drives_length: u32,
    pub drives_addr: u32,
    pub config_table: u32,
    pub boot_loader_name: u32,
    pub apm_table: u32,
    pub vbe_control_info: u32,
    pub vbe_mode_info: u32,
    pub vbe_mode: u16,
    pub vbe_interface_seg: u16,
    pub vbe_interface_off: u16,
    pub vbe_interface_len: u16,
    pub framebuffer_addr: u64,
    pub framebuffer_pitch: u32,
    pub framebuffer_width: u32,
    pub framebuffer_height: u32,
    pub framebuffer_bpp: u8,
    pub framebuffer_type: u8,
}

#[repr(C, packed)]
pub struct Module {
    pub start: u32,
    pub end: u32,
    pub string: u32,
    pub reserved: u32,
}

/// One memory map entry: the region and its type (MMAP_AVAILABLE for RAM).
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Region {
    pub base: u64,
    pub length: u64,
    pub kind: u32,
}

/// The memory map entries in `bytes`, which holds the map as the boot
/// loader wrote it: each entry is a size field followed by `size` bytes.
pub fn regions(bytes: &[u8]) -> impl Iterator<Item = Region> + '_ {
    let mut offset = 0;
    core::iter::from_fn(move || {
        // size, base_addr, length, type
        const ENTRY: usize = 4 + 8 + 8 + 4;
        let entry = bytes.get(offset..offset + ENTRY)?;
        let u32_at = |i: usize| u32::from_le_bytes(entry[i..i + 4].try_into().unwrap());
        let u64_at = |i: usize| u64::from_le_bytes(entry[i..i + 8].try_into().unwrap());
        offset += size_of::<u32>() + u32_at(0) as usize;
        Some(Region {
            base: u64_at(4),
            length: u64_at(12),
            kind: u32_at(20),
        })
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::mem::offset_of;

    #[test]
    fn info_layout_matches_the_specification() {
        assert_eq!(offset_of!(Info, mods_addr), 24);
        assert_eq!(offset_of!(Info, mmap_length), 44);
        assert_eq!(offset_of!(Info, mmap_addr), 48);
        assert_eq!(offset_of!(Info, framebuffer_addr), 88);
        assert_eq!(offset_of!(Info, framebuffer_bpp), 108);
        assert_eq!(size_of::<Module>(), 16);
    }

    fn entry(size: u32, base: u64, length: u64, kind: u32) -> Vec<u8> {
        let mut bytes = Vec::new();
        bytes.extend(size.to_le_bytes());
        bytes.extend(base.to_le_bytes());
        bytes.extend(length.to_le_bytes());
        bytes.extend(kind.to_le_bytes());
        bytes.resize(4 + size as usize, 0);
        bytes
    }

    #[test]
    fn regions_follow_each_entry_size() {
        let mut map = entry(20, 0, 0x9FC00, 1);
        // An entry with 4 extra bytes, which the size field skips.
        map.extend(entry(24, 0x100000, 0x7EE0000, 1));
        map.extend(entry(20, 0xFFFC0000, 0x40000, 2));
        let found: Vec<Region> = regions(&map).collect();
        assert_eq!(
            found,
            [
                Region {
                    base: 0,
                    length: 0x9FC00,
                    kind: 1
                },
                Region {
                    base: 0x100000,
                    length: 0x7EE0000,
                    kind: 1
                },
                Region {
                    base: 0xFFFC0000,
                    length: 0x40000,
                    kind: 2
                },
            ]
        );
    }

    #[test]
    fn a_truncated_entry_ends_the_map() {
        let mut map = entry(20, 0, 0x1000, 1);
        map.extend([0; 10]);
        assert_eq!(regions(&map).count(), 1);
    }
}

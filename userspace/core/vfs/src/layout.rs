// layout.rs -- FAT32 volume layout from the boot sector
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.

use crate::cache::{SECTOR_SIZE, Sector};

/// Where the FATs, the data area and the root directory live.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Layout {
    pub sectors_per_cluster: u32,
    pub fat_start: u32,
    /// Sectors per FAT.
    pub fat_size: u32,
    pub num_fats: u32,
    pub data_start: u32,
    pub root_cluster: u32,
    /// Valid cluster numbers are 2..cluster_end.
    pub cluster_end: u32,
}

fn u16_at(sector: &Sector, offset: usize) -> u32 {
    u16::from_le_bytes([sector[offset], sector[offset + 1]]) as u32
}

fn u32_at(sector: &Sector, offset: usize) -> u32 {
    u32::from_le_bytes(sector[offset..offset + 4].try_into().unwrap())
}

impl Layout {
    /// Parse a FAT32 boot sector, or None if it does not describe a volume
    /// this driver can use (anything that would make the arithmetic below
    /// divide by zero or point outside the disk).
    pub fn parse(boot: &Sector) -> Option<Self> {
        let bytes_per_sector = u16_at(boot, 11);
        let sectors_per_cluster = boot[13] as u32;
        let reserved = u16_at(boot, 14);
        let num_fats = boot[16] as u32;
        let total_sectors = u32_at(boot, 32);
        let fat_size = u32_at(boot, 36);
        let root_cluster = u32_at(boot, 44);

        let valid = bytes_per_sector == SECTOR_SIZE as u32
            && sectors_per_cluster.is_power_of_two()
            && reserved > 0
            && num_fats > 0
            && fat_size > 0;
        if !valid {
            return None;
        }

        let data_start = reserved.checked_add(num_fats.checked_mul(fat_size)?)?;
        let clusters = total_sectors.checked_sub(data_start)? / sectors_per_cluster;
        let cluster_end = clusters.checked_add(2)?;
        // Each FAT must hold an entry for every cluster.
        let fat_entries = fat_size.checked_mul(SECTOR_SIZE as u32 / 4)?;
        if clusters == 0 || cluster_end > fat_entries {
            return None;
        }
        if root_cluster < 2 || root_cluster >= cluster_end {
            return None;
        }

        Some(Self {
            sectors_per_cluster,
            fat_start: reserved,
            fat_size,
            num_fats,
            data_start,
            root_cluster,
            cluster_end,
        })
    }

    pub fn bytes_per_cluster(&self) -> u32 {
        self.sectors_per_cluster * SECTOR_SIZE as u32
    }

    /// Whether `cluster` can hold data.
    pub fn is_data_cluster(&self, cluster: u32) -> bool {
        (2..self.cluster_end).contains(&cluster)
    }

    /// First sector of data cluster `cluster`.
    pub fn cluster_lba(&self, cluster: u32) -> u32 {
        self.data_start + (cluster - 2) * self.sectors_per_cluster
    }
}

#[cfg(test)]
pub mod tests {
    use super::*;

    /// A boot sector with the given geometry (2 FATs, 32 reserved sectors,
    /// root at cluster 2).
    pub fn boot_sector(total: u32, sectors_per_cluster: u8, fat_size: u32) -> Sector {
        let mut b = [0u8; SECTOR_SIZE];
        b[11..13].copy_from_slice(&512u16.to_le_bytes());
        b[13] = sectors_per_cluster;
        b[14..16].copy_from_slice(&32u16.to_le_bytes());
        b[16] = 2;
        b[32..36].copy_from_slice(&total.to_le_bytes());
        b[36..40].copy_from_slice(&fat_size.to_le_bytes());
        b[44..48].copy_from_slice(&2u32.to_le_bytes());
        b[510] = 0x55;
        b[511] = 0xAA;
        b
    }

    #[test]
    fn parses_a_valid_volume() {
        let l = Layout::parse(&boot_sector(65536, 1, 512)).unwrap();
        assert_eq!(
            (l.fat_start, l.data_start, l.root_cluster),
            (32, 32 + 2 * 512, 2)
        );
        assert_eq!(l.cluster_end, 65536 - 1056 + 2);
        assert_eq!(l.cluster_lba(2), l.data_start);
        assert_eq!(l.cluster_lba(3), l.data_start + 1);
        assert_eq!(l.bytes_per_cluster(), 512);
    }

    #[test]
    fn zeroed_disk_is_rejected() {
        assert_eq!(Layout::parse(&[0; SECTOR_SIZE]), None);
    }

    #[test]
    fn bad_geometry_is_rejected() {
        let cases = [
            boot_sector(65536, 3, 512), // cluster size not a power of two
            boot_sector(65536, 0, 512), // no sectors per cluster
            boot_sector(65536, 1, 0),   // no FAT
            boot_sector(1000, 1, 512),  // FATs larger than the disk
            boot_sector(65536, 1, 16),  // FAT too small for the clusters
        ];
        for boot in cases {
            assert_eq!(Layout::parse(&boot), None);
        }
        let mut root_outside = boot_sector(65536, 1, 512);
        root_outside[44..48].copy_from_slice(&1u32.to_le_bytes());
        assert_eq!(Layout::parse(&root_outside), None);
    }
}

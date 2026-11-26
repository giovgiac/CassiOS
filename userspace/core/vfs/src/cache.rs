// cache.rs -- block device and LRU sector cache
//
// Copyright (c) 2019-2026 Giovanni Giacomo. All Rights Reserved.
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file.
//
// Every sector the filesystem touches goes through a small LRU cache.
// Writes go straight through to the device, so a cached sector is never
// newer than the disk and eviction never writes. A sector whose write
// fails is dropped, so the next read sees what is really on the disk.

use alloc::boxed::Box;
use alloc::vec::Vec;

pub const SECTOR_SIZE: usize = 512;

pub type Sector = [u8; SECTOR_SIZE];

/// A disk addressed in 512-byte sectors.
pub trait BlockDevice {
    fn read(&mut self, lba: u32, buf: &mut Sector) -> bool;
    fn write(&mut self, lba: u32, buf: &Sector) -> bool;
}

#[cfg(target_arch = "x86")]
impl BlockDevice for cassio::ata::Ata {
    fn read(&mut self, lba: u32, buf: &mut Sector) -> bool {
        self.read_sector(lba, buf)
    }

    fn write(&mut self, lba: u32, buf: &Sector) -> bool {
        self.write_sector(lba, buf)
    }
}

struct Slot {
    lba: u32,
    /// Last use, for LRU eviction; 0 means the slot is empty.
    used: u32,
    data: Box<Sector>,
}

/// A write-through LRU cache of `capacity` sectors in front of a device.
pub struct SectorCache<D: BlockDevice> {
    device: D,
    slots: Vec<Slot>,
    clock: u32,
}

impl<D: BlockDevice> SectorCache<D> {
    /// A cache over `device`, or None if its buffers cannot be allocated.
    pub fn new(device: D, capacity: usize) -> Option<Self> {
        let mut slots = Vec::new();
        slots.try_reserve_exact(capacity).ok()?;
        for _ in 0..capacity {
            let mut buf = Vec::new();
            buf.try_reserve_exact(SECTOR_SIZE).ok()?;
            buf.resize(SECTOR_SIZE, 0);
            let data: Box<Sector> = buf.into_boxed_slice().try_into().ok()?;
            slots.push(Slot {
                lba: 0,
                used: 0,
                data,
            });
        }
        Some(Self {
            device,
            slots,
            clock: 0,
        })
    }

    /// Sector `lba`, read from the device on a miss; None on a read error.
    pub fn read(&mut self, lba: u32) -> Option<&Sector> {
        let i = match self.find(lba) {
            Some(i) => i,
            None => {
                let i = self.victim();
                let slot = &mut self.slots[i];
                slot.used = 0;
                if !self.device.read(lba, &mut slot.data) {
                    return None;
                }
                slot.lba = lba;
                i
            }
        };
        self.touch(i);
        Some(&self.slots[i].data)
    }

    /// Read sector `lba`, let `change` modify it, and write it back.
    pub fn update(&mut self, lba: u32, change: impl FnOnce(&mut Sector)) -> bool {
        if self.read(lba).is_none() {
            return false;
        }
        let i = self.find(lba).unwrap();
        change(&mut self.slots[i].data);
        self.write_through(i)
    }

    /// Replace sector `lba` with `data` without reading it first.
    pub fn write(&mut self, lba: u32, data: &Sector) -> bool {
        let i = self.find(lba).unwrap_or_else(|| self.victim());
        let slot = &mut self.slots[i];
        slot.data.copy_from_slice(data);
        slot.lba = lba;
        self.touch(i);
        self.write_through(i)
    }

    /// Write slot `i` to the device, dropping it from the cache on failure.
    fn write_through(&mut self, i: usize) -> bool {
        let slot = &mut self.slots[i];
        let ok = self.device.write(slot.lba, &slot.data);
        if !ok {
            slot.used = 0;
        }
        ok
    }

    fn find(&self, lba: u32) -> Option<usize> {
        self.slots.iter().position(|s| s.used != 0 && s.lba == lba)
    }

    /// An empty slot, or else the least recently used one.
    fn victim(&self) -> usize {
        (0..self.slots.len())
            .min_by_key(|&i| self.slots[i].used)
            .unwrap()
    }

    fn touch(&mut self, i: usize) {
        self.clock += 1;
        self.slots[i].used = self.clock;
    }

    #[cfg(test)]
    pub fn device(&mut self) -> &mut D {
        &mut self.device
    }
}

#[cfg(test)]
pub mod tests {
    use super::*;

    /// An in-memory disk that counts device reads.
    pub struct MemDisk {
        pub sectors: Vec<Sector>,
        pub reads: usize,
        /// Writes to this sector fail.
        pub fail_lba: Option<u32>,
    }

    impl MemDisk {
        pub fn new(count: usize) -> Self {
            Self {
                sectors: alloc::vec![[0; SECTOR_SIZE]; count],
                reads: 0,
                fail_lba: None,
            }
        }
    }

    impl BlockDevice for MemDisk {
        fn read(&mut self, lba: u32, buf: &mut Sector) -> bool {
            self.reads += 1;
            match self.sectors.get(lba as usize) {
                Some(s) => {
                    buf.copy_from_slice(s);
                    true
                }
                None => false,
            }
        }

        fn write(&mut self, lba: u32, buf: &Sector) -> bool {
            if self.fail_lba == Some(lba) {
                return false;
            }
            match self.sectors.get_mut(lba as usize) {
                Some(s) => {
                    s.copy_from_slice(buf);
                    true
                }
                None => false,
            }
        }
    }

    fn cache(sectors: usize, capacity: usize) -> SectorCache<MemDisk> {
        let mut disk = MemDisk::new(sectors);
        for (i, s) in disk.sectors.iter_mut().enumerate() {
            s[0] = i as u8;
        }
        SectorCache::new(disk, capacity).unwrap()
    }

    #[test]
    fn hits_do_not_touch_the_device() {
        let mut c = cache(8, 2);
        assert_eq!(c.read(3).unwrap()[0], 3);
        assert_eq!(c.read(3).unwrap()[0], 3);
        assert_eq!(c.device().reads, 1);
    }

    #[test]
    fn least_recently_used_is_evicted() {
        let mut c = cache(8, 2);
        c.read(1);
        c.read(2);
        c.read(1);
        c.read(3); // evicts 2
        c.read(1);
        assert_eq!(c.device().reads, 3);
        c.read(2);
        assert_eq!(c.device().reads, 4);
    }

    #[test]
    fn update_writes_through() {
        let mut c = cache(8, 2);
        assert!(c.update(5, |s| s[1] = 0xAB));
        assert_eq!(c.read(5).unwrap()[1], 0xAB);
        assert_eq!(c.device().sectors[5][1], 0xAB);
    }

    #[test]
    fn write_replaces_without_reading() {
        let mut c = cache(8, 2);
        assert!(c.write(4, &[7; SECTOR_SIZE]));
        assert_eq!(c.device().reads, 0);
        assert_eq!(c.read(4).unwrap()[0], 7);
        assert_eq!(c.device().sectors[4][0], 7);
    }

    #[test]
    fn failed_writes_are_not_cached() {
        let mut c = cache(8, 2);
        assert_eq!(c.read(5).unwrap()[1], 0);
        c.device().fail_lba = Some(5);
        assert!(!c.update(5, |s| s[1] = 0xAB));
        c.device().fail_lba = Some(6);
        assert!(!c.write(6, &[7; SECTOR_SIZE]));
        c.device().fail_lba = None;
        assert_eq!(c.read(5).unwrap()[1], 0);
        assert_eq!(c.read(6).unwrap()[0], 6);
    }

    #[test]
    fn read_errors_are_reported_and_not_cached() {
        let mut c = cache(4, 2);
        assert!(c.read(9).is_none());
        assert!(c.read(9).is_none());
        assert!(!c.update(9, |_| {}));
    }
}
